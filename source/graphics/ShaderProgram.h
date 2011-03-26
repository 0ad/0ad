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

#ifndef INCLUDED_SHADERPROGRAM
#define INCLUDED_SHADERPROGRAM

#include "lib/ogl.h"
#include "lib/file/vfs/vfs_path.h"
#include "lib/res/handle.h"
#include "ps/CStr.h"

class CColor;
class CMatrix3D;
class CVector3D;

/**
 * A compiled vertex+fragment shader program.
 * The implementation may use GL_ARB_{vertex,fragment}_program (assembly syntax)
 * or GL_ARB_{vertex,fragment}_shader (GLSL); the difference is hidden from the caller.
 *
 * Texture/uniform IDs are typically strings, corresponding to the names defined
 * in the shader .xml file. Alternatively (and more efficiently, if used extremely
 * frequently), call GetUniformBinding and pass its return value as the ID.
 * Setting uniforms that the shader .xml doesn't support is harmless.
 */
class CShaderProgram
{
	NONCOPYABLE(CShaderProgram);

public:
	/**
	 * Construct based on ARB vertex/fragment program files.
	 */
	static CShaderProgram* ConstructARB(const VfsPath& vertexFile, const VfsPath& fragmentFile,
		const std::map<CStr, CStr>& defines,
		const std::map<CStr, int>& vertexIndexes, const std::map<CStr, int>& fragmentIndexes,
		int streamflags);

	typedef const char* attrib_id_t;
	typedef const char* texture_id_t;
	typedef const char* uniform_id_t;

	/**
	 * Represents a uniform attribute binding.
	 */
	struct Binding
	{
		friend class CShaderProgramARB;
	private:
		Binding(int v, int f) : vertex(v), fragment(f) { }
		i16 vertex;
		i16 fragment;
	public:
		Binding() : vertex(-1), fragment(-1) { }

		/**
		 * Returns whether this uniform attribute is active in the shader.
		 * If not then there's no point calling Uniform() to set its value.
		 */
		bool Active() { return vertex != -1 || fragment != -1; }
	};

	virtual ~CShaderProgram() { }

	virtual void Reload() = 0;

	/**
	 * Returns whether this shader was successfully loaded.
	 */
	bool IsValid() const;

	/**
	 * Binds the shader into the GL context. Call this before calling Uniform()
	 * or trying to render with it.
	 */
	virtual void Bind() = 0;

	/**
	 * Unbinds the shader from the GL context. Call this after rendering with it.
	 */
	virtual void Unbind() = 0;

	/**
	 * Returns bitset of STREAM_* value, indicating what vertex data streams the
	 * vertex shader needs.
	 */
	int GetStreamFlags() const;

	// TODO: implement vertex attributes
	GLuint GetAttribIndex(attrib_id_t id);

	/**
	 * Returns whether the shader needs the texture with the given name.
	 */
	virtual bool HasTexture(texture_id_t id) = 0;

	virtual void BindTexture(texture_id_t id, Handle tex) = 0;

	virtual void BindTexture(texture_id_t id, GLuint tex) = 0;

	virtual Binding GetUniformBinding(uniform_id_t id) = 0;

	virtual void Uniform(uniform_id_t id, int v) = 0;
	virtual void Uniform(uniform_id_t id, float v) = 0;
	virtual void Uniform(uniform_id_t id, float v0, float v1, float v2, float v3) = 0;
	virtual void Uniform(uniform_id_t id, const CVector3D& v) = 0;
	virtual void Uniform(uniform_id_t id, const CColor& v) = 0;
	virtual void Uniform(uniform_id_t id, const CMatrix3D& v) = 0;

	virtual void Uniform(Binding id, int v) = 0;
	virtual void Uniform(Binding id, float v) = 0;
	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3) = 0;
	virtual void Uniform(Binding id, const CVector3D& v) = 0;
	virtual void Uniform(Binding id, const CColor& v) = 0;
	virtual void Uniform(Binding id, const CMatrix3D& v) = 0;

protected:
	CShaderProgram(int streamflags);

	bool m_IsValid;
	int m_StreamFlags;
};

typedef shared_ptr<CShaderProgram> CShaderProgramPtr;

#endif // INCLUDED_SHADERPROGRAM
