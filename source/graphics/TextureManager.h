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

#ifndef INCLUDED_TEXTUREMANAGER
#define INCLUDED_TEXTUREMANAGER

#include "graphics/Texture.h"
#include "lib/file/vfs/vfs.h"
#include "lib/tex/tex.h"
#include "renderer/backend/gl/DeviceCommandContext.h"
#include "renderer/backend/gl/Texture.h"

#include <memory>

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
	 * Create a texture with the given properties.
	 * The texture data will not be loaded immediately.
	 */
	CTexturePtr CreateTexture(const CTextureProperties& props);

	/**
	 * Wraps a backend texture.
	 */
	CTexturePtr WrapBackendTexture(
		std::unique_ptr<Renderer::Backend::GL::CTexture> backendTexture);

	/**
	 * Returns a magenta texture. Use this for highlighting errors
	 * (e.g. missing terrain textures).
	 */
	const CTexturePtr& GetErrorTexture();

	/**
	 * Returns a single color RGBA texture with CColor(1.0f, 1.0f, 1.0f, 1.0f).
	 */
	const CTexturePtr& GetWhiteTexture();

	/**
	 * Returns a single color RGBA texture with CColor(0.0f, 0.0f, 0.0f, 0.0f).
	 */
	const CTexturePtr& GetTransparentTexture();

	/**
	 * Returns a white RGBA texture with alpha gradient.
	 */
	const CTexturePtr& GetAlphaGradientTexture();

	/**
	 * Returns a single color RGBA texture cube with CColor(0.0f, 0.0f, 0.0f, 1.0f).
	 */
	const CTexturePtr& GetBlackTextureCube();

	/**
	 * Work on asynchronous texture loading operations, if any.
	 * Returns true if it did any work.
	 * The caller should typically loop this per frame until it returns
	 * false or exceeds the allocated time for this frame.
	 */
	bool MakeProgress();

	/**
	 * Work on asynchronous texture uploading operations, if any.
	 * Returns true if it did any work. Mostly the same as MakeProgress.
	 */
	bool MakeUploadProgress(Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext);

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

	/**
	 * Should be called on any quality or anisotropic change.
	 */
	void OnQualityChanged();

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
	explicit CTextureProperties(const VfsPath& path)
		: m_Path(path)
	{
	}

	CTextureProperties(
		const VfsPath& path, const Renderer::Backend::Format formatOverride)
		: m_Path(path), m_FormatOverride(formatOverride)
	{
	}

	/**
	 * Set sampler address mode.
	 */
	void SetAddressMode(const Renderer::Backend::Sampler::AddressMode addressMode)
	{
		m_AddressModeU = m_AddressModeV = addressMode;
	}

	/**
	 * Set sampler address mode separately for different coordinates.
	 */
	void SetAddressMode(
		const Renderer::Backend::Sampler::AddressMode addressModeU,
		const Renderer::Backend::Sampler::AddressMode addressModeV)
	{
		m_AddressModeU = addressModeU;
		m_AddressModeV = addressModeV;
	}

	/**
	 * The value of max anisotropy is set by options. Though it might make sense
	 * to add an override.
	 */
	void SetAnisotropicFilter(const bool enabled) { m_AnisotropicFilterEnabled = enabled; }

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

	void SetIgnoreQuality(bool ignore) { m_IgnoreQuality = ignore; }

private:
	// Must update TPhash, TPequal_to when changing these fields
	VfsPath m_Path;

	Renderer::Backend::Sampler::AddressMode m_AddressModeU =
		Renderer::Backend::Sampler::AddressMode::REPEAT;
	Renderer::Backend::Sampler::AddressMode m_AddressModeV =
		Renderer::Backend::Sampler::AddressMode::REPEAT;
	bool m_AnisotropicFilterEnabled = false;
	Renderer::Backend::Format m_FormatOverride =
		Renderer::Backend::Format::UNDEFINED;
	bool m_IgnoreQuality = false;
};

/**
 * Represents a texture object.
 * The texture data may or may not have been loaded yet.
 * Before it has been loaded, all operations will act on a default
 * 1x1-pixel grey texture instead.
 */
class CTexture
{
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
	 * Uploads a texture data to a backend texture if successfully loaded.
	 * If the texture data hasn't been loaded yet, this may wait a short while to
	 * load it. If loading takes too long then it will return sooner and the data will
	 * be loaded in a background thread, so this does not guarantee the texture really
	 * will be uploaded.
	 */
	void UploadBackendTextureIfNeeded(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext);

	/**
	 * Returns a backend texture if successfully uploaded, else fallback.
	 */
	Renderer::Backend::GL::CTexture* GetBackendTexture();
	const Renderer::Backend::GL::CTexture* GetBackendTexture() const;

	/**
	 * Attempt to load the texture data quickly, as with
	 * GetUploadedBackendTextureIfNeeded(). Returns whether the texture data is
	 * currently loaded (but not uploaded).
	 */
	bool TryLoad();

	/**
	 * Returns whether the texture data is currently loaded.
	 */
	bool IsLoaded() const { return m_State == LOADED; }

	/**
	 * Returns whether the texture data is currently uploaded.
	 */
	bool IsUploaded() const { return m_State == UPLOADED; }

	/**
	 * Activate the prefetching optimisation for this texture.
	 * Use this if it is likely the texture will be needed in the near future.
	 * It will be loaded in the background so that it is likely to be ready when
	 * it is used by Bind().
	 */
	void Prefetch();

private:
	friend class CTextureManagerImpl;
	friend class CPredefinedTexture;
	friend struct TextureCacheCmp;
	friend struct TPequal_to;
	friend struct TPhash;

	// Only the texture manager can create these
	explicit CTexture(
		std::unique_ptr<Renderer::Backend::GL::CTexture> texture,
		Renderer::Backend::GL::CTexture* fallback,
		const CTextureProperties& props, CTextureManagerImpl* textureManager);

	void ResetBackendTexture(
		std::unique_ptr<Renderer::Backend::GL::CTexture> backendTexture,
		Renderer::Backend::GL::CTexture* fallbackBackendTexture);

	const CTextureProperties m_Properties;

	std::unique_ptr<Renderer::Backend::GL::CTexture> m_BackendTexture;
	// It's possible to m_FallbackBackendTexture references m_BackendTexture.
	Renderer::Backend::GL::CTexture* m_FallbackBackendTexture = nullptr;
	u32 m_BaseColor;
	std::unique_ptr<Tex> m_TextureData;
	size_t m_UploadedSize = 0;
	uint32_t m_BaseLevelOffset = 0;

	enum
	{
		UNLOADED, // loading has not started
		PREFETCH_NEEDS_LOADING, // was prefetched; currently waiting to try loading from cache
		PREFETCH_NEEDS_CONVERTING, // was prefetched; currently waiting to be sent to the texture converter
		PREFETCH_IS_CONVERTING, // was prefetched; currently being processed by the texture converter
		HIGH_NEEDS_CONVERTING, // high-priority; currently waiting to be sent to the texture converter
		HIGH_IS_CONVERTING, // high-priority; currently being processed by the texture converter
		LOADED, // loading texture data has completed (successfully or not)
		UPLOADED // uploading to backend has completed (successfully or not)
	} m_State;

	CTextureManagerImpl* m_TextureManager;

	// Self-reference to let us recover the CTexturePtr for this object.
	// (weak pointer to avoid cycles)
	std::weak_ptr<CTexture> m_Self;
};

#endif // INCLUDED_TEXTUREMANAGER
