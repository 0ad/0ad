/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "ShaderProgram.h"

#include "graphics/Color.h"
#include "graphics/PreprocessorWrapper.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextureManager.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/backend/gl/DeviceCommandContext.h"

#define USE_SHADER_XML_VALIDATION 1

#if USE_SHADER_XML_VALIDATION
#include "ps/XML/RelaxNG.h"
#include "ps/XML/XMLWriter.h"
#endif

#include <algorithm>

namespace Renderer
{

namespace Backend
{

namespace GL
{

namespace
{

GLint GLSizeFromFormat(const Renderer::Backend::Format format)
{
	GLint size = 1;
	if (format == Renderer::Backend::Format::R32_SFLOAT ||
		format == Renderer::Backend::Format::R16_SINT)
		size = 1;
	else if (
		format == Renderer::Backend::Format::R8G8_UNORM ||
		format == Renderer::Backend::Format::R8G8_UINT ||
		format == Renderer::Backend::Format::R16G16_SINT ||
		format == Renderer::Backend::Format::R32G32_SFLOAT)
		size = 2;
	else if (format == Renderer::Backend::Format::R32G32B32_SFLOAT)
		size = 3;
	else if (
		format == Renderer::Backend::Format::R32G32B32A32_SFLOAT ||
		format == Renderer::Backend::Format::R8G8B8A8_UNORM ||
		format == Renderer::Backend::Format::R8G8B8A8_UINT)
		size = 4;
	else
		debug_warn("Unsupported format.");
	return size;
}

GLenum GLTypeFromFormat(const Renderer::Backend::Format format)
{
	GLenum type = GL_FLOAT;
	if (format == Renderer::Backend::Format::R32_SFLOAT ||
		format == Renderer::Backend::Format::R32G32_SFLOAT ||
		format == Renderer::Backend::Format::R32G32B32_SFLOAT ||
		format == Renderer::Backend::Format::R32G32B32A32_SFLOAT)
		type = GL_FLOAT;
	else if (
		format == Renderer::Backend::Format::R16_SINT ||
		format == Renderer::Backend::Format::R16G16_SINT)
		type = GL_SHORT;
	else if (
		format == Renderer::Backend::Format::R8G8_UNORM ||
		format == Renderer::Backend::Format::R8G8_UINT ||
		format == Renderer::Backend::Format::R8G8B8A8_UNORM ||
		format == Renderer::Backend::Format::R8G8B8A8_UINT)
		type = GL_UNSIGNED_BYTE;
	else
		debug_warn("Unsupported format.");
	return type;
}

int GetAttributeLocationFromStream(Renderer::Backend::GL::CDevice* device, const int stream)
{
	// Old mapping makes sense only if we have an old/low-end hardware. Else we
	// need to use sequential numbering to fix #3054. We use presence of
	// compute shaders as a check that the hardware has universal CUs.
	if (device->GetCapabilities().computeShaders)
	{
		switch (stream)
		{
		case STREAM_POS: return 0;
		case STREAM_NORMAL: return 1;
		case STREAM_COLOR: return 2;
		case STREAM_UV0: return 3;
		case STREAM_UV1: return 4;
		case STREAM_UV2: return 5;
		case STREAM_UV3: return 6;
		case STREAM_UV4: return 7;
		case STREAM_UV5: return 8;
		case STREAM_UV6: return 9;
		case STREAM_UV7: return 10;
		}
	}
	else
	{
		// Map known semantics onto the attribute locations documented by NVIDIA:
		//  https://download.nvidia.com/developer/Papers/2005/OpenGL_2.0/NVIDIA_OpenGL_2.0_Support.pdf
		//  https://developer.download.nvidia.com/opengl/glsl/glsl_release_notes.pdf
		switch (stream)
		{
		case STREAM_POS: return 0;
		case STREAM_NORMAL: return 2;
		case STREAM_COLOR: return 3;
		case STREAM_UV0: return 8;
		case STREAM_UV1: return 9;
		case STREAM_UV2: return 10;
		case STREAM_UV3: return 11;
		case STREAM_UV4: return 12;
		case STREAM_UV5: return 13;
		case STREAM_UV6: return 14;
		case STREAM_UV7: return 15;
		}
	}

	debug_warn("Invalid attribute semantics");
	return 0;
}

bool PreprocessShaderFile(
	bool arb, const CShaderDefines& defines, const VfsPath& path,
	CStr& source, std::vector<VfsPath>& fileDependencies)
{
	CVFSFile file;
	if (file.Load(g_VFS, path) != PSRETURN_OK)
	{
		LOGERROR("Failed to load shader file: '%s'", path.string8());
		return false;
	}

	CPreprocessorWrapper preprocessor(
		[arb, &fileDependencies](const CStr& includePath, CStr& out) -> bool
		{
			const VfsPath includeFilePath(
				(arb ? L"shaders/arb/" : L"shaders/glsl/") + wstring_from_utf8(includePath));
			// Add dependencies anyway to reload the shader when the file is
			// appeared.
			fileDependencies.push_back(includeFilePath);
			CVFSFile includeFile;
			if (includeFile.Load(g_VFS, includeFilePath) != PSRETURN_OK)
			{
				LOGERROR("Failed to load shader include file: '%s'", includeFilePath.string8());
				return false;
			}
			out = includeFile.GetAsString();
			return true;
		});
	preprocessor.AddDefines(defines);

#if CONFIG2_GLES
	if (!arb)
	{
		// GLES defines the macro "GL_ES" in its GLSL preprocessor,
		// but since we run our own preprocessor first, we need to explicitly
		// define it here
		preprocessor.AddDefine("GL_ES", "1");
	}
#endif

	source = preprocessor.Preprocess(file.GetAsString());

	return true;
}

} // anonymous namespace

#if !CONFIG2_GLES

class CShaderProgramARB final : public CShaderProgram
{
public:
	CShaderProgramARB(
		CDevice* device,
		const VfsPath& vertexFilePath, const VfsPath& fragmentFilePath,
		const CShaderDefines& defines,
		const std::map<CStrIntern, int>& vertexIndexes, const std::map<CStrIntern, frag_index_pair_t>& fragmentIndexes,
		int streamflags) :
		CShaderProgram(streamflags),
		m_Device(device),
		m_VertexIndexes(vertexIndexes), m_FragmentIndexes(fragmentIndexes)
	{
		glGenProgramsARB(1, &m_VertexProgram);
		glGenProgramsARB(1, &m_FragmentProgram);

		std::vector<VfsPath> newFileDependencies = {vertexFilePath, fragmentFilePath};

		CStr vertexCode;
		if (!PreprocessShaderFile(true, defines, vertexFilePath, vertexCode, newFileDependencies))
			return;
		CStr fragmentCode;
		if (!PreprocessShaderFile(true, defines, fragmentFilePath, fragmentCode, newFileDependencies))
			return;

		m_FileDependencies = std::move(newFileDependencies);

		// TODO: replace by scoped bind.
		m_Device->GetActiveCommandContext()->SetGraphicsPipelineState(
			MakeDefaultGraphicsPipelineStateDesc());

		if (!Compile(GL_VERTEX_PROGRAM_ARB, "vertex", m_VertexProgram, vertexFilePath, vertexCode))
			return;

		if (!Compile(GL_FRAGMENT_PROGRAM_ARB, "fragment", m_FragmentProgram, fragmentFilePath, fragmentCode))
			return;
	}

	~CShaderProgramARB() override
	{
		glDeleteProgramsARB(1, &m_VertexProgram);
		glDeleteProgramsARB(1, &m_FragmentProgram);
	}

	bool Compile(GLuint target, const char* targetName, GLuint program, const VfsPath& file, const CStr& code)
	{
		ogl_WarnIfError();

		glBindProgramARB(target, program);

		ogl_WarnIfError();

		glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)code.length(), code.c_str());

		if (ogl_SquelchError(GL_INVALID_OPERATION))
		{
			GLint errPos = 0;
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
			int errLine = std::count(code.begin(), code.begin() + std::min((int)code.length(), errPos + 1), '\n') + 1;
			char* errStr = (char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
			LOGERROR("Failed to compile %s program '%s' (line %d):\n%s", targetName, file.string8(), errLine, errStr);
			return false;
		}

		glBindProgramARB(target, 0);

		ogl_WarnIfError();

		return true;
	}

	void Bind(CShaderProgram* previousShaderProgram) override
	{
		if (previousShaderProgram)
			previousShaderProgram->Unbind();

		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_VertexProgram);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_FragmentProgram);

		BindClientStates();
	}

	void Unbind() override
	{
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);

		UnbindClientStates();

		// TODO: should unbind textures, probably
	}

	int GetUniformVertexIndex(CStrIntern id)
	{
		std::map<CStrIntern, int>::iterator it = m_VertexIndexes.find(id);
		if (it == m_VertexIndexes.end())
			return -1;
		return it->second;
	}

	frag_index_pair_t GetUniformFragmentIndex(CStrIntern id)
	{
		std::map<CStrIntern, frag_index_pair_t>::iterator it = m_FragmentIndexes.find(id);
		if (it == m_FragmentIndexes.end())
			return std::make_pair(-1, 0);
		return it->second;
	}

	Binding GetTextureBinding(texture_id_t id) override
	{
		frag_index_pair_t fPair = GetUniformFragmentIndex(id);
		int index = fPair.first;
		if (index == -1)
			return Binding();
		else
			return Binding((int)fPair.second, index);
	}

	void BindTexture(texture_id_t id, GLuint tex) override
	{
		frag_index_pair_t fPair = GetUniformFragmentIndex(id);
		int index = fPair.first;
		if (index != -1)
		{
			m_Device->GetActiveCommandContext()->BindTexture(index, fPair.second, tex);
		}
	}

	void BindTexture(Binding id, GLuint tex) override
	{
		int index = id.second;
		if (index != -1)
		{
			m_Device->GetActiveCommandContext()->BindTexture(index, id.first, tex);
		}
	}

	Binding GetUniformBinding(uniform_id_t id) override
	{
		return Binding(GetUniformVertexIndex(id), GetUniformFragmentIndex(id).first);
	}

	void Uniform(Binding id, float v0, float v1, float v2, float v3) override
	{
		if (id.first != -1)
			glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.first, v0, v1, v2, v3);

		if (id.second != -1)
			glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.second, v0, v1, v2, v3);
	}

	void Uniform(Binding id, const CMatrix3D& v) override
	{
		if (id.first != -1)
		{
			glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.first+0, v._11, v._12, v._13, v._14);
			glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.first+1, v._21, v._22, v._23, v._24);
			glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.first+2, v._31, v._32, v._33, v._34);
			glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.first+3, v._41, v._42, v._43, v._44);
		}

		if (id.second != -1)
		{
			glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.second+0, v._11, v._12, v._13, v._14);
			glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.second+1, v._21, v._22, v._23, v._24);
			glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.second+2, v._31, v._32, v._33, v._34);
			glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.second+3, v._41, v._42, v._43, v._44);
		}
	}

	void Uniform(Binding id, size_t count, const CMatrix3D* v) override
	{
		ENSURE(count == 1);
		Uniform(id, v[0]);
	}

	void Uniform(Binding id, size_t count, const float* v) override
	{
		ENSURE(count == 4);
		Uniform(id, v[0], v[1], v[2], v[3]);
	}

	std::vector<VfsPath> GetFileDependencies() const override
	{
		return m_FileDependencies;
	}

private:
	CDevice* m_Device = nullptr;

	std::vector<VfsPath> m_FileDependencies;

	GLuint m_VertexProgram;
	GLuint m_FragmentProgram;

	std::map<CStrIntern, int> m_VertexIndexes;

	// pair contains <index, gltype>
	std::map<CStrIntern, frag_index_pair_t> m_FragmentIndexes;
};

#endif // !CONFIG2_GLES

class CShaderProgramGLSL final : public CShaderProgram
{
public:
	CShaderProgramGLSL(
		CDevice* device, const CStr& name,
		const VfsPath& vertexFilePath, const VfsPath& fragmentFilePath,
		const CShaderDefines& defines,
		const std::map<CStrIntern, int>& vertexAttribs,
		int streamflags) :
	CShaderProgram(streamflags),
		m_Device(device), m_Name(name),
		m_VertexAttribs(vertexAttribs)
	{
		for (std::map<CStrIntern, int>::iterator it = m_VertexAttribs.begin(); it != m_VertexAttribs.end(); ++it)
			m_ActiveVertexAttributes.emplace_back(it->second);
		std::sort(m_ActiveVertexAttributes.begin(), m_ActiveVertexAttributes.end());

		m_Program = 0;
		m_VertexShader = glCreateShader(GL_VERTEX_SHADER);
		m_FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		m_FileDependencies = {vertexFilePath, fragmentFilePath};

#if !CONFIG2_GLES
		if (m_Device->GetCapabilities().debugLabels)
		{
			glObjectLabel(GL_SHADER, m_VertexShader, -1, vertexFilePath.string8().c_str());
			glObjectLabel(GL_SHADER, m_FragmentShader, -1, fragmentFilePath.string8().c_str());
		}
#endif

		std::vector<VfsPath> newFileDependencies = {vertexFilePath, fragmentFilePath};

		CStr vertexCode;
		if (!PreprocessShaderFile(false, defines, vertexFilePath, vertexCode, newFileDependencies))
			return;
		CStr fragmentCode;
		if (!PreprocessShaderFile(false, defines, fragmentFilePath, fragmentCode, newFileDependencies))
			return;

		m_FileDependencies = std::move(newFileDependencies);

		if (vertexCode.empty())
		{
			LOGERROR("Failed to preprocess vertex shader: '%s'", vertexFilePath.string8());
			return;
		}
		if (fragmentCode.empty())
		{
			LOGERROR("Failed to preprocess fragment shader: '%s'", fragmentFilePath.string8());
			return;
		}

#if CONFIG2_GLES
		// Ugly hack to replace desktop GLSL 1.10/1.20 with GLSL ES 1.00,
		// and also to set default float precision for fragment shaders
		vertexCode.Replace("#version 110\n", "#version 100\n");
		vertexCode.Replace("#version 110\r\n", "#version 100\n");
		vertexCode.Replace("#version 120\n", "#version 100\n");
		vertexCode.Replace("#version 120\r\n", "#version 100\n");
		fragmentCode.Replace("#version 110\n", "#version 100\nprecision mediump float;\n");
		fragmentCode.Replace("#version 110\r\n", "#version 100\nprecision mediump float;\n");
		fragmentCode.Replace("#version 120\n", "#version 100\nprecision mediump float;\n");
		fragmentCode.Replace("#version 120\r\n", "#version 100\nprecision mediump float;\n");
#endif

		// TODO: replace by scoped bind.
		m_Device->GetActiveCommandContext()->SetGraphicsPipelineState(
			MakeDefaultGraphicsPipelineStateDesc());

		if (!Compile(m_VertexShader, vertexFilePath, vertexCode))
			return;

		if (!Compile(m_FragmentShader, fragmentFilePath, fragmentCode))
			return;

		if (!Link(vertexFilePath, fragmentFilePath))
			return;
	}

	~CShaderProgramGLSL() override
	{
		if (m_Program)
			glDeleteProgram(m_Program);

		glDeleteShader(m_VertexShader);
		glDeleteShader(m_FragmentShader);
	}

	bool Compile(GLuint shader, const VfsPath& file, const CStr& code)
	{
		const char* code_string = code.c_str();
		GLint code_length = code.length();
		glShaderSource(shader, 1, &code_string, &code_length);

		ogl_WarnIfError();

		glCompileShader(shader);

		GLint ok = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

		GLint length = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

		// Apparently sometimes GL_INFO_LOG_LENGTH is incorrectly reported as 0
		// (http://code.google.com/p/android/issues/detail?id=9953)
		if (!ok && length == 0)
			length = 4096;

		if (length > 1)
		{
			char* infolog = new char[length];
			glGetShaderInfoLog(shader, length, NULL, infolog);

			if (ok)
				LOGMESSAGE("Info when compiling shader '%s':\n%s", file.string8(), infolog);
			else
				LOGERROR("Failed to compile shader '%s':\n%s", file.string8(), infolog);

			delete[] infolog;
		}

		ogl_WarnIfError();

		return ok;
	}

	bool Link(const VfsPath& vertexFilePath, const VfsPath& fragmentFilePath)
	{
		ENSURE(!m_Program);
		m_Program = glCreateProgram();

#if !CONFIG2_GLES
		if (m_Device->GetCapabilities().debugLabels)
		{
			glObjectLabel(GL_PROGRAM, m_Program, -1, m_Name.c_str());
		}
#endif

		glAttachShader(m_Program, m_VertexShader);
		ogl_WarnIfError();
		glAttachShader(m_Program, m_FragmentShader);
		ogl_WarnIfError();

		// Set up the attribute bindings explicitly, since apparently drivers
		// don't always pick the most efficient bindings automatically,
		// and also this lets us hardcode indexes into VertexPointer etc
		for (std::map<CStrIntern, int>::iterator it = m_VertexAttribs.begin(); it != m_VertexAttribs.end(); ++it)
			glBindAttribLocation(m_Program, it->second, it->first.c_str());

		glLinkProgram(m_Program);

		GLint ok = 0;
		glGetProgramiv(m_Program, GL_LINK_STATUS, &ok);

		GLint length = 0;
		glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &length);

		if (!ok && length == 0)
			length = 4096;

		if (length > 1)
		{
			char* infolog = new char[length];
			glGetProgramInfoLog(m_Program, length, NULL, infolog);

			if (ok)
				LOGMESSAGE("Info when linking program '%s'+'%s':\n%s", vertexFilePath.string8(), fragmentFilePath.string8(), infolog);
			else
				LOGERROR("Failed to link program '%s'+'%s':\n%s", vertexFilePath.string8(), fragmentFilePath.string8(), infolog);

			delete[] infolog;
		}

		ogl_WarnIfError();

		if (!ok)
			return false;

		m_Uniforms.clear();
		m_Samplers.clear();

		Bind(nullptr);

		ogl_WarnIfError();

		GLint numUniforms = 0;
		glGetProgramiv(m_Program, GL_ACTIVE_UNIFORMS, &numUniforms);
		ogl_WarnIfError();
		for (GLint i = 0; i < numUniforms; ++i)
		{
			char name[256] = {0};
			GLsizei nameLength = 0;
			GLint size = 0;
			GLenum type = 0;
			glGetActiveUniform(m_Program, i, ARRAY_SIZE(name), &nameLength, &size, &type, name);
			ogl_WarnIfError();

			GLint loc = glGetUniformLocation(m_Program, name);

			CStrIntern nameIntern(name);
			m_Uniforms[nameIntern] = std::make_pair(loc, type);

			// Assign sampler uniforms to sequential texture units
			if (type == GL_SAMPLER_2D
			 || type == GL_SAMPLER_CUBE
#if !CONFIG2_GLES
			 || type == GL_SAMPLER_2D_SHADOW
#endif
			)
			{
				int unit = (int)m_Samplers.size();
				m_Samplers[nameIntern].first = (type == GL_SAMPLER_CUBE ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D);
				m_Samplers[nameIntern].second = unit;
				glUniform1i(loc, unit); // link uniform to unit
				ogl_WarnIfError();
			}
		}

		// TODO: verify that we're not using more samplers than is supported

		Unbind();

		ogl_WarnIfError();

		return true;
	}

	void Bind(CShaderProgram* previousShaderProgram) override
	{
		CShaderProgramGLSL* previousShaderProgramGLSL = nullptr;
		if (previousShaderProgram)
			previousShaderProgramGLSL = static_cast<CShaderProgramGLSL*>(previousShaderProgram);
		ENSURE(this != previousShaderProgramGLSL);

		glUseProgram(m_Program);

		if (previousShaderProgramGLSL)
		{
			std::vector<int>::iterator itPrevious = previousShaderProgramGLSL->m_ActiveVertexAttributes.begin();
			std::vector<int>::iterator itNext = m_ActiveVertexAttributes.begin();
			while (
				itPrevious != previousShaderProgramGLSL->m_ActiveVertexAttributes.end() ||
				itNext != m_ActiveVertexAttributes.end())
			{
				if (itPrevious != previousShaderProgramGLSL->m_ActiveVertexAttributes.end() &&
					itNext != m_ActiveVertexAttributes.end())
				{
					if (*itPrevious == *itNext)
					{
						++itPrevious;
						++itNext;
					}
					else if (*itPrevious < *itNext)
					{
						glDisableVertexAttribArray(*itPrevious);
						++itPrevious;
					}
					else if (*itPrevious > *itNext)
					{
						glEnableVertexAttribArray(*itNext);
						++itNext;
					}
				}
				else if (itPrevious != previousShaderProgramGLSL->m_ActiveVertexAttributes.end())
				{
					glDisableVertexAttribArray(*itPrevious);
					++itPrevious;
				}
				else if (itNext != m_ActiveVertexAttributes.end())
				{
					glEnableVertexAttribArray(*itNext);
					++itNext;
				}
			}
		}
		else
		{
			for (const int index : m_ActiveVertexAttributes)
				glEnableVertexAttribArray(index);
		}
	}

	void Unbind() override
	{
		glUseProgram(0);

		for (const int index : m_ActiveVertexAttributes)
			glDisableVertexAttribArray(index);

		// TODO: should unbind textures, probably
	}

	Binding GetTextureBinding(texture_id_t id) override
	{
		std::map<CStrIntern, std::pair<GLenum, int>>::iterator it = m_Samplers.find(CStrIntern(id));
		if (it == m_Samplers.end())
			return Binding();
		else
			return Binding((int)it->second.first, it->second.second);
	}

	void BindTexture(texture_id_t id, GLuint tex) override
	{
		std::map<CStrIntern, std::pair<GLenum, int>>::iterator it = m_Samplers.find(CStrIntern(id));
		if (it == m_Samplers.end())
			return;

		m_Device->GetActiveCommandContext()->BindTexture(it->second.second, it->second.first, tex);
	}

	void BindTexture(Binding id, GLuint tex) override
	{
		if (id.second == -1)
			return;

		m_Device->GetActiveCommandContext()->BindTexture(id.second, id.first, tex);
	}

	Binding GetUniformBinding(uniform_id_t id) override
	{
		std::map<CStrIntern, std::pair<int, GLenum>>::iterator it = m_Uniforms.find(id);
		if (it == m_Uniforms.end())
			return Binding();
		else
			return Binding(it->second.first, (int)it->second.second);
	}

	void Uniform(Binding id, float v0, float v1, float v2, float v3) override
	{
		if (id.first != -1)
		{
			if (id.second == GL_FLOAT)
				glUniform1f(id.first, v0);
			else if (id.second == GL_FLOAT_VEC2)
				glUniform2f(id.first, v0, v1);
			else if (id.second == GL_FLOAT_VEC3)
				glUniform3f(id.first, v0, v1, v2);
			else if (id.second == GL_FLOAT_VEC4)
				glUniform4f(id.first, v0, v1, v2, v3);
			else
				LOGERROR("CShaderProgramGLSL::Uniform(): Invalid uniform type (expected float, vec2, vec3, vec4)");
		}
	}

	void Uniform(Binding id, const CMatrix3D& v) override
	{
		if (id.first != -1)
		{
			if (id.second == GL_FLOAT_MAT4)
				glUniformMatrix4fv(id.first, 1, GL_FALSE, &v._11);
			else
				LOGERROR("CShaderProgramGLSL::Uniform(): Invalid uniform type (expected mat4)");
		}
	}

	void Uniform(Binding id, size_t count, const CMatrix3D* v) override
	{
		if (id.first != -1)
		{
			if (id.second == GL_FLOAT_MAT4)
				glUniformMatrix4fv(id.first, count, GL_FALSE, &v->_11);
			else
				LOGERROR("CShaderProgramGLSL::Uniform(): Invalid uniform type (expected mat4)");
		}
	}

	void Uniform(Binding id, size_t count, const float* v) override
	{
		if (id.first != -1)
		{
			if (id.second == GL_FLOAT)
				glUniform1fv(id.first, count, v);
			else
				LOGERROR("CShaderProgramGLSL::Uniform(): Invalid uniform type (expected float)");
		}
	}

	// Map the various fixed-function Pointer functions onto generic vertex attributes
	// (matching the attribute indexes from ShaderManager's ParseAttribSemantics):

	void VertexPointer(const Renderer::Backend::Format format, GLsizei stride, const void* pointer) override
	{
		const GLint size = GLSizeFromFormat(format);
		const GLenum type = GLTypeFromFormat(format);
		glVertexAttribPointer(0, size, type, GL_FALSE, stride, pointer);
		m_ValidStreams |= STREAM_POS;
	}

	void NormalPointer(const Renderer::Backend::Format format, GLsizei stride, const void* pointer) override
	{
		const GLint size = GLSizeFromFormat(format);
		const GLenum type = GLTypeFromFormat(format);
		glVertexAttribPointer(m_Device->GetCapabilities().computeShaders ? 1 : 2, size, type, (type == GL_FLOAT ? GL_FALSE : GL_TRUE), stride, pointer);
		m_ValidStreams |= STREAM_NORMAL;
	}

	void ColorPointer(const Renderer::Backend::Format format, GLsizei stride, const void* pointer) override
	{
		const GLint size = GLSizeFromFormat(format);
		const GLenum type = GLTypeFromFormat(format);
		glVertexAttribPointer(m_Device->GetCapabilities().computeShaders ? 2 : 3, size, type, (type == GL_FLOAT ? GL_FALSE : GL_TRUE), stride, pointer);
		m_ValidStreams |= STREAM_COLOR;
	}

	void TexCoordPointer(GLenum texture, const Renderer::Backend::Format format, GLsizei stride, const void* pointer) override
	{
		const GLint size = GLSizeFromFormat(format);
		const GLenum type = GLTypeFromFormat(format);
		glVertexAttribPointer(
			(m_Device->GetCapabilities().computeShaders ? 3 : 8) + texture - GL_TEXTURE0, size, type, GL_FALSE, stride, pointer);
		m_ValidStreams |= STREAM_UV0 << (texture - GL_TEXTURE0);
	}

	void VertexAttribPointer(attrib_id_t id, const Renderer::Backend::Format format, GLboolean normalized, GLsizei stride, const void* pointer) override
	{
		std::map<CStrIntern, int>::iterator it = m_VertexAttribs.find(id);
		if (it != m_VertexAttribs.end())
		{
			const GLint size = GLSizeFromFormat(format);
			const GLenum type = GLTypeFromFormat(format);
			glVertexAttribPointer(it->second, size, type, normalized, stride, pointer);
			m_ValidStreams |= STREAM_UV0 << (it->second - (m_Device->GetCapabilities().computeShaders ? 3 : 8));
		}
	}

	std::vector<VfsPath> GetFileDependencies() const override
	{
		return m_FileDependencies;
	}

private:
	CDevice* m_Device = nullptr;

	CStr m_Name;
	std::vector<VfsPath> m_FileDependencies;

	std::map<CStrIntern, int> m_VertexAttribs;
	// Sorted list of active vertex attributes.
	std::vector<int> m_ActiveVertexAttributes;

	GLuint m_Program;
	GLuint m_VertexShader, m_FragmentShader;

	std::map<CStrIntern, std::pair<int, GLenum>> m_Uniforms;
	std::map<CStrIntern, std::pair<GLenum, int>> m_Samplers; // texture target & unit chosen for each uniform sampler
};

CShaderProgram::CShaderProgram(int streamflags)
	: m_StreamFlags(streamflags), m_ValidStreams(0)
{
}

CShaderProgram::~CShaderProgram() = default;

// static
std::unique_ptr<CShaderProgram> CShaderProgram::Create(CDevice* device, const CStr& name, const CShaderDefines& baseDefines)
{
	PROFILE2("loading shader");
	PROFILE2_ATTR("name: %s", name.c_str());

	VfsPath xmlFilename = L"shaders/" + wstring_from_utf8(name) + L".xml";

	CXeromyces XeroFile;
	PSRETURN ret = XeroFile.Load(g_VFS, xmlFilename);
	if (ret != PSRETURN_OK)
		return nullptr;

#if USE_SHADER_XML_VALIDATION
	{
		// Serialize the XMB data and pass it to the validator
		XMLWriter_File shaderFile;
		shaderFile.SetPrettyPrint(false);
		shaderFile.XMB(XeroFile);
		bool ok = CXeromyces::ValidateEncoded("shader", name, shaderFile.GetOutput());
		if (!ok)
			return nullptr;
	}
#endif

	// Define all the elements and attributes used in the XML file
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(define);
	EL(fragment);
	EL(stream);
	EL(uniform);
	EL(vertex);
	AT(attribute);
	AT(file);
	AT(if);
	AT(loc);
	AT(name);
	AT(type);
	AT(value);
#undef AT
#undef EL

	CPreprocessorWrapper preprocessor;
	preprocessor.AddDefines(baseDefines);

	XMBElement root = XeroFile.GetRoot();

	VfsPath vertexFile;
	VfsPath fragmentFile;
	CShaderDefines defines = baseDefines;
	std::map<CStrIntern, int> vertexUniforms;
	std::map<CStrIntern, CShaderProgram::frag_index_pair_t> fragmentUniforms;
	std::map<CStrIntern, int> vertexAttribs;
	int streamFlags = 0;

	XERO_ITER_EL(root, child)
	{
		if (child.GetNodeName() == el_define)
		{
			defines.Add(CStrIntern(child.GetAttributes().GetNamedItem(at_name)), CStrIntern(child.GetAttributes().GetNamedItem(at_value)));
		}
		else if (child.GetNodeName() == el_vertex)
		{
			vertexFile = L"shaders/" + child.GetAttributes().GetNamedItem(at_file).FromUTF8();

			XERO_ITER_EL(child, param)
			{
				XMBAttributeList attributes = param.GetAttributes();

				CStr cond = attributes.GetNamedItem(at_if);
				if (!cond.empty() && !preprocessor.TestConditional(cond))
					continue;

				if (param.GetNodeName() == el_uniform)
				{
					vertexUniforms[CStrIntern(attributes.GetNamedItem(at_name))] = attributes.GetNamedItem(at_loc).ToInt();
				}
				else if (param.GetNodeName() == el_stream)
				{
					const CStr streamName = attributes.GetNamedItem(at_name);
					const CStr attributeName = attributes.GetNamedItem(at_attribute);
					if (attributeName.empty())
						LOGERROR("Empty attribute name in vertex shader description '%s'", vertexFile.string8().c_str());

					int stream = 0;
					if (streamName == "pos")
						stream = STREAM_POS;
					else if (streamName == "normal")
						stream = STREAM_NORMAL;
					else if (streamName == "color")
						stream = STREAM_COLOR;
					else if (streamName == "uv0")
						stream = STREAM_UV0;
					else if (streamName == "uv1")
						stream = STREAM_UV1;
					else if (streamName == "uv2")
						stream = STREAM_UV2;
					else if (streamName == "uv3")
						stream = STREAM_UV3;
					else if (streamName == "uv4")
						stream = STREAM_UV4;
					else if (streamName == "uv5")
						stream = STREAM_UV5;
					else if (streamName == "uv6")
						stream = STREAM_UV6;
					else if (streamName == "uv7")
						stream = STREAM_UV7;
					else
						LOGERROR("Unknown stream '%s' in vertex shader description '%s'", streamName.c_str(), vertexFile.string8().c_str());

					const int attributeLocation = GetAttributeLocationFromStream(device, stream);
					vertexAttribs[CStrIntern(attributeName)] = attributeLocation;
					streamFlags |= stream;
				}
			}
		}
		else if (child.GetNodeName() == el_fragment)
		{
			fragmentFile = L"shaders/" + child.GetAttributes().GetNamedItem(at_file).FromUTF8();

			XERO_ITER_EL(child, param)
			{
				XMBAttributeList attributes = param.GetAttributes();

				CStr cond = attributes.GetNamedItem(at_if);
				if (!cond.empty() && !preprocessor.TestConditional(cond))
					continue;

				if (param.GetNodeName() == el_uniform)
				{
					// A somewhat incomplete listing, missing "shadow" and "rect" versions
					// which are interpreted as 2D (NB: our shadowmaps may change
					// type based on user config).
					GLenum type = GL_TEXTURE_2D;
					const CStr t = attributes.GetNamedItem(at_type);
					if (t == "sampler1D")
#if CONFIG2_GLES
						debug_warn(L"sampler1D not implemented on GLES");
#else
						type = GL_TEXTURE_1D;
#endif
					else if (t == "sampler2D")
						type = GL_TEXTURE_2D;
					else if (t == "sampler3D")
#if CONFIG2_GLES
						debug_warn(L"sampler3D not implemented on GLES");
#else
						type = GL_TEXTURE_3D;
#endif
					else if (t == "samplerCube")
						type = GL_TEXTURE_CUBE_MAP;

					fragmentUniforms[CStrIntern(attributes.GetNamedItem(at_name))] =
						std::make_pair(attributes.GetNamedItem(at_loc).ToInt(), type);
				}
			}
		}
	}

	if (root.GetAttributes().GetNamedItem(at_type) == "glsl")
		return CShaderProgram::ConstructGLSL(device, name, vertexFile, fragmentFile, defines, vertexAttribs, streamFlags);
	else
		return CShaderProgram::ConstructARB(device, vertexFile, fragmentFile, defines, vertexUniforms, fragmentUniforms, streamFlags);
}

#if CONFIG2_GLES
// static
std::unique_ptr<CShaderProgram> CShaderProgram::ConstructARB(CDevice* UNUSED(device), const VfsPath& vertexFile, const VfsPath& fragmentFile,
	const CShaderDefines& UNUSED(defines),
	const std::map<CStrIntern, int>& UNUSED(vertexIndexes), const std::map<CStrIntern, frag_index_pair_t>& UNUSED(fragmentIndexes),
	int UNUSED(streamflags))
{
	LOGERROR("CShaderProgram::ConstructARB: '%s'+'%s': ARB shaders not supported on this device",
		vertexFile.string8(), fragmentFile.string8());
	return nullptr;
}
#else
// static
std::unique_ptr<CShaderProgram> CShaderProgram::ConstructARB(
	CDevice* device, const VfsPath& vertexFile, const VfsPath& fragmentFile,
	const CShaderDefines& defines,
	const std::map<CStrIntern, int>& vertexIndexes,
	const std::map<CStrIntern, frag_index_pair_t>& fragmentIndexes,
	int streamflags)
{
	return std::make_unique<CShaderProgramARB>(
		device, vertexFile, fragmentFile, defines, vertexIndexes, fragmentIndexes, streamflags);
}
#endif

// static
std::unique_ptr<CShaderProgram> CShaderProgram::ConstructGLSL(
	CDevice* device, const CStr& name,
	const VfsPath& vertexFile, const VfsPath& fragmentFile,
	const CShaderDefines& defines,
	const std::map<CStrIntern, int>& vertexAttribs, int streamflags)
{
	return std::make_unique<CShaderProgramGLSL>(
		device, name, vertexFile, fragmentFile, defines, vertexAttribs, streamflags);
}

int CShaderProgram::GetStreamFlags() const
{
	return m_StreamFlags;
}

void CShaderProgram::BindTexture(texture_id_t id, const Renderer::Backend::GL::CTexture* tex)
{
	BindTexture(id, tex->GetHandle());
}

void CShaderProgram::BindTexture(Binding id, const Renderer::Backend::GL::CTexture* tex)
{
	BindTexture(id, tex->GetHandle());
}

void CShaderProgram::Uniform(Binding id, int v)
{
	Uniform(id, (float)v, (float)v, (float)v, (float)v);
}

void CShaderProgram::Uniform(Binding id, float v)
{
	Uniform(id, v, v, v, v);
}

void CShaderProgram::Uniform(Binding id, float v0, float v1)
{
	Uniform(id, v0, v1, 0.0f, 0.0f);
}

void CShaderProgram::Uniform(Binding id, const CVector3D& v)
{
	Uniform(id, v.X, v.Y, v.Z, 0.0f);
}

void CShaderProgram::Uniform(Binding id, const CColor& v)
{
	Uniform(id, v.r, v.g, v.b, v.a);
}

void CShaderProgram::Uniform(uniform_id_t id, int v)
{
	Uniform(GetUniformBinding(id), (float)v, (float)v, (float)v, (float)v);
}

void CShaderProgram::Uniform(uniform_id_t id, float v)
{
	Uniform(GetUniformBinding(id), v, v, v, v);
}

void CShaderProgram::Uniform(uniform_id_t id, float v0, float v1)
{
	Uniform(GetUniformBinding(id), v0, v1, 0.0f, 0.0f);
}

void CShaderProgram::Uniform(uniform_id_t id, const CVector3D& v)
{
	Uniform(GetUniformBinding(id), v.X, v.Y, v.Z, 0.0f);
}

void CShaderProgram::Uniform(uniform_id_t id, const CColor& v)
{
	Uniform(GetUniformBinding(id), v.r, v.g, v.b, v.a);
}

void CShaderProgram::Uniform(uniform_id_t id, float v0, float v1, float v2, float v3)
{
	Uniform(GetUniformBinding(id), v0, v1, v2, v3);
}

void CShaderProgram::Uniform(uniform_id_t id, const CMatrix3D& v)
{
	Uniform(GetUniformBinding(id), v);
}

void CShaderProgram::Uniform(uniform_id_t id, size_t count, const CMatrix3D* v)
{
	Uniform(GetUniformBinding(id), count, v);
}

void CShaderProgram::Uniform(uniform_id_t id, size_t count, const float* v)
{
	Uniform(GetUniformBinding(id), count, v);
}


// These should all be overridden by CShaderProgramGLSL, and not used
// if a non-GLSL shader was loaded instead:

void CShaderProgram::VertexAttribPointer(attrib_id_t UNUSED(id), const Renderer::Backend::Format UNUSED(format),
	GLboolean UNUSED(normalized), GLsizei UNUSED(stride), const void* UNUSED(pointer))
{
	debug_warn("Shader type doesn't support VertexAttribPointer");
}

#if CONFIG2_GLES

// These should all be overridden by CShaderProgramGLSL
// (GLES doesn't support any other types of shader program):

void CShaderProgram::VertexPointer(const Renderer::Backend::Format UNUSED(format), GLsizei UNUSED(stride), const void* UNUSED(pointer))
{
	debug_warn("CShaderProgram::VertexPointer should be overridden");
}
void CShaderProgram::NormalPointer(const Renderer::Backend::Format UNUSED(format), GLsizei UNUSED(stride), const void* UNUSED(pointer))
{
	debug_warn("CShaderProgram::NormalPointer should be overridden");
}
void CShaderProgram::ColorPointer(const Renderer::Backend::Format UNUSED(format), GLsizei UNUSED(stride), const void* UNUSED(pointer))
{
	debug_warn("CShaderProgram::ColorPointer should be overridden");
}
void CShaderProgram::TexCoordPointer(GLenum UNUSED(texture), const Renderer::Backend::Format UNUSED(format), GLsizei UNUSED(stride), const void* UNUSED(pointer))
{
	debug_warn("CShaderProgram::TexCoordPointer should be overridden");
}

#else

// These are overridden by CShaderProgramGLSL, but fixed-function and ARB shaders
// both use the fixed-function vertex attribute pointers so we'll share their
// definitions here:

void CShaderProgram::VertexPointer(const Renderer::Backend::Format format, GLsizei stride, const void* pointer)
{
	const GLint size = GLSizeFromFormat(format);
	ENSURE(2 <= size && size <= 4);
	const GLenum type = GLTypeFromFormat(format);
	glVertexPointer(size, type, stride, pointer);
	m_ValidStreams |= STREAM_POS;
}

void CShaderProgram::NormalPointer(const Renderer::Backend::Format format, GLsizei stride, const void* pointer)
{
	ENSURE(format == Renderer::Backend::Format::R32G32B32_SFLOAT);
	glNormalPointer(GL_FLOAT, stride, pointer);
	m_ValidStreams |= STREAM_NORMAL;
}

void CShaderProgram::ColorPointer(const Renderer::Backend::Format format, GLsizei stride, const void* pointer)
{
	const GLint size = GLSizeFromFormat(format);
	ENSURE(3 <= size && size <= 4);
	const GLenum type = GLTypeFromFormat(format);
	glColorPointer(size, type, stride, pointer);
	m_ValidStreams |= STREAM_COLOR;
}

void CShaderProgram::TexCoordPointer(GLenum texture, const Renderer::Backend::Format format, GLsizei stride, const void* pointer)
{
	glClientActiveTextureARB(texture);
	const GLint size = GLSizeFromFormat(format);
	ENSURE(1 <= size && size <= 4);
	const GLenum type = GLTypeFromFormat(format);
	glTexCoordPointer(size, type, stride, pointer);
	glClientActiveTextureARB(GL_TEXTURE0);
	m_ValidStreams |= STREAM_UV0 << (texture - GL_TEXTURE0);
}

void CShaderProgram::BindClientStates()
{
	ENSURE(m_StreamFlags == (m_StreamFlags & (STREAM_POS|STREAM_NORMAL|STREAM_COLOR|STREAM_UV0|STREAM_UV1)));

	// Enable all the desired client states for non-GLSL rendering

	if (m_StreamFlags & STREAM_POS)    glEnableClientState(GL_VERTEX_ARRAY);
	if (m_StreamFlags & STREAM_NORMAL) glEnableClientState(GL_NORMAL_ARRAY);
	if (m_StreamFlags & STREAM_COLOR)  glEnableClientState(GL_COLOR_ARRAY);

	if (m_StreamFlags & STREAM_UV0)
	{
		glClientActiveTextureARB(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if (m_StreamFlags & STREAM_UV1)
	{
		glClientActiveTextureARB(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glClientActiveTextureARB(GL_TEXTURE0);
	}

	// Rendering code must subsequently call VertexPointer etc for all of the streams
	// that were activated in this function, else AssertPointersBound will complain
	// that some arrays were unspecified
	m_ValidStreams = 0;
}

void CShaderProgram::UnbindClientStates()
{
	if (m_StreamFlags & STREAM_POS)    glDisableClientState(GL_VERTEX_ARRAY);
	if (m_StreamFlags & STREAM_NORMAL) glDisableClientState(GL_NORMAL_ARRAY);
	if (m_StreamFlags & STREAM_COLOR)  glDisableClientState(GL_COLOR_ARRAY);

	if (m_StreamFlags & STREAM_UV0)
	{
		glClientActiveTextureARB(GL_TEXTURE0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if (m_StreamFlags & STREAM_UV1)
	{
		glClientActiveTextureARB(GL_TEXTURE1);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glClientActiveTextureARB(GL_TEXTURE0);
	}
}

#endif // !CONFIG2_GLES

void CShaderProgram::AssertPointersBound()
{
	ENSURE((m_StreamFlags & ~m_ValidStreams) == 0);
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
