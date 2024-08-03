/* Copyright (C) 2024 Wildfire Games.
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

#include "TextureManager.h"

#include "graphics/Color.h"
#include "graphics/TextureConverter.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/bits.h"
#include "lib/file/vfs/vfs_tree.h"
#include "lib/hash.h"
#include "lib/timer.h"
#include "maths/MathUtil.h"
#include "maths/MD5.h"
#include "ps/CacheLoader.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/Util.h"
#include "renderer/backend/IDevice.h"
#include "renderer/Renderer.h"

#include <algorithm>
#include <boost/filesystem.hpp>
#include <iomanip>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace
{

Renderer::Backend::Format ChooseFormatAndTransformTextureDataIfNeeded(
	Renderer::Backend::IDevice* device, Tex& textureData, const bool hasS3TC)
{
	const bool alpha = (textureData.m_Flags & TEX_ALPHA) != 0;
	const bool grey = (textureData.m_Flags & TEX_GREY) != 0;
	const size_t dxt = textureData.m_Flags & TEX_DXT;

	// Some backends don't support BGR as an internal format (like GLES).
	// TODO: add a check that the format is internally supported.
	if ((textureData.m_Flags & TEX_BGR) != 0)
	{
		LOGWARNING("Using slow path to convert BGR texture.");
		textureData.transform_to(textureData.m_Flags & ~TEX_BGR);
	}

	if (dxt)
	{
		if (hasS3TC)
		{
			switch (dxt)
			{
			case DXT1A:
				return Renderer::Backend::Format::BC1_RGBA_UNORM;
			case 1:
				return Renderer::Backend::Format::BC1_RGB_UNORM;
			case 3:
				return Renderer::Backend::Format::BC2_UNORM;
			case 5:
				return Renderer::Backend::Format::BC3_UNORM;
			default:
				LOGERROR("Unknown DXT compression.");
				return Renderer::Backend::Format::UNDEFINED;
			}
		}
		else
			textureData.transform_to(textureData.m_Flags & ~TEX_DXT);
	}

	switch (textureData.m_Bpp)
	{
	case 8:
		ENSURE(grey);
		return Renderer::Backend::Format::L8_UNORM;
	case 24:
		ENSURE(!alpha);
		if (device->IsTextureFormatSupported(Renderer::Backend::Format::R8G8B8_UNORM))
			return Renderer::Backend::Format::R8G8B8_UNORM;
		else
		{
			LOGWARNING("Using slow path to convert unsupported RGB texture to RGBA.");
			textureData.transform_to(textureData.m_Flags & TEX_ALPHA);
			return Renderer::Backend::Format::R8G8B8A8_UNORM;
		}
	case 32:
		ENSURE(alpha);
		return Renderer::Backend::Format::R8G8B8A8_UNORM;
	default:
		LOGERROR("Unsupported BPP: %zu", textureData.m_Bpp);
	}

	return Renderer::Backend::Format::UNDEFINED;
}

} // anonymous namespace

class CPredefinedTexture
{
public:
	const CTexturePtr& GetTexture()
	{
		return m_Texture;
	}

	void CreateTexture(
		std::unique_ptr<Renderer::Backend::ITexture> backendTexture,
		CTextureManagerImpl* textureManager)
	{
		Renderer::Backend::ITexture* fallback = backendTexture.get();
		CTextureProperties props(VfsPath{});
		m_Texture = CTexturePtr(new CTexture(
			std::move(backendTexture), fallback, props, textureManager));
		m_Texture->m_State = CTexture::UPLOADED;
		m_Texture->m_Self = m_Texture;
	}

private:
	CTexturePtr m_Texture;
};

class CSingleColorTexture final : public CPredefinedTexture
{
public:
	CSingleColorTexture(const CColor& color, Renderer::Backend::IDevice* device,
		CTextureManagerImpl* textureManager)
		: m_Color(color)
	{
		std::stringstream textureName;
		textureName << "SingleColorTexture (";
		textureName << "R:" << m_Color.r << ", ";
		textureName << "G:" << m_Color.g << ", ";
		textureName << "B:" << m_Color.b << ", ";
		textureName << "A:" << m_Color.a << ")";

		std::unique_ptr<Renderer::Backend::ITexture> backendTexture =
			device->CreateTexture2D(
				textureName.str().c_str(),
				Renderer::Backend::ITexture::Usage::TRANSFER_DST |
					Renderer::Backend::ITexture::Usage::SAMPLED,
				Renderer::Backend::Format::R8G8B8A8_UNORM,
				1, 1, Renderer::Backend::Sampler::MakeDefaultSampler(
					Renderer::Backend::Sampler::Filter::LINEAR,
					Renderer::Backend::Sampler::AddressMode::REPEAT));
		CreateTexture(std::move(backendTexture), textureManager);
	}

	void Upload(Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
	{
		if (!GetTexture() || !GetTexture()->GetBackendTexture())
			return;

		const SColor4ub color32 = m_Color.AsSColor4ub();
		// Construct 1x1 32-bit texture
		const u8 data[4] =
		{
			color32.R,
			color32.G,
			color32.B,
			color32.A
		};
		deviceCommandContext->UploadTexture(GetTexture()->GetBackendTexture(),
			Renderer::Backend::Format::R8G8B8A8_UNORM, data, std::size(data));
	}

private:
	CColor m_Color;
};

class CSingleColorTextureCube final : public CPredefinedTexture
{
public:
	CSingleColorTextureCube(const CColor& color, Renderer::Backend::IDevice* device,
		CTextureManagerImpl* textureManager)
		: m_Color(color)
	{
		std::stringstream textureName;
		textureName << "SingleColorTextureCube (";
		textureName << "R:" << m_Color.r << ", ";
		textureName << "G:" << m_Color.g << ", ";
		textureName << "B:" << m_Color.b << ", ";
		textureName << "A:" << m_Color.a << ")";

		std::unique_ptr<Renderer::Backend::ITexture> backendTexture =
			device->CreateTexture(
				textureName.str().c_str(), Renderer::Backend::ITexture::Type::TEXTURE_CUBE,
				Renderer::Backend::ITexture::Usage::TRANSFER_DST |
					Renderer::Backend::ITexture::Usage::SAMPLED,
				Renderer::Backend::Format::R8G8B8A8_UNORM,
				1, 1, Renderer::Backend::Sampler::MakeDefaultSampler(
					Renderer::Backend::Sampler::Filter::LINEAR,
					Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE), 1, 1);
		CreateTexture(std::move(backendTexture), textureManager);
	}

	void Upload(Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
	{
		if (!GetTexture() || !GetTexture()->GetBackendTexture())
			return;

		const SColor4ub color32 = m_Color.AsSColor4ub();
		// Construct 1x1 32-bit texture
		const u8 data[4] =
		{
			color32.R,
			color32.G,
			color32.B,
			color32.A
		};

		for (size_t face = 0; face < 6; ++face)
		{
			deviceCommandContext->UploadTexture(
				GetTexture()->GetBackendTexture(), Renderer::Backend::Format::R8G8B8A8_UNORM,
				data, std::size(data), 0, face);
		}
	}

private:
	CColor m_Color;
};

class CGradientTexture final : public CPredefinedTexture
{
public:
	static const uint32_t WIDTH = 256;
	static const uint32_t NUMBER_OF_LEVELS = 9;

	CGradientTexture(const CColor& colorFrom, const CColor& colorTo,
		Renderer::Backend::IDevice* device, CTextureManagerImpl* textureManager)
		: m_ColorFrom(colorFrom), m_ColorTo(colorTo)
	{
		std::stringstream textureName;
		textureName << "GradientTexture";
		textureName << " From (";
		textureName << "R:" << m_ColorFrom.r << ", ";
		textureName << "G:" << m_ColorFrom.g << ", ";
		textureName << "B:" << m_ColorFrom.b << ", ";
		textureName << "A:" << m_ColorFrom.a << ")";
		textureName << " To (";
		textureName << "R:" << m_ColorTo.r << ", ";
		textureName << "G:" << m_ColorTo.g << ", ";
		textureName << "B:" << m_ColorTo.b << ", ";
		textureName << "A:" << m_ColorTo.a << ")";

		std::unique_ptr<Renderer::Backend::ITexture> backendTexture =
			device->CreateTexture2D(
				textureName.str().c_str(),
				Renderer::Backend::ITexture::Usage::TRANSFER_DST |
					Renderer::Backend::ITexture::Usage::SAMPLED,
				Renderer::Backend::Format::R8G8B8A8_UNORM,
				WIDTH, 1, Renderer::Backend::Sampler::MakeDefaultSampler(
					Renderer::Backend::Sampler::Filter::LINEAR,
					Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE),
				NUMBER_OF_LEVELS);
		CreateTexture(std::move(backendTexture), textureManager);
	}

	void Upload(Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
	{
		if (!GetTexture() || !GetTexture()->GetBackendTexture())
			return;

		std::array<std::array<u8, 4>, WIDTH> data;
		for (uint32_t x = 0; x < WIDTH; ++x)
		{
			const float t = static_cast<float>(x) / (WIDTH - 1);
			const CColor color(
				Interpolate(m_ColorFrom.r, m_ColorTo.r, t),
				Interpolate(m_ColorFrom.g, m_ColorTo.g, t),
				Interpolate(m_ColorFrom.b, m_ColorTo.b, t),
				Interpolate(m_ColorFrom.a, m_ColorTo.a, t));
			const SColor4ub color32 = color.AsSColor4ub();
			data[x][0] = color32.R;
			data[x][1] = color32.G;
			data[x][2] = color32.B;
			data[x][3] = color32.A;
		}
		for (uint32_t level = 0; level < NUMBER_OF_LEVELS; ++level)
		{
			deviceCommandContext->UploadTexture(GetTexture()->GetBackendTexture(),
				Renderer::Backend::Format::R8G8B8A8_UNORM, data.data(), (WIDTH >> level) * data[0].size(), level);
			// Prepare data for the next level.
			const uint32_t nextLevelWidth = (WIDTH >> (level + 1));
			if (nextLevelWidth > 0)
			{
				for (uint32_t x = 0; x < nextLevelWidth; ++x)
					data[x] = data[(x << 1)];
				// Border values should be the same.
				data[nextLevelWidth - 1] = data[(WIDTH >> level) - 1];
			}
		}
	}

private:
	CColor m_ColorFrom, m_ColorTo;
};

struct TPhash
{
	std::size_t operator()(const CTextureProperties& textureProperties) const
	{
		std::size_t seed = 0;
		hash_combine(seed, m_PathHash(textureProperties.m_Path));
		hash_combine(seed, textureProperties.m_AddressModeU);
		hash_combine(seed, textureProperties.m_AddressModeV);
		hash_combine(seed, textureProperties.m_AnisotropicFilterEnabled);
		hash_combine(seed, textureProperties.m_FormatOverride);
		hash_combine(seed, textureProperties.m_IgnoreQuality);
		return seed;
	}

	std::size_t operator()(const CTexturePtr& texture) const
	{
		return this->operator()(texture->m_Properties);
	}

private:
	std::hash<Path> m_PathHash;
};

struct TPequal_to
{
	bool operator()(const CTextureProperties& lhs, const CTextureProperties& rhs) const
	{
		return
			lhs.m_Path == rhs.m_Path &&
			lhs.m_AddressModeU == rhs.m_AddressModeU &&
			lhs.m_AddressModeV == rhs.m_AddressModeV &&
			lhs.m_AnisotropicFilterEnabled == rhs.m_AnisotropicFilterEnabled &&
			lhs.m_FormatOverride == rhs.m_FormatOverride &&
			lhs.m_IgnoreQuality == rhs.m_IgnoreQuality;
	}

	bool operator()(const CTexturePtr& lhs, const CTexturePtr& rhs) const
	{
		return this->operator()(lhs->m_Properties, rhs->m_Properties);
	}
};

class CTextureManagerImpl
{
	friend class CTexture;
public:
	CTextureManagerImpl(PIVFS vfs, bool highQuality, Renderer::Backend::IDevice* device) :
		m_VFS(vfs), m_CacheLoader(vfs, L".dds"), m_Device(device),
		m_TextureConverter(vfs, highQuality),
		m_DefaultTexture(CColor(0.25f, 0.25f, 0.25f, 1.0f), device, this),
		m_ErrorTexture(CColor(1.0f, 0.0f, 1.0f, 1.0f), device, this),
		m_WhiteTexture(CColor(1.0f, 1.0f, 1.0f, 1.0f), device, this),
		m_TransparentTexture(CColor(0.0f, 0.0f, 0.0f, 0.0f), device, this),
		m_AlphaGradientTexture(
			CColor(1.0f, 1.0f, 1.0f, 0.0f), CColor(1.0f, 1.0f, 1.0f, 1.0f), device, this),
		m_BlackTextureCube(CColor(0.0f, 0.0f, 0.0f, 1.0f), device, this)
	{
		// Allow hotloading of textures
		RegisterFileReloadFunc(ReloadChangedFileCB, this);

		m_HasS3TC =
			m_Device->IsTextureFormatSupported(Renderer::Backend::Format::BC1_RGB_UNORM) &&
			m_Device->IsTextureFormatSupported(Renderer::Backend::Format::BC1_RGBA_UNORM) &&
			m_Device->IsTextureFormatSupported(Renderer::Backend::Format::BC2_UNORM) &&
			m_Device->IsTextureFormatSupported(Renderer::Backend::Format::BC3_UNORM);
	}

	~CTextureManagerImpl()
	{
		UnregisterFileReloadFunc(ReloadChangedFileCB, this);
	}

	const CTexturePtr& GetErrorTexture()
	{
		return m_ErrorTexture.GetTexture();
	}

	const CTexturePtr& GetWhiteTexture()
	{
		return m_WhiteTexture.GetTexture();
	}

	const CTexturePtr& GetTransparentTexture()
	{
		return m_TransparentTexture.GetTexture();
	}

	const CTexturePtr& GetAlphaGradientTexture()
	{
		return m_AlphaGradientTexture.GetTexture();
	}

	const CTexturePtr& GetBlackTextureCube()
	{
		return m_BlackTextureCube.GetTexture();
	}

	/**
	 * See CTextureManager::CreateTexture
	 */
	CTexturePtr CreateTexture(const CTextureProperties& props)
	{
		// Construct a new default texture with the given properties to use as the search key
		CTexturePtr texture(new CTexture(
			nullptr, m_DefaultTexture.GetTexture()->GetBackendTexture(), props, this));

		// Try to find an existing texture with the given properties
		TextureCache::iterator it = m_TextureCache.find(texture);
		if (it != m_TextureCache.end())
			return *it;

		// Can't find an existing texture - finish setting up this new texture
		texture->m_Self = texture;
		m_TextureCache.insert(texture);
		m_HotloadFiles[props.m_Path].insert(texture);

		return texture;
	}

	CTexturePtr WrapBackendTexture(
		std::unique_ptr<Renderer::Backend::ITexture> backendTexture)
	{
		ENSURE(backendTexture);
		Renderer::Backend::ITexture* fallback = backendTexture.get();

		CTextureProperties props(VfsPath{});
		CTexturePtr texture(new CTexture(
			std::move(backendTexture), fallback, props, this));
		texture->m_State = CTexture::UPLOADED;
		texture->m_Self = texture;
		return texture;
	}

	/**
	 * Load the given file into the texture object and upload it to OpenGL.
	 * Assumes the file already exists.
	 */
	void LoadTexture(const CTexturePtr& texture, const VfsPath& path)
	{
		PROFILE2("load texture");
		PROFILE2_ATTR("name: %ls", path.string().c_str());

		std::shared_ptr<u8> fileData;
		size_t fileSize;
		const Status loadStatus = m_VFS->LoadFile(path, fileData, fileSize);
		if (loadStatus != INFO::OK)
		{
			LOGERROR("Texture failed to load; \"%s\" %s",
				texture->m_Properties.m_Path.string8(), GetStatusAsString(loadStatus).c_str());
			texture->ResetBackendTexture(
				nullptr, m_ErrorTexture.GetTexture()->GetBackendTexture());
			texture->m_TextureData.reset();
			return;
		}

		texture->m_TextureData = std::make_unique<Tex>();
		Tex& textureData = *texture->m_TextureData;
		const Status decodeStatus = textureData.decode(fileData, fileSize);
		if (decodeStatus != INFO::OK)
		{
			LOGERROR("Texture failed to decode; \"%s\" %s",
				texture->m_Properties.m_Path.string8(), GetStatusAsString(decodeStatus).c_str());
			texture->ResetBackendTexture(
				nullptr, m_ErrorTexture.GetTexture()->GetBackendTexture());
			texture->m_TextureData.reset();
			return;
		}

		if (!is_pow2(textureData.m_Width) || !is_pow2(textureData.m_Height))
		{
			LOGERROR("Texture should have width and height be power of two; \"%s\" %zux%zu",
				texture->m_Properties.m_Path.string8(), textureData.m_Width, textureData.m_Height);
			texture->ResetBackendTexture(
				nullptr, m_ErrorTexture.GetTexture()->GetBackendTexture());
			texture->m_TextureData.reset();
			return;
		}

		// Initialise base color from the texture
		texture->m_BaseColor = textureData.get_average_color();

		Renderer::Backend::Format format = Renderer::Backend::Format::UNDEFINED;
		if (texture->m_Properties.m_FormatOverride != Renderer::Backend::Format::UNDEFINED)
		{
			format = texture->m_Properties.m_FormatOverride;
			// TODO: it'd be good to remove the override hack and provide information
			// via XML.
			ENSURE((textureData.m_Flags & TEX_DXT) == 0);
			if (format == Renderer::Backend::Format::A8_UNORM)
			{
				ENSURE(textureData.m_Bpp == 8 && (textureData.m_Flags & TEX_GREY));
			}
			else if (format == Renderer::Backend::Format::R8G8B8A8_UNORM)
			{
				ENSURE(textureData.m_Bpp == 32 && (textureData.m_Flags & TEX_ALPHA));
			}
			else
				debug_warn("Unsupported format override.");
		}
		else
		{
			format = ChooseFormatAndTransformTextureDataIfNeeded(m_Device, textureData, m_HasS3TC);
		}

		if (format == Renderer::Backend::Format::UNDEFINED)
		{
			LOGERROR("Texture failed to choose format; \"%s\"", texture->m_Properties.m_Path.string8());
			texture->ResetBackendTexture(
				nullptr, m_ErrorTexture.GetTexture()->GetBackendTexture());
			texture->m_TextureData.reset();
			return;
		}

		const uint32_t width = texture->m_TextureData->m_Width;
		const uint32_t height = texture->m_TextureData->m_Height ;
		const uint32_t MIPLevelCount = texture->m_TextureData->GetMIPLevels().size();
		texture->m_BaseLevelOffset = 0;

		Renderer::Backend::Sampler::Desc defaultSamplerDesc =
			Renderer::Backend::Sampler::MakeDefaultSampler(
				Renderer::Backend::Sampler::Filter::LINEAR,
				Renderer::Backend::Sampler::AddressMode::REPEAT);

		defaultSamplerDesc.addressModeU = texture->m_Properties.m_AddressModeU;
		defaultSamplerDesc.addressModeV = texture->m_Properties.m_AddressModeV;
		if (texture->m_Properties.m_AnisotropicFilterEnabled && m_Device->GetCapabilities().anisotropicFiltering)
		{
			int maxAnisotropy = 1;
			CFG_GET_VAL("textures.maxanisotropy", maxAnisotropy);
			const int allowedValues[] = {2, 4, 8, 16};
			if (std::find(std::begin(allowedValues), std::end(allowedValues), maxAnisotropy) != std::end(allowedValues))
			{
				defaultSamplerDesc.anisotropyEnabled = true;
				defaultSamplerDesc.maxAnisotropy = maxAnisotropy;
			}
		}

		if (!texture->m_Properties.m_IgnoreQuality)
		{
			int quality = 2;
			CFG_GET_VAL("textures.quality", quality);
			if (quality == 1)
			{
				if (MIPLevelCount > 1 && std::min(width, height) > 8)
					texture->m_BaseLevelOffset += 1;
			}
			else if (quality == 0)
			{
				if (MIPLevelCount > 2 && std::min(width, height) > 16)
					texture->m_BaseLevelOffset += 2;
				while (std::min(width >> texture->m_BaseLevelOffset, height >> texture->m_BaseLevelOffset) > 256 &&
					MIPLevelCount > texture->m_BaseLevelOffset + 1)
				{
					texture->m_BaseLevelOffset += 1;
				}
				defaultSamplerDesc.mipFilter = Renderer::Backend::Sampler::Filter::NEAREST;
				defaultSamplerDesc.anisotropyEnabled = false;
			}
		}

		texture->m_BackendTexture = m_Device->CreateTexture2D(
			texture->m_Properties.m_Path.string8().c_str(),
			Renderer::Backend::ITexture::Usage::TRANSFER_DST |
				Renderer::Backend::ITexture::Usage::SAMPLED,
			format, (width >> texture->m_BaseLevelOffset), (height >> texture->m_BaseLevelOffset),
			defaultSamplerDesc, MIPLevelCount - texture->m_BaseLevelOffset);
	}

	/**
	 * Set up some parameters for the loose cache filename code.
	 */
	void PrepareCacheKey(const CTexturePtr& texture, MD5& hash, u32& version)
	{
		// Hash the settings, so we won't use an old loose cache file if the
		// settings have changed
		CTextureConverter::Settings settings = GetConverterSettings(texture);
		settings.Hash(hash);

		// Arbitrary version number - change this if we update the code and
		// need to invalidate old users' caches
		version = 1;
	}

	/**
	 * Attempts to load a cached version of a texture.
	 * If the texture is loaded (or there was an error), returns true.
	 * Otherwise, returns false to indicate the caller should generate the cached version.
	 */
	bool TryLoadingCached(const CTexturePtr& texture)
	{
		MD5 hash;
		u32 version;
		PrepareCacheKey(texture, hash, version);

		VfsPath loadPath;
		Status ret = m_CacheLoader.TryLoadingCached(texture->m_Properties.m_Path, hash, version, loadPath);

		if (ret == INFO::OK)
		{
			// Found a cached texture - load it
			LoadTexture(texture, loadPath);
			return true;
		}
		else if (ret == INFO::SKIPPED)
		{
			// No cached version was found - we'll need to create it
			return false;
		}
		else
		{
			ENSURE(ret < 0);

			// No source file or archive cache was found, so we can't load the
			// real texture at all - return the error texture instead
			LOGERROR("CCacheLoader failed to find archived or source file for: \"%s\"", texture->m_Properties.m_Path.string8());
			texture->ResetBackendTexture(
				nullptr, m_ErrorTexture.GetTexture()->GetBackendTexture());
			return true;
		}
	}

	/**
	 * Initiates an asynchronous conversion process, from the texture's
	 * source file to the corresponding loose cache file.
	 */
	void ConvertTexture(const CTexturePtr& texture)
	{
		const VfsPath sourcePath = texture->m_Properties.m_Path;

		PROFILE2("convert texture");
		PROFILE2_ATTR("name: %ls", sourcePath.string().c_str());

		MD5 hash;
		u32 version;
		PrepareCacheKey(texture, hash, version);
		const VfsPath looseCachePath = m_CacheLoader.LooseCachePath(sourcePath, hash, version);

		CTextureConverter::Settings settings = GetConverterSettings(texture);

		m_TextureConverter.ConvertTexture(texture, sourcePath, looseCachePath, settings);
	}

	bool TextureExists(const VfsPath& path) const
	{
		return m_VFS->GetFileInfo(m_CacheLoader.ArchiveCachePath(path), 0) == INFO::OK ||
		       m_VFS->GetFileInfo(path, 0) == INFO::OK;
	}

	bool GenerateCachedTexture(const VfsPath& sourcePath, VfsPath& archiveCachePath)
	{
		archiveCachePath = m_CacheLoader.ArchiveCachePath(sourcePath);

		CTextureProperties textureProps(sourcePath);
		CTexturePtr texture = CreateTexture(textureProps);
		CTextureConverter::Settings settings = GetConverterSettings(texture);

		if (!m_TextureConverter.ConvertTexture(texture, sourcePath, VfsPath("cache") / archiveCachePath, settings))
			return false;

		while (true)
		{
			CTexturePtr textureOut;
			VfsPath dest;
			bool ok;
			if (m_TextureConverter.Poll(textureOut, dest, ok))
				return ok;

			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	VfsPath GetCachedPath(const VfsPath& path) const
	{
		return m_CacheLoader.ArchiveCachePath(path);
	}

	bool MakeProgress()
	{
		// Process any completed conversion tasks
		{
			CTexturePtr texture;
			VfsPath dest;
			bool ok;
			if (m_TextureConverter.Poll(texture, dest, ok))
			{
				if (ok)
				{
					LoadTexture(texture, dest);
				}
				else
				{
					LOGERROR("Texture failed to convert: \"%s\"", texture->m_Properties.m_Path.string8());
					texture->ResetBackendTexture(
						nullptr, m_ErrorTexture.GetTexture()->GetBackendTexture());
				}
				texture->m_State = CTexture::LOADED;
				return true;
			}
		}

		// We'll only push new conversion requests if it's not already busy
		bool converterBusy = m_TextureConverter.IsBusy();

		if (!converterBusy)
		{
			// Look for all high-priority textures needing conversion.
			// (Iterating over all textures isn't optimally efficient, but it
			// doesn't seem to be a problem yet and it's simpler than maintaining
			// multiple queues.)
			for (TextureCache::iterator it = m_TextureCache.begin(); it != m_TextureCache.end(); ++it)
			{
				if ((*it)->m_State == CTexture::HIGH_NEEDS_CONVERTING)
				{
					// Start converting this texture
					(*it)->m_State = CTexture::HIGH_IS_CONVERTING;
					ConvertTexture(*it);
					return true;
				}
			}
		}

		// Try loading prefetched textures from their cache
		for (TextureCache::iterator it = m_TextureCache.begin(); it != m_TextureCache.end(); ++it)
		{
			if ((*it)->m_State == CTexture::PREFETCH_NEEDS_LOADING)
			{
				if (TryLoadingCached(*it))
				{
					(*it)->m_State = CTexture::LOADED;
				}
				else
				{
					(*it)->m_State = CTexture::PREFETCH_NEEDS_CONVERTING;
				}
				return true;
			}
		}

		// If we've got nothing better to do, then start converting prefetched textures.
		if (!converterBusy)
		{
			for (TextureCache::iterator it = m_TextureCache.begin(); it != m_TextureCache.end(); ++it)
			{
				if ((*it)->m_State == CTexture::PREFETCH_NEEDS_CONVERTING)
				{
					(*it)->m_State = CTexture::PREFETCH_IS_CONVERTING;
					ConvertTexture(*it);
					return true;
				}
			}
		}

		return false;
	}

	bool MakeUploadProgress(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
	{
		if (!m_PredefinedTexturesUploaded)
		{
			m_DefaultTexture.Upload(deviceCommandContext);
			m_ErrorTexture.Upload(deviceCommandContext);
			m_WhiteTexture.Upload(deviceCommandContext);
			m_TransparentTexture.Upload(deviceCommandContext);
			m_AlphaGradientTexture.Upload(deviceCommandContext);
			m_BlackTextureCube.Upload(deviceCommandContext);
			m_PredefinedTexturesUploaded = true;
			return true;
		}
		return false;
	}

	/**
	 * Compute the conversion settings that apply to a given texture, by combining
	 * the textures.xml files from its directory and all parent directories
	 * (up to the VFS root).
	 */
	CTextureConverter::Settings GetConverterSettings(const CTexturePtr& texture)
	{
		fs::wpath srcPath = texture->m_Properties.m_Path.string();

		std::vector<CTextureConverter::SettingsFile*> files;
		VfsPath p;
		for (fs::wpath::iterator it = srcPath.begin(); it != srcPath.end(); ++it)
		{
			VfsPath settingsPath = p / "textures.xml";
			m_HotloadFiles[settingsPath].insert(texture);
			CTextureConverter::SettingsFile* f = GetSettingsFile(settingsPath);
			if (f)
				files.push_back(f);
			p = p / GetWstringFromWpath(*it);
		}
		return m_TextureConverter.ComputeSettings(GetWstringFromWpath(srcPath.filename()), files);
	}

	/**
	 * Return the (cached) settings file with the given filename,
	 * or NULL if it doesn't exist.
	 */
	CTextureConverter::SettingsFile* GetSettingsFile(const VfsPath& path)
	{
		SettingsFilesMap::iterator it = m_SettingsFiles.find(path);
		if (it != m_SettingsFiles.end())
			return it->second.get();

		if (m_VFS->GetFileInfo(path, NULL) >= 0)
		{
			std::shared_ptr<CTextureConverter::SettingsFile> settings(m_TextureConverter.LoadSettings(path));
			m_SettingsFiles.insert(std::make_pair(path, settings));
			return settings.get();
		}
		else
		{
			m_SettingsFiles.insert(std::make_pair(path, std::shared_ptr<CTextureConverter::SettingsFile>()));
			return NULL;
		}
	}

	static Status ReloadChangedFileCB(void* param, const VfsPath& path)
	{
		return static_cast<CTextureManagerImpl*>(param)->ReloadChangedFile(path);
	}

	Status ReloadChangedFile(const VfsPath& path)
	{
		// Uncache settings file, if this is one
		m_SettingsFiles.erase(path);

		// Find all textures using this file
		HotloadFilesMap::iterator files = m_HotloadFiles.find(path);
		if (files != m_HotloadFiles.end())
		{
			// Flag all textures using this file as needing reloading
			for (std::set<std::weak_ptr<CTexture>>::iterator it = files->second.begin(); it != files->second.end(); ++it)
			{
				if (std::shared_ptr<CTexture> texture = it->lock())
				{
					texture->m_State = CTexture::UNLOADED;
					texture->ResetBackendTexture(
						nullptr, m_DefaultTexture.GetTexture()->GetBackendTexture());
					texture->m_TextureData.reset();
				}
			}
		}

		return INFO::OK;
	}

	void ReloadAllTextures()
	{
		for (const CTexturePtr& texture : m_TextureCache)
		{
			texture->m_State = CTexture::UNLOADED;
			texture->ResetBackendTexture(
				nullptr, m_DefaultTexture.GetTexture()->GetBackendTexture());
			texture->m_TextureData.reset();
		}
	}

	size_t GetBytesUploaded() const
	{
		size_t size = 0;
		for (TextureCache::const_iterator it = m_TextureCache.begin(); it != m_TextureCache.end(); ++it)
			size += (*it)->GetUploadedSize();
		return size;
	}

	void OnQualityChanged()
	{
		ReloadAllTextures();
	}

private:
	PIVFS m_VFS;
	CCacheLoader m_CacheLoader;
	Renderer::Backend::IDevice* m_Device = nullptr;
	CTextureConverter m_TextureConverter;

	CSingleColorTexture m_DefaultTexture;
	CSingleColorTexture m_ErrorTexture;
	CSingleColorTexture m_WhiteTexture;
	CSingleColorTexture m_TransparentTexture;
	CGradientTexture m_AlphaGradientTexture;
	CSingleColorTextureCube m_BlackTextureCube;
	bool m_PredefinedTexturesUploaded = false;

	// Cache of all loaded textures
	using TextureCache =
		std::unordered_set<CTexturePtr, TPhash, TPequal_to>;
	TextureCache m_TextureCache;
	// TODO: we ought to expire unused textures from the cache eventually

	// Store the set of textures that need to be reloaded when the given file
	// (a source file or settings.xml) is modified
	using HotloadFilesMap =
		std::unordered_map<VfsPath, std::set<std::weak_ptr<CTexture>, std::owner_less<std::weak_ptr<CTexture>>>>;
	HotloadFilesMap m_HotloadFiles;

	// Cache for the conversion settings files
	using SettingsFilesMap =
		std::unordered_map<VfsPath, std::shared_ptr<CTextureConverter::SettingsFile>>;
	SettingsFilesMap m_SettingsFiles;

	bool m_HasS3TC = false;
};

CTexture::CTexture(
	std::unique_ptr<Renderer::Backend::ITexture> texture,
	Renderer::Backend::ITexture* fallback,
	const CTextureProperties& props, CTextureManagerImpl* textureManager) :
	m_BackendTexture(std::move(texture)), m_FallbackBackendTexture(fallback),
	m_BaseColor(0), m_State(UNLOADED), m_Properties(props),
	m_TextureManager(textureManager)
{
}

CTexture::~CTexture() = default;

void CTexture::UploadBackendTextureIfNeeded(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
{
	if (IsUploaded())
		return;

	if (!IsLoaded())
		TryLoad();

	if (!IsLoaded())
		return;
	else if (!m_TextureData || !m_BackendTexture)
	{
		ResetBackendTexture(nullptr, m_TextureManager->GetErrorTexture()->GetBackendTexture());
		m_State = UPLOADED;
		return;
	}

	m_UploadedSize = 0;
	for (uint32_t textureDataLevel = m_BaseLevelOffset, level = 0; textureDataLevel < m_TextureData->GetMIPLevels().size(); ++textureDataLevel)
	{
		const Tex::MIPLevel& levelData = m_TextureData->GetMIPLevels()[textureDataLevel];
		deviceCommandContext->UploadTexture(m_BackendTexture.get(), m_BackendTexture->GetFormat(),
			levelData.data, levelData.dataSize, level++);
		m_UploadedSize += levelData.dataSize;
	}
	m_TextureData.reset();

	m_State = UPLOADED;
}

Renderer::Backend::ITexture* CTexture::GetBackendTexture()
{
	return m_BackendTexture && IsUploaded() ? m_BackendTexture.get() : m_FallbackBackendTexture;
}

const Renderer::Backend::ITexture* CTexture::GetBackendTexture() const
{
	return m_BackendTexture && IsUploaded() ? m_BackendTexture.get() : m_FallbackBackendTexture;
}

bool CTexture::TryLoad()
{
	// If we haven't started loading, then try loading, and if that fails then request conversion.
	// If we have already tried prefetch loading, and it failed, bump the conversion request to HIGH priority.
	if (m_State == UNLOADED || m_State == PREFETCH_NEEDS_LOADING || m_State == PREFETCH_NEEDS_CONVERTING)
	{
		if (std::shared_ptr<CTexture> self = m_Self.lock())
		{
			if (m_State != PREFETCH_NEEDS_CONVERTING && m_TextureManager->TryLoadingCached(self))
				m_State = LOADED;
			else
				m_State = HIGH_NEEDS_CONVERTING;
		}
	}

	return IsLoaded() || IsUploaded();
}

void CTexture::Prefetch()
{
	if (m_State == UNLOADED)
	{
		if (std::shared_ptr<CTexture> self = m_Self.lock())
		{
			m_State = PREFETCH_NEEDS_LOADING;
		}
	}
}

void CTexture::ResetBackendTexture(
	std::unique_ptr<Renderer::Backend::ITexture> backendTexture,
	Renderer::Backend::ITexture* fallbackBackendTexture)
{
	m_BackendTexture = std::move(backendTexture);
	m_FallbackBackendTexture = fallbackBackendTexture;
}

size_t CTexture::GetWidth() const
{
	return GetBackendTexture()->GetWidth();
}

size_t CTexture::GetHeight() const
{
	return GetBackendTexture()->GetHeight();
}

bool CTexture::HasAlpha() const
{
	const Renderer::Backend::Format format = GetBackendTexture()->GetFormat();
	return
		format == Renderer::Backend::Format::A8_UNORM ||
		format == Renderer::Backend::Format::R8G8B8A8_UNORM ||
		format == Renderer::Backend::Format::BC1_RGBA_UNORM ||
		format == Renderer::Backend::Format::BC2_UNORM ||
		format == Renderer::Backend::Format::BC3_UNORM;
}

u32 CTexture::GetBaseColor() const
{
	return m_BaseColor;
}

size_t CTexture::GetUploadedSize() const
{
	return m_UploadedSize;
}

// CTextureManager: forward all calls to impl:

CTextureManager::CTextureManager(PIVFS vfs, bool highQuality, Renderer::Backend::IDevice* device) :
	m(new CTextureManagerImpl(vfs, highQuality, device))
{
}

CTextureManager::~CTextureManager()
{
	delete m;
}

CTexturePtr CTextureManager::CreateTexture(const CTextureProperties& props)
{
	return m->CreateTexture(props);
}

CTexturePtr CTextureManager::WrapBackendTexture(
	std::unique_ptr<Renderer::Backend::ITexture> backendTexture)
{
	return m->WrapBackendTexture(std::move(backendTexture));
}

bool CTextureManager::TextureExists(const VfsPath& path) const
{
	return m->TextureExists(path);
}

const CTexturePtr& CTextureManager::GetErrorTexture()
{
	return m->GetErrorTexture();
}

const CTexturePtr& CTextureManager::GetWhiteTexture()
{
	return m->GetWhiteTexture();
}

const CTexturePtr& CTextureManager::GetTransparentTexture()
{
	return m->GetTransparentTexture();
}

const CTexturePtr& CTextureManager::GetAlphaGradientTexture()
{
	return m->GetAlphaGradientTexture();
}

const CTexturePtr& CTextureManager::GetBlackTextureCube()
{
	return m->GetBlackTextureCube();
}

bool CTextureManager::MakeProgress()
{
	return m->MakeProgress();
}

bool CTextureManager::MakeUploadProgress(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
{
	return m->MakeUploadProgress(deviceCommandContext);
}

bool CTextureManager::GenerateCachedTexture(const VfsPath& path, VfsPath& outputPath)
{
	return m->GenerateCachedTexture(path, outputPath);
}

VfsPath CTextureManager::GetCachedPath(const VfsPath& path) const
{
	return m->GetCachedPath(path);
}

size_t CTextureManager::GetBytesUploaded() const
{
	return m->GetBytesUploaded();
}

void CTextureManager::OnQualityChanged()
{
	m->OnQualityChanged();
}
