/* Copyright (C) 2011 Wildfire Games.
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

#include "lib/res/graphics/ogl_tex.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Overlay.h"
#include "ps/Preprocessor.h"

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

	CStr Preprocess(CPreprocessor& preprocessor, const CStr& input)
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
	}

	virtual void Unbind()
	{
		glDisable(GL_VERTEX_PROGRAM_ARB);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		pglBindProgramARB(GL_VERTEX_PROGRAM_ARB, 0);
		pglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0);

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

	virtual Binding GetUniformBinding(uniform_id_t id)
	{
		return Binding(GetUniformVertexIndex(id), GetUniformFragmentIndex(id));
	}

	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3)
	{
		if (id.vertex != -1)
			pglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.vertex, v0, v1, v2, v3);

		if (id.fragment != -1)
			pglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.fragment, v0, v1, v2, v3);
	}

	virtual void Uniform(Binding id, const CMatrix3D& v)
	{
		if (id.vertex != -1)
		{
			pglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.vertex+0, v._11, v._12, v._13, v._14);
			pglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.vertex+1, v._21, v._22, v._23, v._24);
			pglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.vertex+2, v._31, v._32, v._33, v._34);
			pglProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, (GLuint)id.vertex+3, v._41, v._42, v._43, v._44);
		}

		if (id.fragment != -1)
		{
			pglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.fragment+0, v._11, v._12, v._13, v._14);
			pglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.fragment+1, v._21, v._22, v._23, v._24);
			pglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.fragment+2, v._31, v._32, v._33, v._34);
			pglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, (GLuint)id.fragment+3, v._41, v._42, v._43, v._44);
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



CShaderProgram::CShaderProgram(int streamflags)
	: m_IsValid(false), m_StreamFlags(streamflags)
{
}

/*static*/ CShaderProgram* CShaderProgram::ConstructARB(const VfsPath& vertexFile, const VfsPath& fragmentFile,
	const std::map<CStr, CStr>& defines,
	const std::map<CStr, int>& vertexIndexes, const std::map<CStr, int>& fragmentIndexes,
	int streamflags)
{
	return new CShaderProgramARB(vertexFile, fragmentFile, defines, vertexIndexes, fragmentIndexes, streamflags);
}

bool CShaderProgram::IsValid() const
{
	return m_IsValid;
}

int CShaderProgram::GetStreamFlags() const
{
	return m_StreamFlags;
}

void CShaderProgram::Uniform(Binding id, int v)
{
	Uniform(id, (float)v, (float)v, (float)v, (float)v);
}

void CShaderProgram::Uniform(Binding id, float v)
{
	Uniform(id, v, v, v, v);
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
