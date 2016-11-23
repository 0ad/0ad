/* Copyright (C) 2015 Wildfire Games.
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

#include "ColladaManager.h"

#include <boost/algorithm/string.hpp>

#include "graphics/ModelDef.h"
#include "lib/fnv_hash.h"
#include "maths/MD5.h"
#include "ps/CacheLoader.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/DllLoader.h"
#include "ps/Filesystem.h"

namespace Collada
{
	#include "collada/DLL.h"
}

namespace
{
	void ColladaLog(void* cb_data, int severity, const char* text)
	{
		VfsPath* path = static_cast<VfsPath*>(cb_data);

		if (severity == LOG_INFO)
			LOGMESSAGE("%s: %s", path->string8(), text);
		else if (severity == LOG_WARNING)
			LOGWARNING("%s: %s", path->string8(), text);
		else
			LOGERROR("%s: %s", path->string8(), text);
	}

	void ColladaOutput(void* cb_data, const char* data, unsigned int length)
	{
		WriteBuffer* writeBuffer = static_cast<WriteBuffer*>(cb_data);
		writeBuffer->Append(data, (size_t)length);
	}
}

class CColladaManagerImpl
{
	DllLoader dll;

	void (*set_logger)(Collada::LogFn logger, void* cb_data);
	int (*set_skeleton_definitions)(const char* xml, int length);
	int (*convert_dae_to_pmd)(const char* dae, Collada::OutputFn pmd_writer, void* cb_data);
	int (*convert_dae_to_psa)(const char* dae, Collada::OutputFn psa_writer, void* cb_data);

public:
	CColladaManagerImpl(const PIVFS& vfs)
		: dll("Collada"), m_VFS(vfs), m_skeletonHashInvalidated(true)
	{
		// Support hotloading
		RegisterFileReloadFunc(ReloadChangedFileCB, this);
	}

	~CColladaManagerImpl()
	{
		if (dll.IsLoaded())
			set_logger(NULL, NULL); // unregister the log handler
		UnregisterFileReloadFunc(ReloadChangedFileCB, this);
	}

	Status ReloadChangedFile(const VfsPath& path)
	{
		// Ignore files that aren't in the right path
		if (!boost::algorithm::starts_with(path.string(), L"art/skeletons/"))
			return INFO::OK;

		if (path.Extension() != L".xml")
			return INFO::OK;

		m_skeletonHashInvalidated = true;

		// If the file doesn't exist (e.g. it was deleted), don't bother reloading
		// or 'unloading' since that isn't possible
		if (!VfsFileExists(path))
			return INFO::OK;

		if (!dll.IsLoaded() && !TryLoadDLL())
			return ERR::FAIL;

		LOGMESSAGE("Hotloading skeleton definitions from '%s'", path.string8());
		// Set the filename for the logger to report
		set_logger(ColladaLog, const_cast<void*>(static_cast<const void*>(&path)));

		CVFSFile skeletonFile;
		if (skeletonFile.Load(m_VFS, path) != PSRETURN_OK)
		{
			LOGERROR("Failed to read skeleton defintions from '%s'", path.string8());
			return ERR::FAIL;
		}

		int ok = set_skeleton_definitions((const char*)skeletonFile.GetBuffer(), (int)skeletonFile.GetBufferSize());
		if (ok < 0)
		{
			LOGERROR("Failed to load skeleton definitions from '%s'", path.string8());
			return ERR::FAIL;
		}

		return INFO::OK;
	}

	static Status ReloadChangedFileCB(void* param, const VfsPath& path)
	{
		return static_cast<CColladaManagerImpl*>(param)->ReloadChangedFile(path);
	}

	bool Convert(const VfsPath& daeFilename, const VfsPath& pmdFilename, CColladaManager::FileType type)
	{
		// To avoid always loading the DLL when it's usually not going to be
		// used (and to do the same on Linux where delay-loading won't help),
		// and to avoid compile-time dependencies (because it's a minor pain
		// to get all the right libraries to build the COLLADA DLL), we load
		// it dynamically when it is required, instead of using the exported
		// functions and binding at link-time.
		if (!dll.IsLoaded())
		{
			if (!TryLoadDLL())
				return false;

			if (!LoadSkeletonDefinitions())
			{
				dll.Unload(); // Error should have been logged already
				return false;
			}
		}

		// Set the filename for the logger to report
		set_logger(ColladaLog, const_cast<void*>(static_cast<const void*>(&daeFilename)));

		// We need to null-terminate the buffer, so do it (possibly inefficiently)
		// by converting to a CStr
		CStr daeData;
		{
			CVFSFile daeFile;
			if (daeFile.Load(m_VFS, daeFilename) != PSRETURN_OK)
				return false;
			daeData = daeFile.GetAsString();
		}

		// Do the conversion into a memory buffer
		// We need to check the result, as archive builder needs to know if the source dae
		//	was sucessfully converted to .pmd/psa
		int result = -1;
		WriteBuffer writeBuffer;
		switch (type)
		{
		case CColladaManager::PMD:
			result = convert_dae_to_pmd(daeData.c_str(), ColladaOutput, &writeBuffer);
			break;
		case CColladaManager::PSA:
			result = convert_dae_to_psa(daeData.c_str(), ColladaOutput, &writeBuffer);
			break;
		}

		// don't create zero-length files (as happens in test_invalid_dae when
		// we deliberately pass invalid XML data) because the VFS caching
		// logic warns when asked to load such.
		if (writeBuffer.Size())
		{
			Status ret = m_VFS->CreateFile(pmdFilename, writeBuffer.Data(), writeBuffer.Size());
			ENSURE(ret == INFO::OK);
		}

		return (result == 0);
	}

	bool TryLoadDLL()
	{
		if (!dll.LoadDLL())
		{
			LOGERROR("Failed to load COLLADA conversion DLL");
			return false;
		}

		try
		{
			dll.LoadSymbol("set_logger", set_logger);
			dll.LoadSymbol("set_skeleton_definitions", set_skeleton_definitions);
			dll.LoadSymbol("convert_dae_to_pmd", convert_dae_to_pmd);
			dll.LoadSymbol("convert_dae_to_psa", convert_dae_to_psa);
		}
		catch (PSERROR_DllLoader&)
		{
			LOGERROR("Failed to load symbols from COLLADA conversion DLL");
			dll.Unload();
			return false;
		}
		return true;
	}

	bool LoadSkeletonDefinitions()
	{
		VfsPaths pathnames;
		if (vfs::GetPathnames(m_VFS, L"art/skeletons/", L"*.xml", pathnames) < 0)
		{
			LOGERROR("No skeleton definition files present");
			return false;
		}

		bool loaded = false;
		for (const VfsPath& path : pathnames)
		{
			LOGMESSAGE("Loading skeleton definitions from '%s'", path.string8());
			// Set the filename for the logger to report
			set_logger(ColladaLog, const_cast<void*>(static_cast<const void*>(&path)));

			CVFSFile skeletonFile;
			if (skeletonFile.Load(m_VFS, path) != PSRETURN_OK)
			{
				LOGERROR("Failed to read skeleton defintions from '%s'", path.string8());
				continue;
			}

			int ok = set_skeleton_definitions((const char*)skeletonFile.GetBuffer(), (int)skeletonFile.GetBufferSize());
			if (ok < 0)
			{
				LOGERROR("Failed to load skeleton definitions from '%s'", path.string8());
				continue;
			}

			loaded = true;
		}

		if (!loaded)
			LOGERROR("Failed to load any skeleton definitions");

		return loaded;
	}

	/**
	 * Creates MD5 hash key from skeletons.xml info and COLLADA converter version,
	 * used to invalidate cached .pmd/psas
	 *
	 * @param[out] hash resulting MD5 hash
	 * @param[out] version version passed to CCacheLoader, used if code change should force
	 *		  cache invalidation
	 */
	void PrepareCacheKey(MD5& hash, u32& version)
	{
		// Add converter version to the hash
		version = COLLADA_CONVERTER_VERSION;

		// Cache the skeleton files hash data
		if (m_skeletonHashInvalidated)
		{
			VfsPaths paths;
			if (vfs::GetPathnames(m_VFS, L"art/skeletons/", L"*.xml", paths) != INFO::OK)
			{
				LOGWARNING("Failed to load skeleton definitions");
				return;
			}

			// Sort the paths to not invalidate the cache if mods are mounted in different order
			// (No need to stable_sort as the VFS gurantees that we have no duplicates)
			std::sort(paths.begin(), paths.end());

			// We need two u64s per file
			m_skeletonHashes.clear();
			m_skeletonHashes.reserve(paths.size()*2);

			CFileInfo fileInfo;
			for (const VfsPath& path : paths)
			{
				// This will cause an assertion failure if *it doesn't exist,
				//	because fileinfo is not a NULL pointer, which is annoying but that
				//	should never happen, unless there really is a problem
				if (m_VFS->GetFileInfo(path, &fileInfo) != INFO::OK)
				{
					LOGERROR("Failed to stat '%s' for DAE caching", path.string8());
				}
				else
				{
					m_skeletonHashes.push_back((u64)fileInfo.MTime() & ~1); //skip lowest bit, since zip and FAT don't preserve it
					m_skeletonHashes.push_back((u64)fileInfo.Size());
				}
			}

			// Check if we were able to load any skeleton files
			if (m_skeletonHashes.empty())
				LOGERROR("Failed to stat any skeleton definitions for DAE caching");
				// We can continue, something else will break if we try loading a skeletal model

			m_skeletonHashInvalidated = false;
		}

		for (const u64& h : m_skeletonHashes)
			hash.Update((const u8*)&h, sizeof(h));
	}

private:
	PIVFS m_VFS;
	bool m_skeletonHashInvalidated;
	std::vector<u64> m_skeletonHashes;
};

CColladaManager::CColladaManager(const PIVFS& vfs)
: m(new CColladaManagerImpl(vfs)), m_VFS(vfs)
{
}

CColladaManager::~CColladaManager()
{
	delete m;
}

VfsPath CColladaManager::GetLoadablePath(const VfsPath& pathnameNoExtension, FileType type)
{
	std::wstring extn;
	switch (type)
	{
	case PMD: extn = L".pmd"; break;
	case PSA: extn = L".psa"; break;
		// no other alternatives
	}

	/*

	Algorithm:
	* Calculate hash of skeletons.xml and converter version.
	* Use CCacheLoader to check for archived or loose cached .pmd/psa.
	* If cached version exists:
		* Return pathname of cached .pmd/psa.
	* Else, if source .dae for this model exists:
		* Convert it to cached .pmd/psa.
		* If converter succeeded:
			* Return pathname of cached .pmd/psa.
		* Else, fail (return empty path).
	* Else, if uncached .pmd/psa exists:
		* Return pathname of uncached .pmd/psa.
	* Else, fail (return empty path).

	Since we use CCacheLoader which automatically hashes file size and mtime,
	and handles archived files and loose cache, when preparing the cache key
	we add converter version number (so updates of the converter cause
	regeneration of the .pmd/psa) and the global skeletons.xml file size and
	mtime, as modelers frequently change the contents of skeletons.xml and get
	perplexed if the in-game models haven't updated as expected (we don't know
	which models were affected by the skeletons.xml change, if any, so we just
	regenerate all of them)

	TODO (maybe): The .dae -> .pmd/psa conversion may fail (e.g. if the .dae is
	invalid or unsupported), but it may take a long time to start the conversion
	then realise it's not going to work. That will delay the loading of the game
	every time, which is annoying, so maybe it should cache the error message
	until the .dae is updated and fixed. (Alternatively, avoid having that many
	broken .daes in the game.)

	*/

	// Now we're looking for cached files
	CCacheLoader cacheLoader(m_VFS, extn);
	MD5 hash;
	u32 version;
	m->PrepareCacheKey(hash, version);

	VfsPath cachePath;
	VfsPath sourcePath = pathnameNoExtension.ChangeExtension(L".dae");
	Status ret = cacheLoader.TryLoadingCached(sourcePath, hash, version, cachePath);

	if (ret == INFO::OK)
		// Found a valid cached version
		return cachePath;

	// No valid cached version, check if we have a source .dae
	if (ret != INFO::SKIPPED)
	{
		// No valid cached version was found, and no source .dae exists
		ENSURE(ret < 0);

		// Check if source (uncached) .pmd/psa exists
		sourcePath = pathnameNoExtension.ChangeExtension(extn);
		if (m_VFS->GetFileInfo(sourcePath, NULL) != INFO::OK)
		{
			// Broken reference, the caller will need to handle this
			return L"";
		}
		else
		{
			return sourcePath;
		}
	}

	// No valid cached version was found - but source .dae exists
	// We'll try converting it

	// We have a source .dae and invalid cached version, so regenerate cached version
	if (! m->Convert(sourcePath, cachePath, type))
	{
		// The COLLADA converter failed for some reason, this will need to be handled
		//	by the caller
		return L"";
	}

	return cachePath;
}

bool CColladaManager::GenerateCachedFile(const VfsPath& sourcePath, FileType type, VfsPath& archiveCachePath)
{
	std::wstring extn;
	switch (type)
	{
	case PMD: extn = L".pmd"; break;
	case PSA: extn = L".psa"; break;
		// no other alternatives
	}

	CCacheLoader cacheLoader(m_VFS, extn);

	archiveCachePath = cacheLoader.ArchiveCachePath(sourcePath);

	return m->Convert(sourcePath, VfsPath("cache") / archiveCachePath, type);
}
