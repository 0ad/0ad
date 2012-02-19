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

#include "precompiled.h"

#include "ColladaManager.h"

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
			LOGMESSAGE(L"%ls: %hs", path->string().c_str(), text);
		else if (severity == LOG_WARNING)
			LOGWARNING(L"%ls: %hs", path->string().c_str(), text);
		else
			LOGERROR(L"%ls: %hs", path->string().c_str(), text);
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
		: dll("Collada"), m_VFS(vfs)
	{
	}

	~CColladaManagerImpl()
	{
		if (dll.IsLoaded())
			set_logger(NULL, NULL); // unregister the log handler
	}

	bool Convert(const VfsPath& daeFilename, const VfsPath& pmdFilename, CColladaManager::FileType type)
	{
		// To avoid always loading the DLL when it's usually not going to be
		// used (and to do the same on Linux where delay-loading won't help),
		// and to avoid compile-time dependencies (because it's a minor pain
		// to get all the right libraries to build the COLLADA DLL), we load
		// it dynamically when it is required, instead of using the exported
		// functions and binding at link-time.
		if (! dll.IsLoaded())
		{
			if (! dll.LoadDLL())
			{
				LOGERROR(L"Failed to load COLLADA conversion DLL");
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
				LOGERROR(L"Failed to load symbols from COLLADA conversion DLL");
				dll.Unload();
				return false;
			}

			VfsPath skeletonPath("art/skeletons/skeletons.xml");

			// Set the filename for the logger to report
			set_logger(ColladaLog, static_cast<void*>(&skeletonPath));

			CVFSFile skeletonFile;
			if (skeletonFile.Load(m_VFS, skeletonPath) != PSRETURN_OK)
			{
				LOGERROR(L"Failed to read skeleton definitions");
				dll.Unload();
				return false;
			}

			int ok = set_skeleton_definitions((const char*)skeletonFile.GetBuffer(), (int)skeletonFile.GetBufferSize());
			if (ok < 0)
			{
				LOGERROR(L"Failed to load skeleton definitions");
				dll.Unload();
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

private:
	PIVFS m_VFS;
};

CColladaManager::CColladaManager(const PIVFS& vfs)
: m(new CColladaManagerImpl(vfs)), m_VFS(vfs)
{
}

CColladaManager::~CColladaManager()
{
	delete m;
}

void CColladaManager::PrepareCacheKey(MD5& hash, u32& version)
{
	// Include skeletons.xml file info in the hash
	VfsPath skeletonPath("art/skeletons/skeletons.xml");
	FileInfo fileInfo;

	// This will cause an assertion failure if skeletons.xml doesn't exist,
	//	because fileinfo is not a NULL pointer, which is annoying but that
	//	should never happen, unless there really is a problem
	if (m_VFS->GetFileInfo(skeletonPath, &fileInfo) != INFO::OK)
	{
		LOGERROR(L"Failed to stat '%ls' for DAE caching", skeletonPath.string().c_str());
		// We can continue, something else will break if we try loading a skeletal model
	}
	else
	{
		u64 skeletonsModifyTime = (u64)fileInfo.MTime() & ~1; // skip lowest bit, since zip and FAT don't preserve it
		u64 skeletonsSize = (u64)fileInfo.Size();	
		hash.Update((const u8*)&skeletonsModifyTime, sizeof(skeletonsModifyTime));
		hash.Update((const u8*)&skeletonsSize, sizeof(skeletonsSize));
	}

	// Add converter version to the hash
	version = COLLADA_CONVERTER_VERSION;
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
	PrepareCacheKey(hash, version);

	VfsPath cachePath;
	VfsPath sourcePath = pathnameNoExtension.ChangeExtension(L".dae");
	Status ret = cacheLoader.TryLoadingCached(sourcePath, hash, version, cachePath);
	if (ret == INFO::OK)
	{
		// Found a valid cached version
		return cachePath;
	}
	else if (ret == INFO::SKIPPED)
	{
		// No valid cached version was found - but source .dae exists
		// We'll try converting it
	}
	else
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
	MD5 hash;
	u32 version;
	PrepareCacheKey(hash, version);

	archiveCachePath = cacheLoader.ArchiveCachePath(sourcePath);

	return m->Convert(sourcePath, VfsPath("cache") / archiveCachePath, type);
}
