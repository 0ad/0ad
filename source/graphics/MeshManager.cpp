#include "precompiled.h"

#include "MeshManager.h"

#include "graphics/ModelDef.h"
#include "ps/CLogger.h"
#include "ps/FileUnpacker.h" // to get access to its CError
#include "ps/CVFSFile.h"
#include "ps/DllLoader.h"
#include "lib/res/file/vfs.h"

namespace Collada
{
	#include "collada/DLL.h"
}

#include <boost/weak_ptr.hpp>

#define LOG_CATEGORY "mesh"

void ColladaLog(int severity, const char* text)
{
	LOG(severity==LOG_INFO ? NORMAL : severity==LOG_WARNING ? WARNING : ERROR,
		"collada", "%s", text);
}

struct VFSOutputCB
{
	VFSOutputCB(Handle hf) : hf(hf) {}
	void operator() (const char* data, unsigned int length)
	{
		FileIOBuf buf = (FileIOBuf)data;
		const ssize_t ret = vfs_io(hf, length, &buf);
		// TODO: handle errors sensibly
	}

	Handle hf;
};

void ColladaOutput(void* cb_data, const char* data, unsigned int length)
{
	VFSOutputCB* cb = static_cast<VFSOutputCB*>(cb_data);
	(*cb)(data, length);
}

typedef STL_HASH_MAP<CStr, boost::weak_ptr<CModelDef>, CStr_hash_compare> mesh_map;
class CMeshManagerImpl
{
	DllLoader dll;

	void (*set_logger)(Collada::LogFn logger);
	int (*convert_dae_to_pmd)(const char* dae, Collada::OutputFn pmd_writer, void* cb_data);

public:
	mesh_map MeshMap;

	CMeshManagerImpl()
		: dll("Collada")
	{
	}

	~CMeshManagerImpl()
	{
		if (dll.IsLoaded())
			set_logger(NULL); // unregister the log handler
	}

	CModelDefPtr Convert(const CStr& daeFilename, const CStr& pmdFilename, const CStr& name)
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
				LOG_ONCE(ERROR, LOG_CATEGORY, "Failed to load COLLADA conversion DLL");
				return CModelDefPtr();
			}

			try
			{
				dll.LoadSymbol("set_logger", set_logger);
				dll.LoadSymbol("convert_dae_to_pmd", convert_dae_to_pmd);
			}
			catch (PSERROR_DllLoader&)
			{
				LOG(ERROR, LOG_CATEGORY, "Failed to load symbols from COLLADA conversion DLL");
				return CModelDefPtr();
			}
			
			set_logger(ColladaLog);
		}

		// We need to null-terminate the buffer, so do it (possibly inefficiently)
		// by converting to a CStr
		CStr daeData;

		{
			CVFSFile daeFile;
			if (daeFile.Load(daeFilename) != PSRETURN_OK)
				return CModelDefPtr();

			daeData = daeFile.GetAsString();

			// scope closes daeFile - necessary if we don't use FILE_LONG_LIVED
		}

		// Prepare the output file
		Handle hf = vfs_open(pmdFilename, FILE_WRITE|FILE_NO_AIO);
		if (hf < 0)
			return CModelDefPtr();

		// Do the conversion
		VFSOutputCB cb (hf);
 		convert_dae_to_pmd(daeData.c_str(), ColladaOutput, static_cast<void*>(&cb));

		vfs_close(hf);

		// Now load the PMD that was just created
		CModelDefPtr model (CModelDef::Load(pmdFilename, name));
		MeshMap[name] = model;
		return model;
	}
};

CMeshManager::CMeshManager()
: m(new CMeshManagerImpl())
{
}

CMeshManager::~CMeshManager()
{
	delete m;
}

CModelDefPtr CMeshManager::GetMesh(const CStr& filename)
{
	// Strip a three-letter file extension (if there is one) from the filename
	CStr name;
	if (filename.Length() > 4 && filename[filename.Length()-4] == '.')
		name = filename.GetSubstring(0, filename.Length()-4);
	else
		name = filename;

	// Find the mesh if it's already been loaded and cached
	mesh_map::iterator iter = m->MeshMap.find(name);
	if (iter != m->MeshMap.end() && !iter->second.expired())
		return CModelDefPtr(iter->second);

	/*

	If there is a .dae file:
		* Calculate a hash to identify it.
		* Look for a cached .pmd file matching that hash.
		* If it exists, load it. Else, convert the .dae into .pmd and load it.
	Otherwise, if there is a (non-cache) .pmd file:
		* Load it.
	Else, fail.

	The hash calculation ought to be fast, since normally (during development)
	the .dae file will exist but won't have changed recently and so the cache
	would be used. Hence, just hash the file's size, mtime, and the converter
	version number (so updates will cause regeneration of .pmds) instead of
	its contents.

	TODO (maybe): The .dae -> .pmd conversion may fail (e.g. if the .dae is
	invalid or unsupported), but it may take a long time to start the conversion
	then realise it's not going to work. That will delay the loading of the game
	every time, which is annoying, so maybe it should cache the error messge
	until the .dae is updated and fixed. (Alternatively, avoid having many
	broken .daes in the game's data files.)

	*/

	try
	{
		CStr dae = name+".dae";
		if (! vfs_exists(dae))
		{
			// No .dae - got to use the .pmd, assuming there is one
			CModelDefPtr model (CModelDef::Load(name+".pmd", name));
			m->MeshMap[name] = model;
			return model;
		}

		// There is a .dae - see if there's an up-to-date cached copy

		struct stat fileStat;
		if (vfs_stat(dae, &fileStat) < 0)
		{
			// This shouldn't occur for any sensible reasons
			LOG(ERROR, LOG_CATEGORY, "Failed to stat DAE file '%s'", filename.c_str());
			return CModelDefPtr();
		}

		// Build a struct of all the data we want to hash.
		// (Use ints and not time_t/off_t because we don't care about overflow
		// but do care about the fields not being 64-bit aligned)
		struct { int version; int mtime; int size; } hashSource
			= { COLLADA_CONVERTER_VERSION, fileStat.st_mtime & ~1, fileStat.st_size };
		cassert(sizeof(hashSource) == sizeof(int) * 3); // no padding, because that would be bad
		// Calculate the hash, convert to hex
		u32 hash = fnv_hash(static_cast<void*>(&hashSource), sizeof(hashSource));
		char hashString[9];
		sprintf(hashString, "%08x", hash);

		char realDaePath[PATH_MAX];
		vfs_realpath(dae, realDaePath);
		// realDaePath is "mods/whatever/art/meshes/whatever.dae"

		CStr cachedPmdVfsPath = "cache/";
		cachedPmdVfsPath += realDaePath;
		// Remove the .dae extension (which will certainly be there)
		cachedPmdVfsPath = cachedPmdVfsPath.GetSubstring(0, cachedPmdVfsPath.Length()-4);
		// Add a _hash.pmd extension
		cachedPmdVfsPath += "_";
		cachedPmdVfsPath += hashString;
		cachedPmdVfsPath += ".pmd";

		// If it's cached, load and return that copy
		if (vfs_exists(cachedPmdVfsPath))
		{
			CModelDefPtr model (CModelDef::Load(cachedPmdVfsPath, name));
			m->MeshMap[name] = model;
			return model;
		}

		// Not in the cache, so create it

		return m->Convert(dae, cachedPmdVfsPath, name);
	}
	catch (PSERROR_File&)
	{
		LOG(ERROR, LOG_CATEGORY, "Could not load mesh '%s'", filename.c_str());
		return CModelDefPtr();
	}
}
