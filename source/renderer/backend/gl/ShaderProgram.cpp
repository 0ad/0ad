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
#include <map>
#include <unordered_map>

namespace Renderer
{

namespace Backend
{

namespace GL
{

namespace
{

struct Binding
{
	Binding(int a, int b) : first(a), second(b) { }

	Binding() : first(-1), second(-1) { }

	/**
		* Returns whether this uniform attribute is active in the shader.
		* If not then there's no point calling Uniform() to set its value.
		*/
	bool Active() const { return first != -1 || second != -1; }

	int first;
	int second;
};

int GetStreamMask(const VertexAttributeStream stream)
{
	return 1 << static_cast<int>(stream);
}

GLint GLSizeFromFormat(const Format format)
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

GLenum GLTypeFromFormat(const Format format)
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

GLboolean NormalizedFromFormat(const Format format)
{
	switch (format)
	{
	case Format::R8G8_UNORM: FALLTHROUGH;
	case Format::R8G8B8_UNORM: FALLTHROUGH;
	case Format::R8G8B8A8_UNORM: FALLTHROUGH;
	case Format::R16_UNORM: FALLTHROUGH;
	case Format::R16G16_UNORM:
		return GL_TRUE;
	default:
		break;
	}
	return GL_FALSE;
}

int GetAttributeLocationFromStream(
	CDevice* device, const VertexAttributeStream stream)
{
	// Old mapping makes sense only if we have an old/low-end hardware. Else we
	// need to use sequential numbering to fix #3054. We use presence of
	// compute shaders as a check that the hardware has universal CUs.
	if (device->GetCapabilities().computeShaders)
	{
		return static_cast<int>(stream);
	}
	else
	{
		// Map known semantics onto the attribute locations documented by NVIDIA:
		//  https://download.nvidia.com/developer/Papers/2005/OpenGL_2.0/NVIDIA_OpenGL_2.0_Support.pdf
		//  https://developer.download.nvidia.com/opengl/glsl/glsl_release_notes.pdf
		switch (stream)
		{
		case VertexAttributeStream::POSITION: return 0;
		case VertexAttributeStream::NORMAL: return 2;
		case VertexAttributeStream::COLOR: return 3;
		case VertexAttributeStream::UV0: return 8;
		case VertexAttributeStream::UV1: return 9;
		case VertexAttributeStream::UV2: return 10;
		case VertexAttributeStream::UV3: return 11;
		case VertexAttributeStream::UV4: return 12;
		case VertexAttributeStream::UV5: return 13;
		case VertexAttributeStream::UV6: return 14;
		case VertexAttributeStream::UV7: return 15;
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

#if !CONFIG2_GLES
std::tuple<GLenum, GLenum, GLint> GetElementTypeAndCountFromString(const CStr& str)
{
#define CASE(MATCH_STRING, TYPE, ELEMENT_TYPE, ELEMENT_COUNT) \
	if (str == MATCH_STRING) return {GL_ ## TYPE, GL_ ## ELEMENT_TYPE, ELEMENT_COUNT}

	CASE("float", FLOAT, FLOAT, 1);
	CASE("vec2", FLOAT_VEC2, FLOAT, 2);
	CASE("vec3", FLOAT_VEC3, FLOAT, 3);
	CASE("vec4", FLOAT_VEC4, FLOAT, 4);
	CASE("mat2", FLOAT_MAT2, FLOAT, 4);
	CASE("mat3", FLOAT_MAT3, FLOAT, 9);
	CASE("mat4", FLOAT_MAT4, FLOAT, 16);
#if !CONFIG2_GLES // GL ES 2.0 doesn't support non-square matrices.
	CASE("mat2x3", FLOAT_MAT2x3, FLOAT, 6);
	CASE("mat2x4", FLOAT_MAT2x4, FLOAT, 8);
	CASE("mat3x2", FLOAT_MAT3x2, FLOAT, 6);
	CASE("mat3x4", FLOAT_MAT3x4, FLOAT, 12);
	CASE("mat4x2", FLOAT_MAT4x2, FLOAT, 8);
	CASE("mat4x3", FLOAT_MAT4x3, FLOAT, 12);
#endif

	// A somewhat incomplete listing, missing "shadow" and "rect" versions
	// which are interpreted as 2D (NB: our shadowmaps may change
	// type based on user config).
#if CONFIG2_GLES
	if (str == "sampler1D") debug_warn(L"sampler1D not implemented on GLES");
#else
	CASE("sampler1D", SAMPLER_1D, TEXTURE_1D, 1);
#endif
	CASE("sampler2D", SAMPLER_2D, TEXTURE_2D, 1);
#if CONFIG2_GLES
	if (str == "sampler2DShadow") debug_warn(L"sampler2DShadow not implemented on GLES");
	if (str == "sampler3D") debug_warn(L"sampler3D not implemented on GLES");
#else
	CASE("sampler2DShadow", SAMPLER_2D_SHADOW, TEXTURE_2D, 1);
	CASE("sampler3D", SAMPLER_3D, TEXTURE_3D, 1);
#endif
	CASE("samplerCube", SAMPLER_CUBE, TEXTURE_CUBE_MAP, 1);

#undef CASE
	return {0, 0, 0};
}
#endif // !CONFIG2_GLES

} // anonymous namespace

#if !CONFIG2_GLES

class CShaderProgramARB final : public CShaderProgram
{
public:
	CShaderProgramARB(
		CDevice* device,
		const VfsPath& path, const VfsPath& vertexFilePath, const VfsPath& fragmentFilePath,
		const CShaderDefines& defines,
		const std::map<CStrIntern, std::pair<CStr, int>>& vertexIndices,
		const std::map<CStrIntern, std::pair<CStr, int>>& fragmentIndices,
		int streamflags)
		: CShaderProgram(streamflags), m_Device(device)
	{
		glGenProgramsARB(1, &m_VertexProgram);
		glGenProgramsARB(1, &m_FragmentProgram);

		std::vector<VfsPath> newFileDependencies = {path, vertexFilePath, fragmentFilePath};

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

		for (const auto& index : vertexIndices)
		{
			BindingSlot& bindingSlot = GetOrCreateBindingSlot(index.first);
			bindingSlot.vertexProgramLocation = index.second.second;
			const auto [type, elementType, elementCount] = GetElementTypeAndCountFromString(index.second.first);
			bindingSlot.type = type;
			bindingSlot.elementType = elementType;
			bindingSlot.elementCount = elementCount;
		}

		for (const auto& index : fragmentIndices)
		{
			BindingSlot& bindingSlot = GetOrCreateBindingSlot(index.first);
			bindingSlot.fragmentProgramLocation = index.second.second;
			const auto [type, elementType, elementCount] = GetElementTypeAndCountFromString(index.second.first);
			if (bindingSlot.type && type != bindingSlot.type)
			{
				LOGERROR("CShaderProgramARB: vertex and fragment program uniforms with the same name should have the same type.");
			}
			bindingSlot.type = type;
			bindingSlot.elementType = elementType;
			bindingSlot.elementCount = elementCount;
		}
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
	}

	IDevice* GetDevice() override { return m_Device; }

	int32_t GetBindingSlot(const CStrIntern name) const override
	{
		auto it = m_BindingSlotsMapping.find(name);
		return it == m_BindingSlotsMapping.end() ? -1 : it->second;
	}

	TextureUnit GetTextureUnit(const int32_t bindingSlot) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return { 0, 0, 0 };
		TextureUnit textureUnit;
		textureUnit.type = m_BindingSlots[bindingSlot].type;
		textureUnit.target = m_BindingSlots[bindingSlot].elementType;
		textureUnit.unit = m_BindingSlots[bindingSlot].fragmentProgramLocation;
		return textureUnit;
	}

	void SetUniform(
		const int32_t bindingSlot,
		const float value) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return;
		if (m_BindingSlots[bindingSlot].type != GL_FLOAT)
		{
			LOGERROR("CShaderProgramARB::SetUniform(): Invalid uniform type (expected float)");
			return;
		}
		SetUniform(m_BindingSlots[bindingSlot], value, 0.0f, 0.0f, 0.0f);
	}

	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return;
		if (m_BindingSlots[bindingSlot].type != GL_FLOAT_VEC2)
		{
			LOGERROR("CShaderProgramARB::SetUniform(): Invalid uniform type (expected vec2)");
			return;
		}
		SetUniform(m_BindingSlots[bindingSlot], valueX, valueY, 0.0f, 0.0f);
	}

	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY, const float valueZ) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return;
		if (m_BindingSlots[bindingSlot].type != GL_FLOAT_VEC3)
		{
			LOGERROR("CShaderProgramARB::SetUniform(): Invalid uniform type (expected vec3)");
			return;
		}
		SetUniform(m_BindingSlots[bindingSlot], valueX, valueY, valueZ, 0.0f);
	}

	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY,
		const float valueZ, const float valueW) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return;
		if (m_BindingSlots[bindingSlot].type != GL_FLOAT_VEC4)
		{
			LOGERROR("CShaderProgramARB::SetUniform(): Invalid uniform type (expected vec4)");
			return;
		}
		SetUniform(m_BindingSlots[bindingSlot], valueX, valueY, valueZ, valueW);
	}

	void SetUniform(
		const int32_t bindingSlot, PS::span<const float> values) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return;
		if (m_BindingSlots[bindingSlot].elementType != GL_FLOAT)
		{
			LOGERROR("CShaderProgramARB::SetUniform(): Invalid uniform element type (expected float)");
			return;
		}
		if (m_BindingSlots[bindingSlot].elementCount > static_cast<GLint>(values.size()))
		{
			LOGERROR(
				"CShaderProgramARB::SetUniform(): Invalid uniform element count (expected: %zu passed: %zu)",
				m_BindingSlots[bindingSlot].elementCount, values.size());
			return;
		}
		const GLenum type = m_BindingSlots[bindingSlot].type;

		if (type == GL_FLOAT)
			SetUniform(m_BindingSlots[bindingSlot], values[0], 0.0f, 0.0f, 0.0f);
		else if (type == GL_FLOAT_VEC2)
			SetUniform(m_BindingSlots[bindingSlot], values[0], values[1], 0.0f, 0.0f);
		else if (type == GL_FLOAT_VEC3)
			SetUniform(m_BindingSlots[bindingSlot], values[0], values[1], values[2], 0.0f);
		else if (type == GL_FLOAT_VEC4)
			SetUniform(m_BindingSlots[bindingSlot], values[0], values[1], values[2], values[3]);
		else if (type == GL_FLOAT_MAT4)
			SetUniformMatrix(m_BindingSlots[bindingSlot], values);
		else
			LOGERROR("CShaderProgramARB::SetUniform(): Invalid uniform type (expected float, vec2, vec3, vec4, mat4)");
		ogl_WarnIfError();
	}

	std::vector<VfsPath> GetFileDependencies() const override
	{
		return m_FileDependencies;
	}

private:
	struct BindingSlot
	{
		CStrIntern name;
		int vertexProgramLocation;
		int fragmentProgramLocation;
		GLenum type;
		GLenum elementType;
		GLint elementCount;
	};

	BindingSlot& GetOrCreateBindingSlot(const CStrIntern name)
	{
		auto it = m_BindingSlotsMapping.find(name);
		if (it == m_BindingSlotsMapping.end())
		{
			m_BindingSlotsMapping[name] = m_BindingSlots.size();
			BindingSlot bindingSlot{};
			bindingSlot.name = name;
			bindingSlot.vertexProgramLocation = -1;
			bindingSlot.fragmentProgramLocation = -1;
			bindingSlot.elementType = 0;
			bindingSlot.elementCount = 0;
			m_BindingSlots.emplace_back(std::move(bindingSlot));
			return m_BindingSlots.back();
		}
		else
			return m_BindingSlots[it->second];
	}

	void SetUniform(
		const BindingSlot& bindingSlot,
		const float v0, const float v1, const float v2, const float v3)
	{
		SetUniform(GL_VERTEX_PROGRAM_ARB, bindingSlot.vertexProgramLocation, v0, v1, v2, v3);
		SetUniform(GL_FRAGMENT_PROGRAM_ARB, bindingSlot.fragmentProgramLocation, v0, v1, v2, v3);
	}

	void SetUniform(
		const GLenum target, const int location,
		const float v0, const float v1, const float v2, const float v3)
	{
		if (location >= 0)
		{
			glProgramLocalParameter4fARB(
				target, static_cast<GLuint>(location), v0, v1, v2, v3);
		}
	}

	void SetUniformMatrix(
		const BindingSlot& bindingSlot, PS::span<const float> values)
	{
		const size_t mat4ElementCount = 16;
		ENSURE(values.size() == mat4ElementCount);
		SetUniformMatrix(GL_VERTEX_PROGRAM_ARB, bindingSlot.vertexProgramLocation, values);
		SetUniformMatrix(GL_FRAGMENT_PROGRAM_ARB, bindingSlot.fragmentProgramLocation, values);
	}

	void SetUniformMatrix(
		const GLenum target, const int location, PS::span<const float> values)
	{
		if (location >= 0)
		{
			glProgramLocalParameter4fARB(
				target, static_cast<GLuint>(location + 0), values[0], values[4], values[8], values[12]);
			glProgramLocalParameter4fARB(
				target, static_cast<GLuint>(location + 1), values[1], values[5], values[9], values[13]);
			glProgramLocalParameter4fARB(
				target, static_cast<GLuint>(location + 2), values[2], values[6], values[10], values[14]);
			glProgramLocalParameter4fARB(
				target, static_cast<GLuint>(location + 3), values[3], values[7], values[11], values[15]);
		}
	}

	CDevice* m_Device = nullptr;

	std::vector<VfsPath> m_FileDependencies;

	GLuint m_VertexProgram;
	GLuint m_FragmentProgram;

	std::vector<BindingSlot> m_BindingSlots;
	std::unordered_map<CStrIntern, int32_t> m_BindingSlotsMapping;
};

#endif // !CONFIG2_GLES

class CShaderProgramGLSL final : public CShaderProgram
{
public:
	CShaderProgramGLSL(
		CDevice* device, const CStr& name,
		const VfsPath& path, const VfsPath& vertexFilePath, const VfsPath& fragmentFilePath,
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
		m_FileDependencies = {path, vertexFilePath, fragmentFilePath};

#if !CONFIG2_GLES
		if (m_Device->GetCapabilities().debugLabels)
		{
			glObjectLabel(GL_SHADER, m_VertexShader, -1, vertexFilePath.string8().c_str());
			glObjectLabel(GL_SHADER, m_FragmentShader, -1, fragmentFilePath.string8().c_str());
		}
#endif

		std::vector<VfsPath> newFileDependencies = {path, vertexFilePath, fragmentFilePath};

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

		Bind(nullptr);

		ogl_WarnIfError();

		// Reorder sampler units to decrease redundant texture unit changes when
		// samplers bound in a different order.
		const std::unordered_map<CStrIntern, int> requiredUnits =
		{
			{CStrIntern("baseTex"), 0},
			{CStrIntern("normTex"), 1},
			{CStrIntern("specTex"), 2},
			{CStrIntern("aoTex"), 3},
			{CStrIntern("shadowTex"), 4},
			{CStrIntern("losTex"), 5},
		};

		std::vector<uint8_t> occupiedUnits;

		GLint numUniforms = 0;
		glGetProgramiv(m_Program, GL_ACTIVE_UNIFORMS, &numUniforms);
		ogl_WarnIfError();
		for (GLint i = 0; i < numUniforms; ++i)
		{
			// TODO: use GL_ACTIVE_UNIFORM_MAX_LENGTH for the size.
			char name[256] = {0};
			GLsizei nameLength = 0;
			GLint size = 0;
			GLenum type = 0;
			glGetActiveUniform(m_Program, i, ARRAY_SIZE(name), &nameLength, &size, &type, name);
			ogl_WarnIfError();

			const GLint location = glGetUniformLocation(m_Program, name);

			// OpenGL specification is a bit vague about a name returned by glGetActiveUniform.
			// NVIDIA drivers return uniform name with "[0]", Intel Windows drivers without;
			while (nameLength > 3 &&
				name[nameLength - 3] == '[' &&
				name[nameLength - 2] == '0' &&
				name[nameLength - 1] == ']')
			{
				nameLength -= 3;
			}
			name[nameLength] = 0;

			const CStrIntern nameIntern(name);

			m_BindingSlotsMapping[nameIntern] = m_BindingSlots.size();
			BindingSlot bindingSlot{};
			bindingSlot.name = nameIntern;
			bindingSlot.location = location;
			bindingSlot.size = size;
			bindingSlot.type = type;
			bindingSlot.isTexture = false;

#define CASE(TYPE, ELEMENT_TYPE, ELEMENT_COUNT) \
			case GL_ ## TYPE: \
				bindingSlot.elementType = GL_ ## ELEMENT_TYPE; \
				bindingSlot.elementCount = ELEMENT_COUNT; \
				break;

			switch (type)
			{
			CASE(FLOAT, FLOAT, 1);
			CASE(FLOAT_VEC2, FLOAT, 2);
			CASE(FLOAT_VEC3, FLOAT, 3);
			CASE(FLOAT_VEC4, FLOAT, 4);
			CASE(INT, INT, 1);
			CASE(FLOAT_MAT2, FLOAT, 4);
			CASE(FLOAT_MAT3, FLOAT, 9);
			CASE(FLOAT_MAT4, FLOAT, 16);
#if !CONFIG2_GLES // GL ES 2.0 doesn't support non-square matrices.
			CASE(FLOAT_MAT2x3, FLOAT, 6);
			CASE(FLOAT_MAT2x4, FLOAT, 8);
			CASE(FLOAT_MAT3x2, FLOAT, 6);
			CASE(FLOAT_MAT3x4, FLOAT, 12);
			CASE(FLOAT_MAT4x2, FLOAT, 8);
			CASE(FLOAT_MAT4x3, FLOAT, 12);
#endif
			}
#undef CASE

			// Assign sampler uniforms to sequential texture units.
			if (type == GL_SAMPLER_2D
			 || type == GL_SAMPLER_CUBE
#if !CONFIG2_GLES
			 || type == GL_SAMPLER_2D_SHADOW
#endif
			)
			{
				const auto it = requiredUnits.find(nameIntern);
				const int unit = it == requiredUnits.end() ? -1 : it->second;
				bindingSlot.elementType = (type == GL_SAMPLER_CUBE ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D);
				bindingSlot.elementCount = unit;
				bindingSlot.isTexture = true;
				if (unit != -1)
				{
					if (unit >= static_cast<int>(occupiedUnits.size()))
						occupiedUnits.resize(unit + 1);
					occupiedUnits[unit] = true;
				}
			}

			if (bindingSlot.elementType == 0)
			{
				LOGERROR("CShaderProgramGLSL::Link: unsupported uniform type: 0x%04x", static_cast<int>(type));
			}

			m_BindingSlots.emplace_back(std::move(bindingSlot));
		}

		for (BindingSlot& bindingSlot : m_BindingSlots)
		{
			if (!bindingSlot.isTexture)
				continue;
			if (bindingSlot.elementCount == -1)
			{
				// We need to find a minimal available unit.
				int unit = 0;
				while (unit < static_cast<int>(occupiedUnits.size()) && occupiedUnits[unit])
					++unit;
				if (unit >= static_cast<int>(occupiedUnits.size()))
					occupiedUnits.resize(unit + 1);
				occupiedUnits[unit] = true;
				bindingSlot.elementCount = unit;
			}
			// Link uniform to unit.
			glUniform1i(bindingSlot.location, bindingSlot.elementCount);
			ogl_WarnIfError();
		}

		// TODO: verify that we're not using more samplers than is supported

		Unbind();

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

		m_ValidStreams = 0;
	}

	void Unbind() override
	{
		glUseProgram(0);

		for (const int index : m_ActiveVertexAttributes)
			glDisableVertexAttribArray(index);
	}

	IDevice* GetDevice() override { return m_Device; }

	int32_t GetBindingSlot(const CStrIntern name) const override
	{
		auto it = m_BindingSlotsMapping.find(name);
		return it == m_BindingSlotsMapping.end() ? -1 : it->second;
	}

	TextureUnit GetTextureUnit(const int32_t bindingSlot) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return { 0, 0, 0 };
		TextureUnit textureUnit;
		textureUnit.type = m_BindingSlots[bindingSlot].type;
		textureUnit.target = m_BindingSlots[bindingSlot].elementType;
		textureUnit.unit = m_BindingSlots[bindingSlot].elementCount;
		return textureUnit;
	}

	void SetUniform(
		const int32_t bindingSlot,
		const float value) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return;
		if (m_BindingSlots[bindingSlot].type != GL_FLOAT ||
			m_BindingSlots[bindingSlot].size != 1)
		{
			LOGERROR("CShaderProgramGLSL::SetUniform(): Invalid uniform type (expected float) '%s'", m_BindingSlots[bindingSlot].name.c_str());
			return;
		}
		glUniform1f(m_BindingSlots[bindingSlot].location, value);
		ogl_WarnIfError();
	}

	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return;
		if (m_BindingSlots[bindingSlot].type != GL_FLOAT_VEC2 ||
			m_BindingSlots[bindingSlot].size != 1)
		{
			LOGERROR("CShaderProgramGLSL::SetUniform(): Invalid uniform type (expected vec2) '%s'", m_BindingSlots[bindingSlot].name.c_str());
			return;
		}
		glUniform2f(m_BindingSlots[bindingSlot].location, valueX, valueY);
		ogl_WarnIfError();
	}

	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY, const float valueZ) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return;
		if (m_BindingSlots[bindingSlot].type != GL_FLOAT_VEC3 ||
			m_BindingSlots[bindingSlot].size != 1)
		{
			LOGERROR("CShaderProgramGLSL::SetUniform(): Invalid uniform type (expected vec3) '%s'", m_BindingSlots[bindingSlot].name.c_str());
			return;
		}
		glUniform3f(m_BindingSlots[bindingSlot].location, valueX, valueY, valueZ);
		ogl_WarnIfError();
	}

	void SetUniform(
		const int32_t bindingSlot,
		const float valueX, const float valueY,
		const float valueZ, const float valueW) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return;
		if (m_BindingSlots[bindingSlot].type != GL_FLOAT_VEC4 ||
			m_BindingSlots[bindingSlot].size != 1)
		{
			LOGERROR("CShaderProgramGLSL::SetUniform(): Invalid uniform type (expected vec4) '%s'", m_BindingSlots[bindingSlot].name.c_str());
			return;
		}
		glUniform4f(m_BindingSlots[bindingSlot].location, valueX, valueY, valueZ, valueW);
		ogl_WarnIfError();
	}

	void SetUniform(
		const int32_t bindingSlot, PS::span<const float> values) override
	{
		if (bindingSlot < 0 || bindingSlot >= static_cast<int32_t>(m_BindingSlots.size()))
			return;
		if (m_BindingSlots[bindingSlot].elementType != GL_FLOAT)
		{
			LOGERROR("CShaderProgramGLSL::SetUniform(): Invalid uniform element type (expected float) '%s'", m_BindingSlots[bindingSlot].name.c_str());
			return;
		}
		if (m_BindingSlots[bindingSlot].size == 1 && m_BindingSlots[bindingSlot].elementCount > static_cast<GLint>(values.size()))
		{
			LOGERROR(
				"CShaderProgramGLSL::SetUniform(): Invalid uniform element count (expected: %zu passed: %zu) '%s'",
				m_BindingSlots[bindingSlot].elementCount, values.size(), m_BindingSlots[bindingSlot].name.c_str());
			return;
		}
		const GLint location = m_BindingSlots[bindingSlot].location;
		const GLenum type = m_BindingSlots[bindingSlot].type;

		if (type == GL_FLOAT)
			glUniform1fv(location, 1, values.data());
		else if (type == GL_FLOAT_VEC2)
			glUniform2fv(location, 1, values.data());
		else if (type == GL_FLOAT_VEC3)
			glUniform3fv(location, 1, values.data());
		else if (type == GL_FLOAT_VEC4)
			glUniform4fv(location, 1, values.data());
		else if (type == GL_FLOAT_MAT4)
		{
			// For case of array of matrices we might pass less number of matrices.
			const GLint size = std::min(
				m_BindingSlots[bindingSlot].size, static_cast<GLint>(values.size() / 16));
			glUniformMatrix4fv(location, size, GL_FALSE, values.data());
		}
		else
			LOGERROR("CShaderProgramGLSL::SetUniform(): Invalid uniform type (expected float, vec2, vec3, vec4, mat4) '%s'", m_BindingSlots[bindingSlot].name.c_str());
		ogl_WarnIfError();
	}

	void VertexAttribPointer(
		const VertexAttributeStream stream, const Format format,
		const uint32_t offset, const uint32_t stride,
		const VertexAttributeRate rate, const void* data) override
	{
		const int attributeLocation = GetAttributeLocationFromStream(m_Device, stream);
		std::vector<int>::const_iterator it =
			std::lower_bound(m_ActiveVertexAttributes.begin(), m_ActiveVertexAttributes.end(), attributeLocation);
		if (it == m_ActiveVertexAttributes.end() || *it != attributeLocation)
			return;
		const GLint size = GLSizeFromFormat(format);
		const GLenum type = GLTypeFromFormat(format);
		const GLboolean normalized = NormalizedFromFormat(format);
		glVertexAttribPointer(
			attributeLocation, size, type, normalized, stride, static_cast<const u8*>(data) + offset);
		if (rate == VertexAttributeRate::PER_INSTANCE)
			ENSURE(m_Device->GetCapabilities().instancing);
		if (m_Device->GetCapabilities().instancing)
		{
			glVertexAttribDivisorARB(attributeLocation, rate == VertexAttributeRate::PER_INSTANCE ? 1 : 0);
		}
		m_ValidStreams |= GetStreamMask(stream);
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

	struct BindingSlot
	{
		CStrIntern name;
		GLint location;
		GLint size;
		GLenum type;
		GLenum elementType;
		GLint elementCount;
		bool isTexture;
	};
	std::vector<BindingSlot> m_BindingSlots;
	std::unordered_map<CStrIntern, int32_t> m_BindingSlotsMapping;
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

	const bool isGLSL = root.GetAttributes().GetNamedItem(at_type) == "glsl";

	VfsPath vertexFile;
	VfsPath fragmentFile;
	CShaderDefines defines = baseDefines;
	std::map<CStrIntern, std::pair<CStr, int>> vertexUniforms;
	std::map<CStrIntern, std::pair<CStr, int>> fragmentUniforms;
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
					vertexUniforms[CStrIntern(attributes.GetNamedItem(at_name))] =
						std::make_pair(attributes.GetNamedItem(at_type), attributes.GetNamedItem(at_loc).ToInt());
				}
				else if (param.GetNodeName() == el_stream)
				{
					const CStr streamName = attributes.GetNamedItem(at_name);
					const CStr attributeName = attributes.GetNamedItem(at_attribute);
					if (attributeName.empty() && isGLSL)
						LOGERROR("Empty attribute name in vertex shader description '%s'", vertexFile.string8().c_str());

					VertexAttributeStream stream =
						VertexAttributeStream::UV7;
					if (streamName == "pos")
						stream = VertexAttributeStream::POSITION;
					else if (streamName == "normal")
						stream = VertexAttributeStream::NORMAL;
					else if (streamName == "color")
						stream = VertexAttributeStream::COLOR;
					else if (streamName == "uv0")
						stream = VertexAttributeStream::UV0;
					else if (streamName == "uv1")
						stream = VertexAttributeStream::UV1;
					else if (streamName == "uv2")
						stream = VertexAttributeStream::UV2;
					else if (streamName == "uv3")
						stream = VertexAttributeStream::UV3;
					else if (streamName == "uv4")
						stream = VertexAttributeStream::UV4;
					else if (streamName == "uv5")
						stream = VertexAttributeStream::UV5;
					else if (streamName == "uv6")
						stream = VertexAttributeStream::UV6;
					else if (streamName == "uv7")
						stream = VertexAttributeStream::UV7;
					else
						LOGERROR("Unknown stream '%s' in vertex shader description '%s'", streamName.c_str(), vertexFile.string8().c_str());

					if (isGLSL)
					{
						const int attributeLocation = GetAttributeLocationFromStream(device, stream);
						vertexAttribs[CStrIntern(attributeName)] = attributeLocation;
					}
					streamFlags |= GetStreamMask(stream);
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
					fragmentUniforms[CStrIntern(attributes.GetNamedItem(at_name))] =
						std::make_pair(attributes.GetNamedItem(at_type), attributes.GetNamedItem(at_loc).ToInt());
				}
			}
		}
	}

	if (isGLSL)
	{
		return std::make_unique<CShaderProgramGLSL>(
			device, name, xmlFilename, vertexFile, fragmentFile, defines,
			vertexAttribs, streamFlags);
	}
	else
	{
#if CONFIG2_GLES
		LOGERROR("CShaderProgram::Create: '%s'+'%s': ARB shaders not supported on this device",
			vertexFile.string8(), fragmentFile.string8());
		return nullptr;
#else
		return std::make_unique<CShaderProgramARB>(
			device, xmlFilename, vertexFile, fragmentFile, defines,
			vertexUniforms, fragmentUniforms, streamFlags);
#endif
	}
}

// These should all be overridden by CShaderProgramGLSL, and not used
// if a non-GLSL shader was loaded instead:

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
	m_ValidStreams |= GetStreamMask(VertexAttributeStream::POSITION);
}

void CShaderProgram::NormalPointer(const Renderer::Backend::Format format, GLsizei stride, const void* pointer)
{
	ENSURE(format == Renderer::Backend::Format::R32G32B32_SFLOAT);
	glNormalPointer(GL_FLOAT, stride, pointer);
	m_ValidStreams |= GetStreamMask(VertexAttributeStream::NORMAL);
}

void CShaderProgram::ColorPointer(const Renderer::Backend::Format format, GLsizei stride, const void* pointer)
{
	const GLint size = GLSizeFromFormat(format);
	ENSURE(3 <= size && size <= 4);
	const GLenum type = GLTypeFromFormat(format);
	glColorPointer(size, type, stride, pointer);
	m_ValidStreams |= GetStreamMask(VertexAttributeStream::COLOR);
}

void CShaderProgram::TexCoordPointer(GLenum texture, const Renderer::Backend::Format format, GLsizei stride, const void* pointer)
{
	glClientActiveTextureARB(texture);
	const GLint size = GLSizeFromFormat(format);
	ENSURE(1 <= size && size <= 4);
	const GLenum type = GLTypeFromFormat(format);
	glTexCoordPointer(size, type, stride, pointer);
	glClientActiveTextureARB(GL_TEXTURE0);
	m_ValidStreams |= GetStreamMask(VertexAttributeStream::UV0) << (texture - GL_TEXTURE0);
}

void CShaderProgram::BindClientStates()
{
	ENSURE(m_StreamFlags == (m_StreamFlags & (
		GetStreamMask(VertexAttributeStream::POSITION) |
		GetStreamMask(VertexAttributeStream::NORMAL) |
		GetStreamMask(VertexAttributeStream::COLOR) |
		GetStreamMask(VertexAttributeStream::UV0) |
		GetStreamMask(VertexAttributeStream::UV1))));

	// Enable all the desired client states for non-GLSL rendering

	if (m_StreamFlags & GetStreamMask(VertexAttributeStream::POSITION))
		glEnableClientState(GL_VERTEX_ARRAY);
	if (m_StreamFlags & GetStreamMask(VertexAttributeStream::NORMAL))
		glEnableClientState(GL_NORMAL_ARRAY);
	if (m_StreamFlags & GetStreamMask(VertexAttributeStream::COLOR))
		glEnableClientState(GL_COLOR_ARRAY);

	if (m_StreamFlags & GetStreamMask(VertexAttributeStream::UV0))
	{
		glClientActiveTextureARB(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if (m_StreamFlags & GetStreamMask(VertexAttributeStream::UV1))
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
	if (m_StreamFlags & GetStreamMask(VertexAttributeStream::POSITION))
		glDisableClientState(GL_VERTEX_ARRAY);
	if (m_StreamFlags & GetStreamMask(VertexAttributeStream::NORMAL))
		glDisableClientState(GL_NORMAL_ARRAY);
	if (m_StreamFlags & GetStreamMask(VertexAttributeStream::COLOR))
		glDisableClientState(GL_COLOR_ARRAY);

	if (m_StreamFlags & GetStreamMask(VertexAttributeStream::UV0))
	{
		glClientActiveTextureARB(GL_TEXTURE0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if (m_StreamFlags & GetStreamMask(VertexAttributeStream::UV1))
	{
		glClientActiveTextureARB(GL_TEXTURE1);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glClientActiveTextureARB(GL_TEXTURE0);
	}
}

#endif // !CONFIG2_GLES

bool CShaderProgram::IsStreamActive(const VertexAttributeStream stream) const
{
	return (m_StreamFlags & GetStreamMask(stream)) != 0;
}

void CShaderProgram::VertexAttribPointer(
	const VertexAttributeStream stream, const Format format,
	const uint32_t offset, const uint32_t stride,
	const VertexAttributeRate rate, const void* data)
{
	ENSURE(rate == VertexAttributeRate::PER_VERTEX);
	switch (stream)
	{
	case VertexAttributeStream::POSITION:
		VertexPointer(format, stride, static_cast<const u8*>(data) + offset);
		break;
	case VertexAttributeStream::NORMAL:
		NormalPointer(format, stride, static_cast<const u8*>(data) + offset);
		break;
	case VertexAttributeStream::COLOR:
		ColorPointer(format, stride, static_cast<const u8*>(data) + offset);
		break;
	case VertexAttributeStream::UV0: FALLTHROUGH;
	case VertexAttributeStream::UV1: FALLTHROUGH;
	case VertexAttributeStream::UV2: FALLTHROUGH;
	case VertexAttributeStream::UV3: FALLTHROUGH;
	case VertexAttributeStream::UV4: FALLTHROUGH;
	case VertexAttributeStream::UV5: FALLTHROUGH;
	case VertexAttributeStream::UV6: FALLTHROUGH;
	case VertexAttributeStream::UV7:
	{
		const int indexOffset = static_cast<int>(stream) - static_cast<int>(VertexAttributeStream::UV0);
		TexCoordPointer(GL_TEXTURE0 + indexOffset, format, stride, static_cast<const u8*>(data) + offset);
		break;
	}
	default:
		debug_warn("Unsupported stream");
	};
}

void CShaderProgram::AssertPointersBound()
{
	ENSURE((m_StreamFlags & ~m_ValidStreams) == 0);
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
