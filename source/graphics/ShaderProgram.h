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

#ifndef INCLUDED_SHADERPROGRAM
#define INCLUDED_SHADERPROGRAM

#include "lib/ogl.h"
#include "lib/file/vfs/vfs_path.h"
#include "lib/res/handle.h"
#include "ps/CStr.h"

#include <map>

struct CColor;
class CMatrix3D;
class CVector3D;
class CPreprocessor;

// Vertex data stream flags
enum
{
	STREAM_POS = (1 << 0),
	STREAM_NORMAL = (1 << 1),
	STREAM_COLOR = (1 << 2),
	STREAM_UV0 = (1 << 3),
	STREAM_UV1 = (1 << 4),
	STREAM_UV2 = (1 << 5),
	STREAM_UV3 = (1 << 6),
	STREAM_POSTOUV0 = (1 << 7),
	STREAM_POSTOUV1 = (1 << 8),
	STREAM_POSTOUV2 = (1 << 9),
	STREAM_POSTOUV3 = (1 << 10)
};

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

	/**
	 * Construct based on GLSL vertex/fragment shader files.
	 */
	static CShaderProgram* ConstructGLSL(const VfsPath& vertexFile, const VfsPath& fragmentFile,
		const std::map<CStr, CStr>& defines,
		const std::map<CStr, GLenum>& uniformTypes,
		int streamflags);

	/**
	 * Construct an instance of a pre-defined fixed-function pipeline setup.
	 */
	static CShaderProgram* ConstructFFP(const std::string& id, const std::map<CStr, CStr>& defines);

	typedef const char* attrib_id_t;
	typedef const char* texture_id_t;
	typedef const char* uniform_id_t;

	/**
	 * Represents a uniform attribute binding.
	 * ARB shaders store vertex location in 'first', fragment location in 'second'.
	 * GLSL shaders store uniform location in 'first', data type in 'second'.
	 * FFP shaders store -1 in 'first', index in 'second'.
	 * Non-existent bindings must store -1 in both.
	 */
	struct Binding
	{
		Binding(int a, int b) : first(a), second(b) { }

		Binding() : first(-1), second(-1) { }

		/**
		 * Returns whether this uniform attribute is active in the shader.
		 * If not then there's no point calling Uniform() to set its value.
		 */
		bool Active() { return first != -1 || second != -1; }

		int first;
		int second;
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

	// Uniform-setting methods that subclasses must define:
	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3) = 0;
	virtual void Uniform(Binding id, const CMatrix3D& v) = 0;

	// Convenient uniform-setting wrappers:

	void Uniform(Binding id, int v);
	void Uniform(Binding id, float v);
	void Uniform(Binding id, const CVector3D& v);
	void Uniform(Binding id, const CColor& v);

	void Uniform(uniform_id_t id, int v);
	void Uniform(uniform_id_t id, float v);
	void Uniform(uniform_id_t id, const CVector3D& v);
	void Uniform(uniform_id_t id, const CColor& v);
	void Uniform(uniform_id_t id, float v0, float v1, float v2, float v3);
	void Uniform(uniform_id_t id, const CMatrix3D& v);

protected:
	CShaderProgram(int streamflags);

	CStr Preprocess(CPreprocessor& preprocessor, const CStr& input);

	bool m_IsValid;
	int m_StreamFlags;
};

typedef shared_ptr<CShaderProgram> CShaderProgramPtr;

#endif // INCLUDED_SHADERPROGRAM
