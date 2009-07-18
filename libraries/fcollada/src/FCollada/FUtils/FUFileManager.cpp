/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUFile.h"
#include "FUFileManager.h"
#include "FUStringConversion.h"

#include <errno.h>

#if defined(WIN32)
	#include <direct.h>
#elif defined(__APPLE__)
	#include <mach-o/dyld.h>
	typedef int (*NSGetExecutablePathProcPtr)(char *buf, size_t *bufsize);
#elif defined(LINUX)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

//
// Macros and extra definitions
//

#ifdef WIN32
#define FOLDER_CHAR '\\'
#define UNWANTED_FOLDER_CHAR '/'
#define FOLDER_STR FC("\\")
#else
#define FOLDER_CHAR '/'
#define UNWANTED_FOLDER_CHAR '\\'
#define FOLDER_STR FC("/")
#endif

inline bool IsSomeFolderChar(fchar c) { return c == FOLDER_CHAR || c == UNWANTED_FOLDER_CHAR; }

SchemeCallbacks::SchemeCallbacks()
:	load(NULL)
,	exists(NULL)
,	request(NULL)
{}

SchemeCallbacks::SchemeCallbacks(const SchemeCallbacks& copy)
:	load(NULL)
,	exists(NULL)
,	request(NULL)
{
	// Do a deep copy here, not just copy pointers.  We own these FUFuctors!
	if (copy.load) load = copy.load->Copy();
	if (copy.exists) exists = copy.exists->Copy();
	if (copy.request) request = copy.request->Copy();

	for (size_t i = 0; i < copy.openers.size(); i++)
	{
		openers.push_back(copy.openers[i]->Copy());
	}
}

SchemeCallbacks::~SchemeCallbacks()
{
	SAFE_DELETE(load);
	SAFE_DELETE(exists);
	SAFE_DELETE(request);
	CLEAR_POINTER_VECTOR(openers);
}

FCOLLADA_EXPORT SchemeCallbacks* NewSchemeCallbacks()
{
	return new SchemeCallbacks();
}

//
// FUFileManager
//

FUFileManager::FUFileManager()
{
#ifdef __PPU__
	// Push on the stack the default home path
	pathStack.push_back(FC("/app_home"));
#else
	// Push on the stack the original root path
	char fullPath[MAX_PATH];
	_getcwd(fullPath, MAX_PATH);
	size_t length = strlen(fullPath);
	if (length < MAX_PATH - 2 && fullPath[length-1] != '/' && fullPath[length-1] != '\\')
	{
		fullPath[length] = '/';
		fullPath[length + 1] = 0;
	}
	pathStack.push_back(TO_FSTRING((const char*) fullPath));
#endif // __PPU__

	forceAbsolute = false;
}

FUFileManager::~FUFileManager()
{
	RemoveAllSchemeCallbacks();
}

// Set a new root path
void FUFileManager::PushRootPath(const fstring& path)
{
	fstring absolutePath = GetCurrentUri().MakeAbsolute(path);
	if (absolutePath.length() > 0 && absolutePath.back() != '\\' && absolutePath.back() != '/') absolutePath.append((fchar) '/');
	pathStack.push_back(FUUri(absolutePath));
}

// Go back to the previous root path
void FUFileManager::PopRootPath()
{
	if (pathStack.size() > 1)
	{
		pathStack.pop_back();
	}
}

// Set the current path root, using a known filename
void FUFileManager::PushRootFile(const fstring& filename)
{
	fstring f = GetCurrentUri().MakeAbsolute(filename);

	// Strip the filename of the actual file's name
	f = StripFileFromPath(f);
	PushRootPath(f);
}

void FUFileManager::PopRootFile()
{
	PopRootPath();
}

// Open a file to read
FUFile* FUFileManager::OpenFile(const fstring& filename, bool write, SchemeOnCompleteCallback* onComplete, size_t userData)
{
	// Make sure we have a absolute URI
	fstring absoluteFilename = GetCurrentUri().MakeAbsolute(filename);
	FUUri uri(absoluteFilename);

	// Get the callback
	SchemeCallbacks* callbacks = NULL;
	SchemeCallbackMap::iterator it = schemeCallbackMap.find(uri.GetScheme());
	if (it != schemeCallbackMap.end()) callbacks = it->second;

	if (callbacks != NULL)
	{
		if (onComplete == NULL)
		{
			// No callback provided so the open is blocking

			if (callbacks->load != NULL)
			{
				// We have a callback for this scheme
				absoluteFilename = (*callbacks->load)(uri);
			}
		}
		else
		{
			if (callbacks->request != NULL)
			{
				(*callbacks->request)(uri, onComplete, userData);

				// No file to return
				return NULL;
			}
		}

		if (!callbacks->openers.empty())
		{
			size_t i = 0;
			do
			{
				for (i = 0; i < callbacks->openers.size(); i++)
				{
					fstring newAbsolutePath;
					if ((*callbacks->openers[i])(absoluteFilename, newAbsolutePath))
					{
						// keep this path
						absoluteFilename = newAbsolutePath;
						// override the URI
						pathStack.back() = FUUri(absoluteFilename);
						break;
					}
				}
			} while(i != callbacks->openers.size()); // Allow pre-processes to process in any order
		}
	}
	return new FUFile(absoluteFilename.c_str(), write ? FUFile::WRITE : FUFile::READ);
}

bool FUFileManager::MakeDirectory(const fstring& directory)
{
	FUUri uri(directory);
	fstring absoluteDirectory = uri.GetAbsolutePath();

#ifdef WIN32
	if (_mkdir(TO_STRING(absoluteDirectory).c_str()) == 0) return true;
	errno_t err; _get_errno(&err);
	if (err == EEXIST) return true;
#elif defined(LINUX)
	if (mkdir(TO_STRING(absoluteDirectory).c_str(), ~0u) == 0) return true; // I think this means all permissions..
#elif defined(__APPLE__)
	fm::string _fname = TO_STRING(directory);
	OSErr err = AddFolderDescriptor('extn', 0, 'relf', 0, 0, 0, (ConstStrFileNameParam)_fname.c_str(), false);
#endif // WIN32

	return false;
}

bool FUFileManager::FileExists(const fstring& filename)
{
	// Make sure we have a absolute URI
	fstring absoluteFilename = GetCurrentUri().MakeAbsolute(filename);
	FUUri uri(absoluteFilename);

	// Get the callback
	SchemeCallbacks* callbacks = NULL;
	SchemeCallbackMap::iterator it = schemeCallbackMap.find(uri.GetScheme());
	if (it != schemeCallbackMap.end()) callbacks = it->second;

	if (callbacks != NULL && callbacks->exists != NULL)
	{
		// We have a callback for this scheme
		return (*callbacks->exists)(uri);
	}

	if (uri.GetScheme() == FUUri::FILE)
	{
		FUFile file(absoluteFilename, FUFile::READ);
		bool exists = (file.GetHandle() != NULL);
		return exists;
	}

	// we can't tell
	return false;
}

// Strip a full filename of its filename, returning the path
fstring FUFileManager::StripFileFromPath(const fstring& filename)
{
	fchar fullPath[MAX_PATH + 1];
	fstrncpy(fullPath, filename.c_str(), MAX_PATH);
	fullPath[MAX_PATH] = 0;
	fchar* lastSlash = fstrrchr(fullPath, FC('/'));
	fchar* lastBackslash = fstrrchr(fullPath, FC('\\'));
	lastSlash = max(lastSlash, lastBackslash);
	if (lastSlash != NULL) *(lastSlash + 1) = 0;
	return fstring(fullPath);
}

// Extract the file extension out of a filename
fstring FUFileManager::GetFileExtension(const fstring& _filename)
{
	fchar filename[MAX_PATH];
	fstrncpy(filename, _filename.c_str(), MAX_PATH);
	filename[MAX_PATH - 1] = 0;

	fchar* lastPeriod = fstrrchr(filename, '.');
	if (lastPeriod == NULL) return emptyFString;

	fchar* lastSlash = fstrrchr(filename, '/');
	fchar* lastBackslash = fstrrchr(filename, '\\');
	lastSlash = max(lastSlash, lastBackslash);
	if (lastSlash > lastPeriod) return emptyFString;

	fstrlower(lastPeriod + 1);	// [claforte] Untested on __PPU__, refer to definition of fstrlower.
	return fstring(lastPeriod + 1);
}

fstring FUFileManager::CleanUri(const FUUri& uri)
{
	fstring out;
	if (uri.GetScheme() == FUUri::NONE) out = FS("#") + uri.GetFragment();
	else if (forceAbsolute) out = uri.GetAbsoluteUri();
	else out = uri.GetRelativeUri(GetCurrentUri());
	return out;
}

fstring FUFileManager::ExtractNetworkHostname(fstring& filename)
{
	fstring hostname;
#ifdef WIN32
	// UNC network paths are only supported on WIN32, right now.
	if (filename.size() > 2 && (filename[0] == '/' || filename[0] == '\\') && filename[1] == filename[0])
	{
		size_t nextSlash = min(filename.find('/', 2), filename.find('\\', 2));
		FUAssert(nextSlash != fstring::npos, return hostname); // The UNC patch should always have at least one network path
		hostname = filename.substr(2, nextSlash - 2);
		filename.erase(0, nextSlash); // Keep the slash to indicate absolute path.
	}
#endif
	return hostname;
}


#ifdef WIN32
// --------------------------------------------------------------------------------------------------------------------
// ------------------   start of from http://www.codeguru.com/Cpp/W-P/dll/tips/article.php/c3635/    ------------------

#if _MSC_VER >= 1300    // for VC 7.0
  // from ATL 7.0 sources
  #ifndef _delayimp_h
  extern "C" IMAGE_DOS_HEADER __ImageBase;
  #endif
#endif

HMODULE GetCurrentModule()
{
#if _MSC_VER < 1300    // earlier than .NET compiler (VC 6.0)

  // Here's a trick that will get you the handle of the module
  // you're running in without any a-priori knowledge:
  // http://www.dotnet247.com/247reference/msgs/13/65259.aspx

  MEMORY_BASIC_INFORMATION mbi;
  static int dummy;
  VirtualQuery(&dummy, &mbi, sizeof(mbi));

  return reinterpret_cast<HMODULE>(mbi.AllocationBase);

#else    // VC 7.0

  // from ATL 7.0 sources

  return reinterpret_cast<HMODULE>(&__ImageBase);
#endif
}

// -------------------   end of from http://www.codeguru.com/Cpp/W-P/dll/tips/article.php/c3635/    -------------------
// --------------------------------------------------------------------------------------------------------------------
#endif // WIN32

fstring FUFileManager::GetModuleFolderName()
{
	fstring _moduleUri;

#ifdef WIN32
	HMODULE currentModule = GetCurrentModule();

	fchar buffer[1024];
	fchar longPath[1024];
	GetModuleFileName(currentModule, buffer, 1024);
	buffer[1023] = 0;
	GetLongPathName(buffer, longPath, 1024);
	longPath[1023] = 0;

	_moduleUri = longPath;
#endif // WIN32

	fstring out;
	GetFolderFromPath(_moduleUri, out);
	return out;
}

fstring FUFileManager::GetApplicationFolderName()
{
	fstring _uri;

#ifdef WIN32
	fchar buffer[1024];
	GetModuleFileName(NULL, buffer, 1024);
	buffer[1023] = 0;
	_uri = buffer;
#elif defined(LINUX)
	char path[1024]; 
	char path2[1024];
	struct stat stat_buf;
	strncpy(path2, "/proc/self/exe", 1023);
	while (1)
	{
		size_t size = readlink(path2, path, 1023);
		if (size == (size_t) ~0)
		{
			path[0] = 0;
			break;
		}
		else
		{
			path[max(size_t(1023), size)] = '\0';
			int i = stat (path, &stat_buf);
			if (i == -1) break; 
			else if (!S_ISLNK(stat_buf.st_mode)) break;
			strncpy(path, path2, 1023);
		}
	}
	//"path" should have the application folder path in it.
	const char * exeName = &path[0];
	_uri = TO_FSTRING(exeName);
#elif defined(__APPLE__)
	char path[1024];
	size_t pathLength = 1023;
	static NSGetExecutablePathProcPtr NSGetExecutablePath = NULL;
	if (NSGetExecutablePath == NULL)
	{
		NSGetExecutablePath = (NSGetExecutablePathProcPtr) NSAddressOfSymbol(NSLookupAndBindSymbol("__NSGetExecutablePath"));
	}
	if (NSGetExecutablePath != NULL)
	{
		(*NSGetExecutablePath)(path, &pathLength);
		path[1023] = 0;
	}
	_uri = TO_FSTRING((const char*) path);
#endif // WIN32

	fstring out;
	GetFolderFromPath(_uri, out);
	return out;
}

void FUFileManager::GetFolderFromPath(const fstring& folder, fstring& path)
{
	path = StripFileFromPath(folder);
	if (path.length() > 0 && (path.back() == UNWANTED_FOLDER_CHAR || path.back() == FOLDER_CHAR))
	{
		path.pop_back();
	}
}

void FUFileManager::SetSchemeCallbacks(FUUri::Scheme scheme, SchemeCallbacks* callbacks)
{
	SchemeCallbackMap::iterator it = schemeCallbackMap.find(scheme);
	if (it != schemeCallbackMap.end())
	{
		RemoveSchemeCallbacks(scheme);
	}

	schemeCallbackMap.insert(scheme, callbacks);
}

void FUFileManager::RemoveSchemeCallbacks(FUUri::Scheme scheme)
{
	SchemeCallbackMap::iterator it = schemeCallbackMap.find(scheme);
	if (it != schemeCallbackMap.end())
	{
		SAFE_DELETE(it->second);
		schemeCallbackMap.erase(it);
	}
}

void FUFileManager::RemoveAllSchemeCallbacks()
{
	SchemeCallbackMap::iterator it = schemeCallbackMap.begin();
	for (; it != schemeCallbackMap.end(); ++it)
	{
		SAFE_DELETE(it->second);
	}
	schemeCallbackMap.clear();
}

void FUFileManager::CloneSchemeCallbacks(const FUFileManager* srcFileManager)
{
	FUAssert(srcFileManager != NULL, return);
	if (srcFileManager == this) return;

	RemoveAllSchemeCallbacks();

	SchemeCallbackMap::const_iterator it = srcFileManager->schemeCallbackMap.begin();
	for (; it != srcFileManager->schemeCallbackMap.end(); ++it)
	{
		SchemeCallbacks* callbacks = new SchemeCallbacks(*(it->second));
		schemeCallbackMap.insert(it->first, callbacks);
	}
}
