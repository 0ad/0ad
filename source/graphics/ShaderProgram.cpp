/* Copyright (C) 2012 Wildfire Games.
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

#include "graphics/TextureManager.h"
#include "lib/res/graphics/ogl_tex.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Overlay.h"
#include "ps/Preprocessor.h"

#if !CONFIG2_GLES

class CShaderProgramARB : public CShaderProgram
{
public:
	CShaderProgramARB(const VfsPath& vertexFile, const VfsPath& fragmentFile,
		const std::map<CStr, CStr>& defines,
		const std::map<CStr, int>& vertexIndexes, const std::map<CStr, int>& fragmentIndexes,
		int streamflags) :
		CShaderProgram(streamflags),
		m_VertexFile(vertexFile), m_FragmentFile(fragmentFile),
		m_Defines(defines),
		m_VertexIndexes(vertexIndexes), m_FragmentIndexes(fragmentIndexes)
	{
		pglGenProgramsARB(1, &m_VertexProgram);
		pglGenProgramsARB(1, &m_FragmentProgram);
	}

	~CShaderProgramARB()
	{
		Unload();

		pglDeleteProgramsARB(1, &m_VertexProgram);
		pglDeleteProgramsARB(1, &m_FragmentProgram);
	}

	bool Compile(GLuint target, const char* targetName, GLuint program, const VfsPath& file, const CStr& code)
	{
		ogl_WarnIfError();

		pglBindProgramARB(target, program);

		ogl_WarnIfError();

		pglProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)code.length(), code.c_str());

		if (ogl_SquelchError(GL_INVALID_OPERATION))
		{
			GLint errPos = 0;
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
			int errLine = std::count(code.begin(), code.begin() + errPos + 1, '\n') + 1;
			char* errStr = (char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
			LOGERROR(L"Failed to compile %hs program '%ls' (line %d):\n%hs", targetName, file.string().c_str(), errLine, errStr);
			return false;
		}

		pglBindProgramARB(target, 0);

		ogl_WarnIfError();

		return true;
	}

	virtual void Reload()
	{
		Unload();

		CVFSFile vertexFile;
		if (vertexFile.Load(g_VFS, m_VertexFile) != PSRETURN_OK)
			return;

		CVFSFile fragmentFile;
		if (fragmentFile.Load(g_VFS, m_FragmentFile) != PSRETURN_OK)
			return;

		CPreprocessor preprocessor;
		for (std::map<CStr, CStr>::iterator it = m_Defines.begin(); it != m_Defines.end(); ++it)
			preprocessor.Define(it->first.c_str(), it->second.c_str());

		CStr vertexCode = Preprocess(preprocessor, vertexFile.GetAsString());
		CStr fragmentCode = Preprocess(preprocessor, fragmentFile.GetAsString());

//		printf(">>>\n%s<<<\n", vertexCode.c_str());
//		printf(">>>\n%s<<<\n", fragmentCode.c_str());

		if (!Compile(GL_VERTEX_PROGRAM_ARB, "vertex", m_VertexProgram, m_VertexFile, vertexCode))
			return;

		if (!Compile(GL_FRAGMENT_PROGRAM_ARB, "fragment", m_FragmentProgram, m_FragmentFile, fragmentCode))
			return;

		m_IsValid = true;
	}

	void Unload()
	{
		m_IsValid = false;
	}

	virtual void Bind()
	{
		glEnable(GL_VERTEX_PROGRAM_ARB);
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		pglBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_VertexProgram);
		pglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_FragmentProgram);

		BindClientStates();
	}

	virtual void Unbind()
	{
		glDisable(GL_VERTEX_PROGRAM_ARB);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		pglBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
		pglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);

		UnbindClientStates();

		// TODO: should unbind textures, probably
	}

	int GetUniformVertexIndex(uniform_id_t id)
	{
		std::map<CStr, int>::iterator it = m_VertexIndexes.find(id);
		if (it == m_VertexIndexes.end())
			return -1;
		return it->second;
	}

	int GetUniformFragmentIndex(uniform_id_t id)
	{
		std::map<CStr, int>::iterator it = m_FragmentIndexes.find(id);
		if (it == m_FragmentIndexes.end())
			return -1;
		return it->second;
	}

	virtual bool HasTexture(texture_id_t id)
	{
		if (GetUniformFragmentIndex(id) != -1)
			return true;
		return false;
	}

	virtual void BindTexture(texture_id_t id, Handle tex)
	{
		int index = GetUniformFragmentIndex(id);
		if (index != -1)
			ogl_tex_bind(tex, index);
	}

	virtual void BindTexture(texture_id_t id, GLuint tex)
	{
		int index = GetUniformFragmentIndex(id);
		if (index != -1)
		{
			pglActiveTextureARB((int)(GL_TEXTURE0+index));
			glBindTexture(GL_TEXTURE_2D, tex);
		}
	}

	virtual int GetTextureUnit(texture_id_t id)
	{
		return GetUniformFragmentIndex(id);
	}

	virtual Binding GetUniformBinding(uniform_id_t id)
	{
		return Binding(GetUniformVertexIndex(id), GetUniformFragmentIndex(id));
	}

	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3)
	{
		if (id.first != -1)
			pglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.first, v0, v1, v2, v3);

		if (id.second != -1)
			pglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.second, v0, v1, v2, v3);
	}

	virtual void Uniform(Binding id, const CMatrix3D& v)
	{
		if (id.first != -1)
		{
			pglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.first+0, v._11, v._12, v._13, v._14);
			pglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.first+1, v._21, v._22, v._23, v._24);
			pglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.first+2, v._31, v._32, v._33, v._34);
			pglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.first+3, v._41, v._42, v._43, v._44);
		}

		if (id.second != -1)
		{
			pglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.second+0, v._11, v._12, v._13, v._14);
			pglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.second+1, v._21, v._22, v._23, v._24);
			pglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.second+2, v._31, v._32, v._33, v._34);
			pglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.second+3, v._41, v._42, v._43, v._44);
		}
	}

private:
	VfsPath m_VertexFile;
	VfsPath m_FragmentFile;
	std::map<CStr, CStr> m_Defines;

	GLuint m_VertexProgram;
	GLuint m_FragmentProgram;

	std::map<CStr, int> m_VertexIndexes;
	std::map<CStr, int> m_FragmentIndexes;
};

#endif // #if !CONFIG2_GLES

class CShaderProgramGLSL : public CShaderProgram
{
public:
	CShaderProgramGLSL(const VfsPath& vertexFile, const VfsPath& fragmentFile,
		const std::map<CStr, CStr>& defines,
		const std::map<CStr, int>& vertexAttribs,
		int streamflags) :
	CShaderProgram(streamflags),
		m_VertexFile(vertexFile), m_FragmentFile(fragmentFile),
		m_Defines(defines),
		m_VertexAttribs(vertexAttribs)
	{
		m_Program = 0;
		m_VertexShader = pglCreateShaderObjectARB(GL_VERTEX_SHADER);
		m_FragmentShader = pglCreateShaderObjectARB(GL_FRAGMENT_SHADER);
	}

	~CShaderProgramGLSL()
	{
		Unload();

		pglDeleteShader(m_VertexShader);
		pglDeleteShader(m_FragmentShader);
	}

	bool Compile(GLhandleARB shader, const VfsPath& file, const CStr& code)
	{
		ogl_WarnIfError();

		const char* code_string = code.c_str();
		GLint code_length = code.length();
		pglShaderSourceARB(shader, 1, &code_string, &code_length);

		pglCompileShaderARB(shader);

		GLint ok = 0;
		pglGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
		if (!ok)
		{
			GLint length = 0;
			pglGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

			// Apparently sometimes GL_INFO_LOG_LENGTH is incorrectly reported as 0
			// (http://code.google.com/p/android/issues/detail?id=9953)
			if (length == 0)
				length = 4096;

			char* infolog = new char[length];
			pglGetShaderInfoLog(shader, length, NULL, infolog);
			LOGERROR(L"Failed to compile shader '%ls':\n%hs", file.string().c_str(), infolog);
			delete[] infolog;

			return false;
		}

		ogl_WarnIfError();

		return true;
	}

	bool Link()
	{
		ENSURE(!m_Program);
		m_Program = pglCreateProgramObjectARB();

		pglAttachObjectARB(m_Program, m_VertexShader);
		ogl_WarnIfError();
		pglAttachObjectARB(m_Program, m_FragmentShader);
		ogl_WarnIfError();

		// Set up the attribute bindings explicitly, since apparently drivers
		// don't always pick the most efficient bindings automatically,
		// and also this lets us hardcode indexes into VertexPointer etc
		for (std::map<CStr, int>::iterator it = m_VertexAttribs.begin(); it != m_VertexAttribs.end(); ++it)
			pglBindAttribLocationARB(m_Program, it->second, it->first.c_str());

		pglLinkProgramARB(m_Program);

		GLint ok = 0;
		pglGetProgramiv(m_Program, GL_LINK_STATUS, &ok);
		if (!ok)
		{
			GLint length = 0;
			pglGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &length);

			if (length == 0)
				length = 4096;

			char* infolog = new char[length];
			pglGetProgramInfoLog(m_Program, length, NULL, infolog);
			LOGERROR(L"Failed to link program '%ls'+'%ls':\n%hs", m_VertexFile.string().c_str(), m_FragmentFile.string().c_str(), infolog);
			delete[] infolog;

			return false;
		}

		ogl_WarnIfError();

		m_UniformLocations.clear();
		m_UniformTypes.clear();
		m_Samplers.clear();

		Bind();

		ogl_WarnIfError();

		GLint numUniforms = 0;
		pglGetProgramiv(m_Program, GL_ACTIVE_UNIFORMS, &numUniforms);
		ogl_WarnIfError();
		for (GLint i = 0; i < numUniforms; ++i)
		{
			char name[256] = {0};
			GLsizei nameLength = 0;
			GLint size = 0;
			GLenum type = 0;
			pglGetActiveUniformARB(m_Program, i, ARRAY_SIZE(name), &nameLength, &size, &type, name);
			ogl_WarnIfError();

			GLint loc = pglGetUniformLocationARB(m_Program, name);

			m_UniformLocations[name] = loc;
			m_UniformTypes[name] = type;

			// Assign sampler uniforms to sequential texture units
			if (type == GL_SAMPLER_2D
			 || type == GL_SAMPLER_CUBE
#if !CONFIG2_GLES
			 || type == GL_SAMPLER_2D_SHADOW
#endif
			)
			{
				int unit = (int)m_Samplers.size();
				m_Samplers[name].first = (type == GL_SAMPLER_CUBE ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D);
				m_Samplers[name].second = unit;
				pglUniform1iARB(loc, unit); // link uniform to unit
				ogl_WarnIfError();
			}
		}

		// TODO: verify that we're not using more samplers than is supported

		Unbind();

		ogl_WarnIfError();

		return true;
	}

	virtual void Reload()
	{
		Unload();

		CVFSFile vertexFile;
		if (vertexFile.Load(g_VFS, m_VertexFile) != PSRETURN_OK)
			return;

		CVFSFile fragmentFile;
		if (fragmentFile.Load(g_VFS, m_FragmentFile) != PSRETURN_OK)
			return;

		CPreprocessor preprocessor;
		for (std::map<CStr, CStr>::iterator it = m_Defines.begin(); it != m_Defines.end(); ++it)
			preprocessor.Define(it->first.c_str(), it->second.c_str());

		CStr vertexCode = Preprocess(preprocessor, vertexFile.GetAsString());
		CStr fragmentCode = Preprocess(preprocessor, fragmentFile.GetAsString());

		if (!Compile(m_VertexShader, m_VertexFile, vertexCode))
			return;

		if (!Compile(m_FragmentShader, m_FragmentFile, fragmentCode))
			return;

		if (!Link())
			return;

		m_IsValid = true;
	}

	void Unload()
	{
		m_IsValid = false;

		if (m_Program)
			pglDeleteProgram(m_Program);
		m_Program = 0;

		// The shader objects can be reused and don't need to be deleted here
	}

	virtual void Bind()
	{
		pglUseProgramObjectARB(m_Program);

		for (std::map<CStr, int>::iterator it = m_VertexAttribs.begin(); it != m_VertexAttribs.end(); ++it)
			pglEnableVertexAttribArrayARB(it->second);
	}

	virtual void Unbind()
	{
		pglUseProgramObjectARB(0);

		for (std::map<CStr, int>::iterator it = m_VertexAttribs.begin(); it != m_VertexAttribs.end(); ++it)
			pglDisableVertexAttribArrayARB(it->second);

		// TODO: should unbind textures, probably
	}

	int GetUniformLocation(uniform_id_t id)
	{
		std::map<CStr, int>::iterator it = m_UniformLocations.find(id);
		if (it == m_UniformLocations.end())
			return -1;
		return it->second;
	}

	virtual bool HasTexture(texture_id_t id)
	{
		if (GetUniformLocation(id) != -1)
			return true;
		return false;
	}

	virtual void BindTexture(texture_id_t id, Handle tex)
	{
		std::map<CStr, std::pair<GLenum, int> >::iterator it = m_Samplers.find(id);
		if (it == m_Samplers.end())
			return;

		GLuint h;
		ogl_tex_get_texture_id(tex, &h);
		pglActiveTextureARB(GL_TEXTURE0 + it->second.second);
		glBindTexture(it->second.first, h);
	}

	virtual void BindTexture(texture_id_t id, GLuint tex)
	{
		std::map<CStr, std::pair<GLenum, int> >::iterator it = m_Samplers.find(id);
		if (it == m_Samplers.end())
			return;

		pglActiveTextureARB(GL_TEXTURE0 + it->second.second);
		glBindTexture(it->second.first, tex);
	}

	virtual int GetTextureUnit(texture_id_t id)
	{
		std::map<CStr, std::pair<GLenum, int> >::iterator it = m_Samplers.find(id);
		if (it == m_Samplers.end())
			return -1;

		return it->second.second;
	}

	virtual Binding GetUniformBinding(uniform_id_t id)
	{
		int loc = GetUniformLocation(id);
		if (loc == -1)
			return Binding();
		else
			return Binding(loc, m_UniformTypes[id]);
	}

	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3)
	{
		if (id.first != -1)
		{
			if (id.second == GL_FLOAT)
				pglUniform1fARB(id.first, v0);
			else if (id.second == GL_FLOAT_VEC2)
				pglUniform2fARB(id.first, v0, v1);
			else if (id.second == GL_FLOAT_VEC3)
				pglUniform3fARB(id.first, v0, v1, v2);
			else if (id.second == GL_FLOAT_VEC4)
				pglUniform4fARB(id.first, v0, v1, v2, v3);
			else
				LOGERROR(L"CShaderProgramGLSL::Uniform(): Invalid uniform type (expected float, vec2, vec3, vec4)");
		}
	}

	virtual void Uniform(Binding id, const CMatrix3D& v)
	{
		if (id.first != -1)
		{
			if (id.second == GL_FLOAT_MAT4)
				pglUniformMatrix4fvARB(id.first, 1, GL_FALSE, &v._11);
			else
				LOGERROR(L"CShaderProgramGLSL::Uniform(): Invalid uniform type (expected mat4)");
		}
	}

	// Map the various fixed-function Pointer functions onto generic vertex attributes
	// (matching the attribute indexes from ShaderManager's ParseAttribSemantics):

	virtual void VertexPointer(GLint size, GLenum type, GLsizei stride, void* pointer)
	{
		pglVertexAttribPointerARB(0, size, type, GL_FALSE, stride, pointer);
		m_ValidStreams |= STREAM_POS;
	}

	virtual void NormalPointer(GLenum type, GLsizei stride, void* pointer)
	{
		pglVertexAttribPointerARB(2, 3, type, GL_TRUE, stride, pointer);
		m_ValidStreams |= STREAM_NORMAL;
	}

	virtual void ColorPointer(GLint size, GLenum type, GLsizei stride, void* pointer)
	{
		pglVertexAttribPointerARB(3, size, type, GL_TRUE, stride, pointer);
		m_ValidStreams |= STREAM_COLOR;
	}

	virtual void TexCoordPointer(GLenum texture, GLint size, GLenum type, GLsizei stride, void* pointer)
	{
		pglVertexAttribPointerARB(8 + texture - GL_TEXTURE0, size, type, GL_FALSE, stride, pointer);
		m_ValidStreams |= STREAM_UV0 << (texture - GL_TEXTURE0);
	}

private:
	VfsPath m_VertexFile;
	VfsPath m_FragmentFile;
	std::map<CStr, CStr> m_Defines;
	std::map<CStr, int> m_VertexAttribs;

	GLhandleARB m_Program;
	GLhandleARB m_VertexShader;
	GLhandleARB m_FragmentShader;

	std::map<CStr, GLenum> m_UniformTypes;
	std::map<CStr, int> m_UniformLocations;
	std::map<CStr, std::pair<GLenum, int> > m_Samplers; // texture target & unit chosen for each uniform sampler
};



CShaderProgram::CShaderProgram(int streamflags)
	: m_IsValid(false), m_StreamFlags(streamflags), m_ValidStreams(0)
{
}

#if CONFIG2_GLES
/*static*/ CShaderProgram* CShaderProgram::ConstructARB(const VfsPath& vertexFile, const VfsPath& fragmentFile,
	const std::map<CStr, CStr>& UNUSED(defines),
	const std::map<CStr, int>& UNUSED(vertexIndexes), const std::map<CStr, int>& UNUSED(fragmentIndexes),
	int UNUSED(streamflags))
{
	LOGERROR(L"CShaderProgram::ConstructARB: '%ls'+'%ls': ARB shaders not supported on this device",
		vertexFile.string().c_str(), fragmentFile.string().c_str());
	return NULL;
}
#else
/*static*/ CShaderProgram* CShaderProgram::ConstructARB(const VfsPath& vertexFile, const VfsPath& fragmentFile,
	const std::map<CStr, CStr>& defines,
	const std::map<CStr, int>& vertexIndexes, const std::map<CStr, int>& fragmentIndexes,
	int streamflags)
{
	return new CShaderProgramARB(vertexFile, fragmentFile, defines, vertexIndexes, fragmentIndexes, streamflags);
}
#endif

/*static*/ CShaderProgram* CShaderProgram::ConstructGLSL(const VfsPath& vertexFile, const VfsPath& fragmentFile,
	const std::map<CStr, CStr>& defines,
	const std::map<CStr, int>& vertexAttribs,
	int streamflags)
{
	return new CShaderProgramGLSL(vertexFile, fragmentFile, defines, vertexAttribs, streamflags);
}

bool CShaderProgram::IsValid() const
{
	return m_IsValid;
}

int CShaderProgram::GetStreamFlags() const
{
	return m_StreamFlags;
}

void CShaderProgram::BindTexture(texture_id_t id, CTexturePtr tex)
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

CStr CShaderProgram::Preprocess(CPreprocessor& preprocessor, const CStr& input)
{
	size_t len = 0;
	char* output = preprocessor.Parse(input.c_str(), input.size(), len);

	if (!output)
	{
		LOGERROR(L"Shader preprocessing failed");
		return "";
	}

	CStr ret(output, len);

	// Free output if it's not inside the source string
	if (!(output >= input.c_str() && output < input.c_str() + input.size()))
		free(output);

	return ret;
}

#if CONFIG2_GLES

// These should all be overridden by CShaderProgramGLSL
void CShaderProgram::VertexPointer(GLint UNUSED(size), GLenum UNUSED(type), GLsizei UNUSED(stride), void* UNUSED(pointer))
{
	debug_warn("CShaderProgram::VertexPointer should be overridden");
}
void CShaderProgram::NormalPointer(GLenum UNUSED(type), GLsizei UNUSED(stride), void* UNUSED(pointer))
{
	debug_warn("CShaderProgram::NormalPointer should be overridden");
}
void CShaderProgram::ColorPointer(GLint UNUSED(size), GLenum UNUSED(type), GLsizei UNUSED(stride), void* UNUSED(pointer))
{
	debug_warn("CShaderProgram::ColorPointer should be overridden");
}
void CShaderProgram::TexCoordPointer(GLenum UNUSED(texture), GLint UNUSED(size), GLenum UNUSED(type), GLsizei UNUSED(stride), void* UNUSED(pointer))
{
	debug_warn("CShaderProgram::TexCoordPointer should be overridden");
}

#else

// These are overridden by CShaderProgramGLSL, but fixed-function and ARB shaders
// both use the fixed-function vertex attribute pointers:

void CShaderProgram::VertexPointer(GLint size, GLenum type, GLsizei stride, void* pointer)
{
	glVertexPointer(size, type, stride, pointer);
	m_ValidStreams |= STREAM_POS;
}

void CShaderProgram::NormalPointer(GLenum type, GLsizei stride, void* pointer)
{
	glNormalPointer(type, stride, pointer);
	m_ValidStreams |= STREAM_NORMAL;
}

void CShaderProgram::ColorPointer(GLint size, GLenum type, GLsizei stride, void* pointer)
{
	glColorPointer(size, type, stride, pointer);
	m_ValidStreams |= STREAM_COLOR;
}

void CShaderProgram::TexCoordPointer(GLenum texture, GLint size, GLenum type, GLsizei stride, void* pointer)
{
	pglClientActiveTextureARB(texture);
	glTexCoordPointer(size, type, stride, pointer);
	pglClientActiveTextureARB(GL_TEXTURE0);
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
		pglClientActiveTextureARB(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if (m_StreamFlags & STREAM_UV1)
	{
		pglClientActiveTextureARB(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		pglClientActiveTextureARB(GL_TEXTURE0);
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
		pglClientActiveTextureARB(GL_TEXTURE0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	if (m_StreamFlags & STREAM_UV1)
	{
		pglClientActiveTextureARB(GL_TEXTURE1);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		pglClientActiveTextureARB(GL_TEXTURE0);
	}
}

#endif // !CONFIG2_GLES

void CShaderProgram::AssertPointersBound()
{
	ENSURE((m_StreamFlags & ~m_ValidStreams) == 0);
}
