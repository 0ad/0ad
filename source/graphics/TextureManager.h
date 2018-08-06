/* Copyright (C) 2018 Wildfire Games.
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

#ifndef INCLUDED_TEXTUREMANAGER
#define INCLUDED_TEXTUREMANAGER

#include "Texture.h"

#include <memory>

#include "lib/ogl.h"
#include "lib/file/vfs/vfs.h"
#include "lib/res/handle.h"

class CTextureProperties;
class CTextureManagerImpl;

/**
 * Texture manager with asynchronous loading and automatic DDS conversion/compression.
 *
 * Input textures can be any format. They will be converted to DDS using settings defined
 * in files named "texture.xml", in the same directory as the texture and in its parent
 * directories. See CTextureConverter for the XML syntax. The DDS file will be cached
 * for faster loading in the future.
 *
 * Typically the graphics code will initialise many textures at the start of the game,
 * mostly for off-screen objects, by calling CreateTexture().
 * Loading texture data may be very slow (especially if it needs to be converted
 * to DDS), and we don't want the game to become unresponsive.
 * CreateTexture therefore returns an object immediately, without loading the
 * texture. If the object is never used then the data will never be loaded.
 *
 * Typically, the renderer will call CTexture::Bind() when it wants to use the
 * texture. This will trigger the loading of the texture data. If it can be loaded
 * quickly (i.e. there is already a cached DDS version), then it will be loaded before
 * the function returns, and the texture can be rendered as normal.
 *
 * If loading will take a long time, then Bind() binds a default placeholder texture
 * and starts loading the texture in the background. It will use the correct texture
 * when the renderer next calls Bind() after the load has finished.
 *
 * It is also possible to prefetch textures which are not being rendered yet, but
 * are expected to be rendered soon (e.g. for off-screen terrain tiles).
 * These will be loaded in the background, when there are no higher-priority textures
 * to load.
 *
 * The same texture file can be safely loaded multiple times with different GL parameters
 * (but this should be avoided whenever possible, as it wastes VRAM).
 *
 * For release packages, DDS files can be precached by appending ".dds" to their name,
 * which will be used instead of doing runtime conversion. This means most players should
 * never experience the slow asynchronous conversion behaviour.
 * These cache files will typically be packed into an archive for faster loading;
 * if no archive cache is available then the source file will be converted and stored
 * as a loose cache file on the user's disk.
 */
class CTextureManager
{
	NONCOPYABLE(CTextureManager);

public:
	/**
	 * Construct texture manager. vfs must be the VFS instance used for all textures
	 * loaded from this object.
	 * highQuality is slower and intended for batch-conversion modes.
	 * disableGL is intended for tests, and will disable all GL uploads.
	 */
	CTextureManager(PIVFS vfs, bool highQuality, bool disableGL);

	~CTextureManager();

	/**
	 * Create a texture with the given GL properties.
	 * The texture data will not be loaded immediately.
	 */
	CTexturePtr CreateTexture(const CTextureProperties& props);

	/**
	 * Returns a magenta texture. Use this for highlighting errors
	 * (e.g. missing terrain textures).
	 */
	CTexturePtr GetErrorTexture();

	/**
	 * Work on asynchronous texture loading operations, if any.
	 * Returns true if it did any work.
	 * The caller should typically loop this per frame until it returns
	 * false or exceeds the allocated time for this frame.
	 */
	bool MakeProgress();

	/**
	 * Synchronously converts and compresses and saves the texture,
	 * and returns the output path (minus a "cache/" prefix). This
	 * is intended for pre-caching textures in release archives.
	 * @return true on success
	 */
	bool GenerateCachedTexture(const VfsPath& path, VfsPath& outputPath);

	/**
	 * Returns true if the given texture exists.
	 * This tests both for the original and converted filename.
	 */
	bool TextureExists(const VfsPath& path) const;

	/**
	 * Returns total number of bytes uploaded for all current texture.
	 */
	size_t GetBytesUploaded() const;

private:
	CTextureManagerImpl* m;
};

/**
 * Represents the filename and GL parameters of a texture,
 * for passing to CTextureManager::CreateTexture.
 */
class CTextureProperties
{
	friend class CTextureManagerImpl;
	friend struct TextureCacheCmp;
	friend struct TPequal_to;
	friend struct TPhash;

public:
	/**
	 * Use the given texture name, and default GL parameters.
	 */
	explicit CTextureProperties(const VfsPath& path) :
		m_Path(path), m_Filter(GL_LINEAR_MIPMAP_LINEAR),
		m_WrapS(GL_REPEAT), m_WrapT(GL_REPEAT), m_Aniso(1.0f), m_Format(0)
	{
	}

	/**
	 * Set min/mag filter mode (typically GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST, etc).
	 */
	void SetFilter(GLint filter) { m_Filter = filter; }

	/**
	 * Set wrapping mode (typically GL_REPEAT, GL_CLAMP_TO_EDGE, etc).
	 */
	void SetWrap(GLint wrap) { m_WrapS = wrap; m_WrapT = wrap; }

	/**
	 * Set wrapping mode (typically GL_REPEAT, GL_CLAMP_TO_EDGE, etc),
	 * separately for S and T.
	 */
	void SetWrap(GLint wrap_s, GLint wrap_t) { m_WrapS = wrap_s; m_WrapT = wrap_t; }

	/**
	 * Set maximum anisotropy value. Must be >= 1.0. Should be a power of 2.
	 */
	void SetMaxAnisotropy(float aniso) { m_Aniso = aniso; }

	/**
	 * Set GL texture upload format, to override the default.
	 * Typically GL_ALPHA or GL_LUMINANCE for 8-bit textures.
	 */
	void SetFormatOverride(GLenum format) { m_Format = format; }

	// TODO: rather than this static definition of texture properties
	// (especially anisotropy), maybe we want something that can be more
	// easily tweaked in an Options menu? e.g. the caller just specifies
	// "terrain texture mode" and we combine it with the user's options.
	// That'd let us dynamically change texture properties easily.
	//
	// enum EQualityMode
	// {
	//   NONE,
	//   TERRAIN,
	//   MODEL,
	//   GUI
	// }
	// void SetQuality(EQualityMode mode, float anisotropy, float lodbias, int reducemipmaps, ...);
	//
	// or something a bit like that.

private:
	// Must update TPhash, TPequal_to when changing these fields
	VfsPath m_Path;
	GLint m_Filter;
	GLint m_WrapS;
	GLint m_WrapT;
	float m_Aniso;
	GLenum m_Format;
};

/**
 * Represents a texture object.
 * The texture data may or may not have been loaded yet.
 * Before it has been loaded, all operations will act on a default
 * 1x1-pixel grey texture instead.
 */
class CTexture
{
	friend class CTextureManagerImpl;
	friend struct TextureCacheCmp;
	friend struct TPequal_to;
	friend struct TPhash;

	// Only the texture manager can create these
	explicit CTexture(Handle handle, const CTextureProperties& props, CTextureManagerImpl* textureManager);

	NONCOPYABLE(CTexture);

public:

	~CTexture();

	/**
	 * Returns the width (in pixels) of the current texture.
	 */
	size_t GetWidth() const;

	/**
	 * Returns the height (in pixels) of the current texture.
	 */
	size_t GetHeight() const;

	/**
	 * Returns whether the current texture has an alpha channel.
	 */
	bool HasAlpha() const;

	/**
	 * Returns the ARGB value of the lowest mipmap level (i.e. the
	 * average of the whole texture).
	 * Returns 0 if the texture has no mipmaps.
	 */
	u32 GetBaseColor() const;

	/**
	 * Returns total number of bytes uploaded for this texture.
	 */
	size_t GetUploadedSize() const;

	/**
	 * Bind the texture to the given GL texture unit.
	 * If the texture data hasn't been loaded yet, this may wait a short while to
	 * load it. If loading takes too long then it will return sooner and the data will
	 * be loaded in a background thread, so this does not guarantee the texture really
	 * will be loaded.
	 */
	void Bind(size_t unit = 0);

	/**
	 * Returns a ogl_tex handle, for later binding. See comments from Bind().
	 */
	Handle GetHandle();

	/**
	 * Attempt to load the texture data quickly, as with Bind().
	 * Returns whether the texture data is currently loaded.
	 */
	bool TryLoad();

	/**
	 * Returns whether the texture data is currently loaded.
	 */
	bool IsLoaded();

	/**
	 * Activate the prefetching optimisation for this texture.
	 * Use this if it is likely the texture will be needed in the near future.
	 * It will be loaded in the background so that it is likely to be ready when
	 * it is used by Bind().
	 */
	void Prefetch();

private:
	/**
	 * Replace the Handle stored by this object.
	 * If takeOwnership is true, it will not increment the Handle's reference count.
	 */
	void SetHandle(Handle handle, bool takeOwnership = false);

	const CTextureProperties m_Properties;

	Handle m_Handle;
	u32 m_BaseColor;

	enum {
		UNLOADED, // loading has not started
		PREFETCH_NEEDS_LOADING, // was prefetched; currently waiting to try loading from cache
		PREFETCH_NEEDS_CONVERTING, // was prefetched; currently waiting to be sent to the texture converter
		PREFETCH_IS_CONVERTING, // was prefetched; currently being processed by the texture converter
		HIGH_NEEDS_CONVERTING, // high-priority; currently waiting to be sent to the texture converter
		HIGH_IS_CONVERTING, // high-priority; currently being processed by the texture converter
		LOADED // loading has completed (successfully or not)
	} m_State;

	CTextureManagerImpl* m_TextureManager;

	// Self-reference to let us recover the CTexturePtr for this object.
	// (weak pointer to avoid cycles)
	std::weak_ptr<CTexture> m_Self;
};

std::size_t hash_value(const CTexturePtr& v);
std::size_t hash_value(const CTextureProperties& v);

#endif // INCLUDED_TEXTUREMANAGER
