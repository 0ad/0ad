/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUPlugin.h"
#include "FUPluginManager.h"
#include "FUFileManager.h"

#ifdef WIN32
	#include <io.h>
	#if defined(UNICODE)
		#define ffinddata _wfinddata_t
		#define ffindfirst _wfindfirst
		#define ffindclose _findclose
		#define ffindnext _wfindnext
	#else // UNICODE
		#define ffinddata _finddata_t
		#define ffindfirst _findfirst
		#define ffindclose _findclose
		#define ffindnext _findnext
	#endif
#elif defined(__APPLE__) || defined(LINUX)
	#include <dlfcn.h>
	#include <dirent.h>
#endif //WIN32

//
// FUPlugin
//

ImplementObjectType(FUPlugin);

//
// FUPluginManager
//

FUPluginManager::FUPluginManager(const fchar* _filter)
{
	fstring applicationFolderName = FUFileManager::GetApplicationFolderName();
	if (!applicationFolderName.empty())
	{
		LoadPluginsInFolderName(applicationFolderName, _filter);
	}

	fstring moduleFolderName = FUFileManager::GetModuleFolderName();
	if (!moduleFolderName.empty() && !IsEquivalent(moduleFolderName, applicationFolderName))
	{
		LoadPluginsInFolderName(moduleFolderName, _filter);
	}
}

void FUPluginManager::LoadPluginsInFolderName(const fstring& folderName, const fchar* _filter)
{
	// Append the wanted extension for the plugins.
	FUStringBuilder pluginFolder(folderName);
	fchar lastChar = folderName[pluginFolder.length() - 1];
	if (lastChar != '\\' && lastChar != '/') pluginFolder.append((fchar) '/');
	pluginFolder.append(FC("Plugins/"));
	pluginFolderName = pluginFolder.ToString();

	if (_filter == NULL || _filter[0] == 0) _filter = FC("*.*");
	do
	{
		const fchar* nextFilter = fstrchr(_filter, '|');
		fstring filter(_filter);
		if (nextFilter != NULL)
		{
			filter.erase(nextFilter - _filter);
			++nextFilter; // skip the pipe.
		}
		_filter = nextFilter;

		// Windows-only for now.
#if defined(WIN32)
		size_t filterLength = filter.length();
		// Iterate over all the filtered files within the given folder.
		ffinddata folderIterator;
		fstring searchString = pluginFolderName + filter;
		intptr_t folderHandle = ffindfirst(searchString.c_str(), &folderIterator);
		if (folderHandle != -1L)
		{
			int32 isDone = FALSE;
			while (isDone == FALSE)
			{
				bool keep = false;
				PluginLibrary* library = new PluginLibrary();
				library->filename = pluginFolderName + folderIterator.name;

				// work around for wildcards and 3 letter extensions that pick up 3+ letter extensions
				// e.g. "dir *.fvp" in command prompt on a directory with "a.fvp", "a.fvpa", and "a.dll" returns 
				// "a.fvp" and "a.fvpa"
				bool checkModule = true;
				if (filterLength > 3)
				{
					if ((filter.at(filterLength-4) == FC('.')) && (filter.at(filterLength-3) != FC('*')) &&
							(filter.at(filterLength-2) != FC('*')) && (filter.at(filterLength-1) != FC('*')))
					{
						size_t filepathLength = fstrlen(folderIterator.name);
						checkModule = (folderIterator.name[filepathLength-4] == filter.at(filterLength-4)) &&
								(folderIterator.name[filepathLength-3] == filter.at(filterLength-3)) &&
								(folderIterator.name[filepathLength-2] == filter.at(filterLength-2)) &&
								(folderIterator.name[filepathLength-1] == filter.at(filterLength-1));
					}
				}

				library->module = LoadLibrary(library->filename.c_str());
				if (checkModule && (library->module != NULL))
				{
					// Retrieve the necessary callbacks
					library->getPluginCount = (GetPluginCount) GetProcAddress(library->module, "GetPluginCount");
					library->getPluginType = (GetPluginType) GetProcAddress(library->module, "GetPluginType");
					library->createPlugin = (CreatePlugin) GetProcAddress(library->module, "CreatePlugin");
					keep = library->createPlugin != NULL && library->getPluginType != NULL && library->getPluginCount != NULL;
				}

				// This is a valid library.
				if (keep) loadedLibraries.push_back(library);
				else { SAFE_DELETE(library); }
				isDone = ffindnext(folderHandle, &folderIterator);
			}
			ffindclose(folderHandle);
		}

#elif defined(__APPLE__) || defined(LINUX)
		fm::string s_filter = TO_STRING(filter);
		if (s_filter.length() > 0 && s_filter.front() == '*') s_filter.erase(0, 1);
		if (s_filter.length() > 0 && s_filter.back() == '*') s_filter.pop_back();

		DIR* directory = opendir(TO_STRING(pluginFolderName).c_str());
		if (directory == NULL) continue;

		dirent* directoryEntry;
		while ((directoryEntry = readdir(directory)) != NULL)
		{
			if (directoryEntry->d_type == DT_DIR) continue; // skip sub-folders.
			if (strstr((const char*) directoryEntry->d_name, s_filter.c_str()) != NULL)
			{
				// We have a match.
				bool keep = false;
				PluginLibrary* library = new PluginLibrary();
				library->filename = pluginFolderName + TO_FSTRING((const char*) directoryEntry->d_name);
				fm::string libraryModuleFilename = TO_STRING(library->filename);
				DEBUG_OUT("Found dynamic library: %s\n", libraryModuleFilename.c_str());
				library->module = dlopen(libraryModuleFilename.c_str(), RTLD_NOW);
				if (library->module != NULL)
				{
					// Retrieve the necessary callbacks
					library->getPluginCount = (GetPluginCount) dlsym(library->module, "GetPluginCount");
					library->getPluginType = (GetPluginType) dlsym(library->module, "GetPluginType");
					library->createPlugin = (CreatePlugin) dlsym(library->module, "CreatePlugin");
					keep = library->createPlugin != NULL && library->getPluginType != NULL && library->getPluginCount != NULL;
				}

				// This is a valid library.
				if (keep) loadedLibraries.push_back(library);
				else { SAFE_DELETE(library); }
			}
		}
		closedir(directory);

#endif // WIN32
	} while (_filter != NULL);
}

FUPluginManager::~FUPluginManager()
{
	UnloadPlugins();
	FUAssert(loadedPlugins.empty(), return);

	// Detach all the plugin libraries.
	for (PluginLibraryList::iterator it = loadedLibraries.begin(); it != loadedLibraries.end(); ++it)
	{
#if defined(WIN32)
		if ((*it)->module != NULL) FreeLibrary((*it)->module);
#elif defined(LINUX) || defined(__APPLE__)
		if ((*it)->module != NULL) dlclose((*it)->module);
#endif // WIN32
	}
	CLEAR_POINTER_VECTOR(loadedLibraries);
}

void FUPluginManager::LoadPlugins(const FUObjectType& pluginType)
{
	for (PluginLibraryList::iterator it = loadedLibraries.begin(); it != loadedLibraries.end(); ++it)
	{
#ifndef _DEBUG
		try
#endif // _DEBUG
		{
			DEBUG_OUT("Loading plug-in: %s\n", TO_STRING((*it)->filename).c_str());
			FUAssert((*it)->createPlugin != NULL && (*it)->getPluginType != NULL && (*it)->getPluginCount != NULL, continue);
			uint32 pluginCount = (*((*it)->getPluginCount))();
			for (uint32 i = 0; i < pluginCount; ++i)
			{
				// Retrieve the types of all the plug-ins within this library.
				// Compare them against the wanted types and create the wanted plug-ins.
				const FUObjectType* type = (*((*it)->getPluginType))(i);
				if (type->Includes(pluginType))
				{
					FUPlugin* plugin = (*((*it)->createPlugin))(i);
					if (plugin == NULL) continue;
					loadedPlugins.push_back(plugin);
				}
			}
		}
#ifndef _DEBUG
		catch (...)
		{
			fm::string _filename = TO_STRING((*it)->filename);
			ERROR_OUT("Unhandled exception when loading plugin: %s.", _filename.c_str());
		}
#endif // _DEBUG
	}
}

void FUPluginManager::AddPluginLibrary(FUPluginManager::GetPluginCount fnGetPluginCount, FUPluginManager::GetPluginType fnGetPluginType, FUPluginManager::CreatePlugin fnCreatePlugin)
{
	PluginLibrary* library = new PluginLibrary();
	library->getPluginCount = fnGetPluginCount;
	library->getPluginType = fnGetPluginType;
	library->createPlugin = fnCreatePlugin;
	library->filename.clear();
	library->module = NULL;
	loadedLibraries.push_back(library);
}

void FUPluginManager::UnloadPlugins()
{
	loadedPlugins.clear();
}
