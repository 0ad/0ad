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

#include "graphics/ShaderProgramPtr.h"
#include "graphics/Texture.h"
#include "lib/ogl.h"
#include "lib/file/vfs/vfs_path.h"
#include "lib/res/handle.h"

#include <map>

struct CColor;
class CMatrix3D;
class CVector3D;
class CShaderDefines;
class CStrIntern;

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
 * The implementation may use GL_ARB_{vertex,fragment}_program (ARB assembly syntax)
 * or GL_ARB_{vertex,fragment}_shader (GLSL), or may use hard-coded fixed-function
 * multitexturing setup code; the difference is hidden from the caller.
 *
 * Texture/uniform IDs are typically strings, corresponding to the names defined in
 * the shader .xml file. Alternatively (and more efficiently, if used very frequently),
 * call GetTextureBinding/GetUniformBinding and pass its return value as the ID.
 * Setting uniforms that the shader .xml doesn't support is harmless.
 * 
 * For a high-level overview of shaders and materials, see
 * http://trac.wildfiregames.com/wiki/MaterialSystem
 */
class CShaderProgram
{
	NONCOPYABLE(CShaderProgram);

public:
	typedef CStrIntern attrib_id_t;
	typedef CStrIntern texture_id_t;
	typedef CStrIntern uniform_id_t;
	typedef std::pair<int, GLenum> frag_index_pair_t;
	
	/**
	 * Construct based on ARB vertex/fragment program files.
	 */
	static CShaderProgram* ConstructARB(const VfsPath& vertexFile, const VfsPath& fragmentFile,
		const CShaderDefines& defines,
		const std::map<CStrIntern, int>& vertexIndexes, const std::map<CStrIntern, frag_index_pair_t>& fragmentIndexes,
		int streamflags);

	/**
	 * Construct based on GLSL vertex/fragment shader files.
	 */
	static CShaderProgram* ConstructGLSL(const VfsPath& vertexFile, const VfsPath& fragmentFile,
		const CShaderDefines& defines,
		const std::map<CStrIntern, int>& vertexAttribs,
		int streamflags);

	/**
	 * Construct an instance of a pre-defined fixed-function pipeline setup.
	 */
	static CShaderProgram* ConstructFFP(const std::string& id, const CShaderDefines& defines);
	
	/**
	 * Represents a uniform attribute or texture binding.
	 * For uniforms:
	 *  - ARB shaders store vertex location in 'first', fragment location in 'second'.
	 *  - GLSL shaders store uniform location in 'first', data type in 'second'.
	 *  - FFP shaders store -1 in 'first', index in 'second'.
	 * For textures, all store texture target (e.g. GL_TEXTURE_2D) in 'first', texture unit in 'second'.
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
	 * vertex shader needs (e.g. position, color, UV, ...).
	 */
	int GetStreamFlags() const;


	virtual Binding GetTextureBinding(texture_id_t id) = 0;

	// Variants of texture binding:
	void BindTexture(texture_id_t id, CTexturePtr tex);
	virtual void BindTexture(texture_id_t id, Handle tex) = 0;
	virtual void BindTexture(texture_id_t id, GLuint tex) = 0;
	virtual void BindTexture(Binding id, Handle tex) = 0;


	virtual Binding GetUniformBinding(uniform_id_t id) = 0;

	// Uniform-setting methods that subclasses must define:
	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3) = 0;
	virtual void Uniform(Binding id, const CMatrix3D& v) = 0;
	virtual void Uniform(Binding id, size_t count, const CMatrix3D* v) = 0;

	// Convenient uniform-setting wrappers:

	void Uniform(Binding id, int v);
	void Uniform(Binding id, float v);
	void Uniform(Binding id, float v0, float v1);
	void Uniform(Binding id, const CVector3D& v);
	void Uniform(Binding id, const CColor& v);

	void Uniform(uniform_id_t id, int v);
	void Uniform(uniform_id_t id, float v);
	void Uniform(uniform_id_t id, float v0, float v1);
	void Uniform(uniform_id_t id, const CVector3D& v);
	void Uniform(uniform_id_t id, const CColor& v);
	void Uniform(uniform_id_t id, float v0, float v1, float v2, float v3);
	void Uniform(uniform_id_t id, const CMatrix3D& v);
	void Uniform(uniform_id_t id, size_t count, const CMatrix3D* v);

	// Vertex attribute pointers (equivalent to glVertexPointer etc):

	virtual void VertexPointer(GLint size, GLenum type, GLsizei stride, void* pointer);
	virtual void NormalPointer(GLenum type, GLsizei stride, void* pointer);
	virtual void ColorPointer(GLint size, GLenum type, GLsizei stride, void* pointer);
	virtual void TexCoordPointer(GLenum texture, GLint size, GLenum type, GLsizei stride, void* pointer);
	virtual void VertexAttribPointer(attrib_id_t id, GLint size, GLenum type, GLboolean normalized, GLsizei stride, void* pointer);
	virtual void VertexAttribIPointer(attrib_id_t id, GLint size, GLenum type, GLsizei stride, void* pointer);

	/**
	 * Checks that all the required vertex attributes have been set.
	 * Call this before calling glDrawArrays/glDrawElements etc to avoid potential crashes.
	 */
	void AssertPointersBound();

protected:
	CShaderProgram(int streamflags);

	bool m_IsValid;
	int m_StreamFlags;

	// Non-GLSL client state handling:
	void BindClientStates();
	void UnbindClientStates();
	int m_ValidStreams; // which streams have been specified via VertexPointer etc since the last Bind
};

#endif // INCLUDED_SHADERPROGRAM
