/* Copyright (C) 2021 Wildfire Games.
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
#include "lib/file/vfs/vfs_tree.h"
#include "lib/hash.h"
#include "lib/res/graphics/ogl_tex.h"
#include "lib/res/h_mgr.h"
#include "lib/timer.h"
#include "maths/MD5.h"
#include "ps/CacheLoader.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"

#include <boost/filesystem.hpp>
#include <iomanip>
#include <set>
#include <unordered_map>
#include <unordered_set>

class SingleColorTexture
{
public:
	SingleColorTexture(const CColor& color, PIVFS vfs, const VfsPath& pathPlaceholder, const bool disableGL, CTextureManagerImpl* textureManager)
	{
		const SColor4ub color32 = color.AsSColor4ub();
		// Construct 1x1 32-bit texture
		std::shared_ptr<u8> data(new u8[3], ArrayDeleter());
		data.get()[0] = color32.R;
		data.get()[1] = color32.G;
		data.get()[2] = color32.B;
		data.get()[3] = color32.A;

		Tex t;
		ignore_result(t.wrap(1, 1, 32, TEX_ALPHA, data, 0));

		m_Handle = ogl_tex_wrap(&t, vfs, pathPlaceholder);
		ignore_result(ogl_tex_set_filter(m_Handle, GL_LINEAR));
		if (!disableGL)
			ignore_result(ogl_tex_upload(m_Handle));

		CTextureProperties props(pathPlaceholder);
		m_Texture = CTexturePtr(new CTexture(m_Handle, props, textureManager));
		m_Texture->m_State = CTexture::LOADED;
		m_Texture->m_Self = m_Texture;
	}

	~SingleColorTexture()
	{
		ignore_result(ogl_tex_free(m_Handle));
	}

	CTexturePtr GetTexture()
	{
		return m_Texture;
	}

	Handle GetHandle()
	{
		return m_Handle;
	}

private:
	Handle m_Handle;
	CTexturePtr m_Texture;
};

struct TPhash
{
	std::size_t operator()(CTextureProperties const& a) const
	{
		std::size_t seed = 0;
		hash_combine(seed, m_PathHash(a.m_Path));
		hash_combine(seed, a.m_Filter);
		hash_combine(seed, a.m_WrapS);
		hash_combine(seed, a.m_WrapT);
		hash_combine(seed, a.m_Aniso);
		hash_combine(seed, a.m_Format);
		return seed;
	}

	std::size_t operator()(CTexturePtr const& a) const
	{
		return (*this)(a->m_Properties);
	}

private:
	std::hash<Path> m_PathHash;
};

struct TPequal_to
{
	bool operator()(CTextureProperties const& a, CTextureProperties const& b) const
	{
		return a.m_Path == b.m_Path && a.m_Filter == b.m_Filter
			&& a.m_WrapS == b.m_WrapS && a.m_WrapT == b.m_WrapT
			&& a.m_Aniso == b.m_Aniso && a.m_Format == b.m_Format;
	}
	bool operator()(CTexturePtr const& a, CTexturePtr const& b) const
	{
		return (*this)(a->m_Properties, b->m_Properties);
	}
};

class CTextureManagerImpl
{
	friend class CTexture;
public:
	CTextureManagerImpl(PIVFS vfs, bool highQuality, bool disableGL) :
		m_VFS(vfs), m_CacheLoader(vfs, L".dds"), m_DisableGL(disableGL),
		m_TextureConverter(vfs, highQuality), m_DefaultHandle(0),
		m_ErrorTexture(CColor(1.0f, 0.0f, 1.0f, 1.0f), vfs, L"(error texture)", disableGL, this),
		m_WhiteTexture(CColor(1.0f, 1.0f, 1.0f, 1.0f), vfs, L"(white texture)", disableGL, this),
		m_TransparentTexture(CColor(0.0f, 0.0f, 0.0f, 0.0f), vfs, L"(transparent texture)", disableGL, this)
	{
		// Initialise some textures that will always be available,
		// without needing to load any files

		// Default placeholder texture (grey)
		if (!m_DisableGL)
		{
			// Construct 1x1 24-bit texture
			std::shared_ptr<u8> data(new u8[3], ArrayDeleter());
			data.get()[0] = 64;
			data.get()[1] = 64;
			data.get()[2] = 64;
			Tex t;
			ignore_result(t.wrap(1, 1, 24, 0, data, 0));

			m_DefaultHandle = ogl_tex_wrap(&t, m_VFS, L"(default texture)");
			ignore_result(ogl_tex_set_filter(m_DefaultHandle, GL_LINEAR));
			if (!m_DisableGL)
				ignore_result(ogl_tex_upload(m_DefaultHandle));
		}

		// Allow hotloading of textures
		RegisterFileReloadFunc(ReloadChangedFileCB, this);
	}

	~CTextureManagerImpl()
	{
		UnregisterFileReloadFunc(ReloadChangedFileCB, this);

		ignore_result(ogl_tex_free(m_DefaultHandle));
	}

	CTexturePtr GetErrorTexture()
	{
		return m_ErrorTexture.GetTexture();
	}

	CTexturePtr GetWhiteTexture()
	{
		return m_WhiteTexture.GetTexture();
	}

	CTexturePtr GetTransparentTexture()
	{
		return m_TransparentTexture.GetTexture();
	}

	/**
	 * See CTextureManager::CreateTexture
	 */
	CTexturePtr CreateTexture(const CTextureProperties& props)
	{
		// Construct a new default texture with the given properties to use as the search key
		CTexturePtr texture(new CTexture(m_DefaultHandle, props, this));

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

	/**
	 * Load the given file into the texture object and upload it to OpenGL.
	 * Assumes the file already exists.
	 */
	void LoadTexture(const CTexturePtr& texture, const VfsPath& path)
	{
		if (m_DisableGL)
			return;

		PROFILE2("load texture");
		PROFILE2_ATTR("name: %ls", path.string().c_str());

		Handle h = ogl_tex_load(m_VFS, path, RES_UNIQUE);
		if (h <= 0)
		{
			LOGERROR("Texture failed to load; \"%s\"", texture->m_Properties.m_Path.string8());

			// Replace with error texture to make it obvious
			texture->SetHandle(m_ErrorTexture.GetHandle());
			return;
		}

		// Get some flags for later use
		size_t flags = 0;
		ignore_result(ogl_tex_get_format(h, &flags, NULL));

		// Initialise base color from the texture
		ignore_result(ogl_tex_get_average_color(h, &texture->m_BaseColor));

		// Set GL upload properties
		ignore_result(ogl_tex_set_wrap(h, texture->m_Properties.m_WrapS, texture->m_Properties.m_WrapT));
		ignore_result(ogl_tex_set_anisotropy(h, texture->m_Properties.m_Aniso));

		// Prevent ogl_tex automatically generating mipmaps (which is slow and unwanted),
		// by avoiding mipmapped filters unless the source texture already has mipmaps
		GLint filter = texture->m_Properties.m_Filter;
		if (!(flags & TEX_MIPMAPS))
		{
			switch (filter)
			{
			case GL_NEAREST_MIPMAP_NEAREST:
			case GL_NEAREST_MIPMAP_LINEAR:
				filter = GL_NEAREST;
				break;
			case GL_LINEAR_MIPMAP_NEAREST:
			case GL_LINEAR_MIPMAP_LINEAR:
				filter = GL_LINEAR;
				break;
			}
		}
		ignore_result(ogl_tex_set_filter(h, filter));

		// Upload to GL
		if (!m_DisableGL && ogl_tex_upload(h, texture->m_Properties.m_Format) < 0)
		{
			LOGERROR("Texture failed to upload: \"%s\"", texture->m_Properties.m_Path.string8());

			ogl_tex_free(h);

			// Replace with error texture to make it obvious
			texture->SetHandle(m_ErrorTexture.GetHandle());
			return;
		}

		// Let the texture object take ownership of this handle
		texture->SetHandle(h, true);
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
			texture->SetHandle(m_ErrorTexture.GetHandle());
			return true;
		}
	}

	/**
	 * Initiates an asynchronous conversion process, from the texture's
	 * source file to the corresponding loose cache file.
	 */
	void ConvertTexture(const CTexturePtr& texture)
	{
		VfsPath sourcePath = texture->m_Properties.m_Path;

		PROFILE2("convert texture");
		PROFILE2_ATTR("name: %ls", sourcePath.string().c_str());

		MD5 hash;
		u32 version;
		PrepareCacheKey(texture, hash, version);
		VfsPath looseCachePath = m_CacheLoader.LooseCachePath(sourcePath, hash, version);

//		LOGWARNING("Converting texture \"%s\"", srcPath.c_str());

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
					texture->SetHandle(m_ErrorTexture.GetHandle());
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
		return m_TextureConverter.ComputeSettings(GetWstringFromWpath(srcPath.leaf()), files);
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
			for (std::set<std::weak_ptr<CTexture> >::iterator it = files->second.begin(); it != files->second.end(); ++it)
			{
				if (std::shared_ptr<CTexture> texture = it->lock())
				{
					texture->m_State = CTexture::UNLOADED;
					texture->SetHandle(m_DefaultHandle);
				}
			}
		}

		return INFO::OK;
	}

	size_t GetBytesUploaded() const
	{
		size_t size = 0;
		for (TextureCache::const_iterator it = m_TextureCache.begin(); it != m_TextureCache.end(); ++it)
			size += (*it)->GetUploadedSize();
		return size;
	}

private:
	PIVFS m_VFS;
	CCacheLoader m_CacheLoader;
	bool m_DisableGL;
	CTextureConverter m_TextureConverter;

	Handle m_DefaultHandle;
	SingleColorTexture m_ErrorTexture;
	SingleColorTexture m_WhiteTexture;
	SingleColorTexture m_TransparentTexture;

	// Cache of all loaded textures
	using TextureCache =
		std::unordered_set<CTexturePtr, TPhash, TPequal_to>;
	TextureCache m_TextureCache;
	// TODO: we ought to expire unused textures from the cache eventually

	// Store the set of textures that need to be reloaded when the given file
	// (a source file or settings.xml) is modified
	using HotloadFilesMap =
		std::unordered_map<VfsPath, std::set<std::weak_ptr<CTexture>, std::owner_less<std::weak_ptr<CTexture> > > >;
	HotloadFilesMap m_HotloadFiles;

	// Cache for the conversion settings files
	using SettingsFilesMap =
		std::unordered_map<VfsPath, std::shared_ptr<CTextureConverter::SettingsFile>>;
	SettingsFilesMap m_SettingsFiles;
};

CTexture::CTexture(Handle handle, const CTextureProperties& props, CTextureManagerImpl* textureManager) :
	m_Handle(handle), m_BaseColor(0), m_State(UNLOADED), m_Properties(props), m_TextureManager(textureManager)
{
	// Add a reference to the handle (it might be shared by multiple CTextures
	// so we can't take ownership of it)
	if (m_Handle)
		h_add_ref(m_Handle);
}

CTexture::~CTexture()
{
	if (m_Handle)
		ogl_tex_free(m_Handle);
}

void CTexture::Bind(size_t unit)
{
	ogl_tex_bind(GetHandle(), unit);
}

Handle CTexture::GetHandle()
{
	// TODO: TryLoad might call ogl_tex_upload which enables GL_TEXTURE_2D
	// on texture unit 0, regardless of 'unit', which callers might
	// not be expecting. Ideally that wouldn't happen.

	TryLoad();

	return m_Handle;
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

	return (m_State == LOADED);
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

bool CTexture::IsLoaded()
{
	return (m_State == LOADED);
}

void CTexture::SetHandle(Handle handle, bool takeOwnership)
{
	if (handle == m_Handle)
		return;

	if (!takeOwnership)
		h_add_ref(handle);

	ogl_tex_free(m_Handle);
	m_Handle = handle;
}

size_t CTexture::GetWidth() const
{
	size_t w = 0;
	ignore_result(ogl_tex_get_size(m_Handle, &w, 0, 0));
	return w;
}

size_t CTexture::GetHeight() const
{
	size_t h = 0;
	ignore_result(ogl_tex_get_size(m_Handle, 0, &h, 0));
	return h;
}

bool CTexture::HasAlpha() const
{
	size_t flags = 0;
	ignore_result(ogl_tex_get_format(m_Handle, &flags, 0));
	return (flags & TEX_ALPHA) != 0;
}

u32 CTexture::GetBaseColor() const
{
	return m_BaseColor;
}

size_t CTexture::GetUploadedSize() const
{
	size_t size = 0;
	ignore_result(ogl_tex_get_uploaded_size(m_Handle, &size));
	return size;
}


// CTextureManager: forward all calls to impl:

CTextureManager::CTextureManager(PIVFS vfs, bool highQuality, bool disableGL) :
	m(new CTextureManagerImpl(vfs, highQuality, disableGL))
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

bool CTextureManager::TextureExists(const VfsPath& path) const
{
	return m->TextureExists(path);
}

CTexturePtr CTextureManager::GetErrorTexture()
{
	return m->GetErrorTexture();
}

CTexturePtr CTextureManager::GetWhiteTexture()
{
	return m->GetWhiteTexture();
}

CTexturePtr CTextureManager::GetTransparentTexture()
{
	return m->GetTransparentTexture();
}

bool CTextureManager::MakeProgress()
{
	return m->MakeProgress();
}

bool CTextureManager::GenerateCachedTexture(const VfsPath& path, VfsPath& outputPath)
{
	return m->GenerateCachedTexture(path, outputPath);
}

size_t CTextureManager::GetBytesUploaded() const
{
	return m->GetBytesUploaded();
}
