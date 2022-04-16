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

#ifndef INCLUDED_RENDERER_BACKEND_GL_SHADERPROGRAM
#define INCLUDED_RENDERER_BACKEND_GL_SHADERPROGRAM

#include "lib/ogl.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/CStrForward.h"
#include "renderer/backend/Format.h"
#include "renderer/backend/gl/Texture.h"
#include "renderer/backend/IShaderProgram.h"

#include <map>
#include <vector>

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
	STREAM_UV4 = (1 << 7),
	STREAM_UV5 = (1 << 8),
	STREAM_UV6 = (1 << 9),
	STREAM_UV7 = (1 << 10),
};

namespace Renderer
{

namespace Backend
{

namespace GL
{

class CDevice;

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
class CShaderProgram : public IShaderProgram
{
	NONCOPYABLE(CShaderProgram);

public:
	typedef CStrIntern attrib_id_t;
	typedef CStrIntern texture_id_t;
	typedef CStrIntern uniform_id_t;
	typedef std::pair<int, GLenum> frag_index_pair_t;

	static std::unique_ptr<CShaderProgram> Create(CDevice* device, const CStr& name, const CShaderDefines& baseDefines);

	/**
	 * Represents a uniform attribute or texture binding.
	 * For uniforms:
	 *  - ARB shaders store vertex location in 'first', fragment location in 'second'.
	 *  - GLSL shaders store uniform location in 'first', data type in 'second'.
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
		bool Active() const { return first != -1 || second != -1; }

		int first;
		int second;
	};

	~CShaderProgram() override;

	/**
	 * Binds the shader into the GL context. Call this before calling Uniform()
	 * or trying to render with it.
	 */
	virtual void Bind(CShaderProgram* previousShaderProgram) = 0;

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
	void BindTexture(texture_id_t id, const Renderer::Backend::GL::CTexture* tex);
	void BindTexture(Binding id, const Renderer::Backend::GL::CTexture* tex);

	virtual Binding GetUniformBinding(uniform_id_t id) = 0;

	// Uniform-setting methods that subclasses must define:
	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3) = 0;
	virtual void Uniform(Binding id, const CMatrix3D& v) = 0;
	virtual void Uniform(Binding id, size_t count, const CMatrix3D* v) = 0;
	virtual void Uniform(Binding id, size_t count, const float* v) = 0;

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
	void Uniform(uniform_id_t id, size_t count, const float* v);

	// Vertex attribute pointers (equivalent to glVertexPointer etc):

	virtual void VertexPointer(const Renderer::Backend::Format format, GLsizei stride, const void* pointer);
	virtual void NormalPointer(const Renderer::Backend::Format format, GLsizei stride, const void* pointer);
	virtual void ColorPointer(const Renderer::Backend::Format format, GLsizei stride, const void* pointer);
	virtual void TexCoordPointer(GLenum texture, const Renderer::Backend::Format format, GLsizei stride, const void* pointer);
	virtual void VertexAttribPointer(attrib_id_t id, const Renderer::Backend::Format format, GLboolean normalized, GLsizei stride, const void* pointer);

	/**
	 * Checks that all the required vertex attributes have been set.
	 * Call this before calling Draw/DrawIndexed etc to avoid potential crashes.
	 */
	void AssertPointersBound();

	virtual std::vector<VfsPath> GetFileDependencies() const = 0;

protected:
	CShaderProgram(int streamflags);

	/**
	 * Construct based on ARB vertex/fragment program files.
	 */
	static std::unique_ptr<CShaderProgram> ConstructARB(
		CDevice* device, const VfsPath& vertexFile, const VfsPath& fragmentFile,
		const CShaderDefines& defines,
		const std::map<CStrIntern, int>& vertexIndexes, const std::map<CStrIntern, frag_index_pair_t>& fragmentIndexes,
		int streamflags);

	/**
	 * Construct based on GLSL vertex/fragment shader files.
	 */
	static std::unique_ptr<CShaderProgram> ConstructGLSL(
		CDevice* device, const CStr& name,
		const VfsPath& vertexFile, const VfsPath& fragmentFile,
		const CShaderDefines& defines,
		const std::map<CStrIntern, int>& vertexAttribs,
		int streamflags);

	virtual void BindTexture(texture_id_t id, GLuint tex) = 0;
	virtual void BindTexture(Binding id, GLuint tex) = 0;

	int m_StreamFlags;

	// Non-GLSL client state handling:
	void BindClientStates();
	void UnbindClientStates();
	int m_ValidStreams; // which streams have been specified via VertexPointer etc since the last Bind
};

} // namespace GL

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_GL_SHADERPROGRAM
