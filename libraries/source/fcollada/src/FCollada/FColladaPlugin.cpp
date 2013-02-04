/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FColladaPlugin.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExtra.h"
#include "FUtils/FUFileManager.h"
#include "FUtils/FUPlugin.h"
#include "FUtils/FUPluginManager.h"
#include "FUtils/FUUri.h"

//
// FColladaPlugin
//
ImplementObjectType(FCPExtraTechnique);

//
// FArchivingPlugin
//
ImplementObjectType(FCPArchive);

//
// FColladaPluginManager
//
ImplementObjectType(FColladaPluginManager);

FColladaPluginManager::FColladaPluginManager()
:	loader(NULL)
{
	// Create the plug-in loader and create all the FCollada plug-ins.
	loader = new FUPluginManager(FC("*.fcp|*.fvp"));
	loader->LoadPlugins(FUPlugin::GetClassType());

	// Retrieve and sort the plug-ins.
	size_t pluginCount = loader->GetLoadedPluginCount();
	for (size_t i = 0; i < pluginCount; ++i)
	{
		FUPlugin* _plugin = loader->GetLoadedPlugin(i);
		if (_plugin->HasType(FCPExtraTechnique::GetClassType()))
		{
			FCPExtraTechnique* plugin = (FCPExtraTechnique*) _plugin;
			const char* profileName = plugin->GetProfileName();
			if (profileName != NULL && profileName[0] != 0)
			{
				extraTechniquePlugins.push_back(plugin);
			}
		}
		else if (_plugin->HasType(FCPArchive::GetClassType()))
		{
			archivePlugins.push_back((FCPArchive*)_plugin);
		}
	}

}

FColladaPluginManager::~FColladaPluginManager()
{
	SAFE_DELETE(loader);
}

bool FColladaPluginManager::RegisterPlugin(FUPlugin* plugin)
{
	if (plugin != NULL)
	{
		if (plugin->HasType(FCPArchive::GetClassType()))
		{
			archivePlugins.push_back((FCPArchive*) plugin);
			return true;
		}
		else if (plugin->HasType(FCPExtraTechnique::GetClassType()))
		{
			FCPExtraTechnique* extraPlugin = (FCPExtraTechnique*) plugin;
			const char* profileName = extraPlugin->GetProfileName();
			if (profileName != NULL && *profileName != 0)
			{
				extraTechniquePlugins.push_back(extraPlugin);
				return true;
			}
		}
	}
	return false;
}

void FColladaPluginManager::CreateExtraTechniquePluginMap(FCPExtraMap& map)
{
	for (FCPExtraTechnique** itP = extraTechniquePlugins.begin(); itP != extraTechniquePlugins.end(); ++itP)
	{
		const char* profileName = (*itP)->GetProfileName();
		uint32 crc = FUCrc32::CRC32(profileName);
		map.insert(crc, *itP);
	}
}

bool FColladaPluginManager::LoadDocumentFromFile(FCDocument* document, const fchar* filename)
{
	FCPArchive* archiver = FindArchivePlugin(filename);
	if (archiver != NULL)
	{
		bool success = archiver->ImportFile(filename, document);
		if (success) PostImportDocument(document);
		return success;
	}
	else
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::NO_MATCHING_PLUGIN, 0);
		return false;
	}
}

bool FColladaPluginManager::LoadDocumentFromMemory(const fchar* filename, FCDocument* document, void* data, size_t length)
{
	FCPArchive* archiver = FindArchivePlugin(filename);
	if (archiver != NULL)
	{
		bool success = archiver->ImportFileFromMemory(filename, document, data, length);
		if (success) PostImportDocument(document);
		return success;
	}
	else
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::NO_MATCHING_PLUGIN, 0);
		return false;
	}
}

bool FColladaPluginManager::SaveDocumentToFile(FCDocument* document, const fchar* filename)
{
	FCPArchive* archiver = FindArchivePlugin(filename);
	if (archiver != NULL)
	{
		FCDETechniqueList techniques;
		PreExportDocument(document, techniques);
		bool success = archiver->ExportFile(document, filename);
		PostExportDocument(document, techniques);
		return success;
	}
	else
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::NO_MATCHING_PLUGIN, 0);
		return false;
	}
}

FCPArchive* FColladaPluginManager::FindArchivePlugin(const fchar* filename)
{
	FUUri fileUri(filename);
	fstring extension = FUFileManager::GetFileExtension(fileUri.GetPath());

	for (size_t i = 0; i < archivePlugins.size(); ++i)
	{
		FCPArchive* curArchive = archivePlugins[i];
		for (int j = 0; j < curArchive->GetSupportedExtensionsCount(); ++j)
		{
			fstring targetExt = FUStringConversion::ToFString(curArchive->GetSupportedExtensionAt(j));
			if (IsEquivalentI(extension, targetExt))
			{
				return curArchive;
			}
		}
	}

	return NULL;
}

void FColladaPluginManager::PostImportDocument(FCDocument* document)
{
	// Create a map of the plugin profile names in order to process the extra trees faster.
	FCPExtraMap pluginMap;
	CreateExtraTechniquePluginMap(pluginMap);
	if (pluginMap.empty()) return;

	FCDExtraSet& extraTrees = document->GetExtraTrees();
	for (FCDExtraSet::iterator itE = extraTrees.begin(); itE != extraTrees.end(); ++itE)
	{
		size_t typeCount = itE->first->GetTypeCount();
		for (size_t i = 0; i < typeCount; ++i)
		{
			FCDEType* type = itE->first->GetType(i);
			size_t techniqueCount = type->GetTechniqueCount();
			for (size_t j = 0; j < techniqueCount; ++j)
			{
				FCDETechnique* technique = type->GetTechnique(j);
				uint32 crc = FUCrc32::CRC32(technique->GetProfile());
				FCPExtraMap::iterator itP = pluginMap.find(crc);
				if (itP != pluginMap.end())
				{
					// Call the corresponding plug-in to process this information.
					FUTrackable* customized = itP->second->ReadFromArchive(technique, itE->first->GetParent());
					if (customized != NULL)
					{
						technique->SetPluginObject(customized);
						while (technique->GetChildNodeCount() != 0) technique->GetChildNode(technique->GetChildNodeCount() - 1)->Release();
					}
				}
			}
		}
	}
}

void FColladaPluginManager::PreExportDocument(FCDocument* document, FCDETechniqueList& techniques)
{
	// Create a map of the plugin profile names in order to process the extra trees faster.
	FCPExtraMap pluginMap;
	CreateExtraTechniquePluginMap(pluginMap);
	if (pluginMap.empty()) return;

	FCDExtraSet& extraTrees = document->GetExtraTrees();
	for (FCDExtraSet::iterator itE = extraTrees.begin(); itE != extraTrees.end(); ++itE)
	{
		size_t typeCount = itE->first->GetTypeCount();
		for (size_t i = 0; i < typeCount; ++i)
		{
			FCDEType* type = itE->first->GetType(i);
			size_t techniqueCount = type->GetTechniqueCount();
			for (size_t j = 0; j < techniqueCount; ++j)
			{
				FCDETechnique* technique = type->GetTechnique(j);
				FUTrackable* customized = technique->GetPluginObject();
				if (customized != NULL)
				{
					uint32 crc = FUCrc32::CRC32(technique->GetProfile());
					FCPExtraMap::iterator itP = pluginMap.find(crc);
					if (itP != pluginMap.end())
					{
						techniques.push_back(technique);
						itP->second->WriteToArchive(technique, customized);
					}
				}
			}
		}
	}
}

void FColladaPluginManager::PostExportDocument(FCDocument* UNUSED(document), FCDETechniqueList& techniques)
{
	for (FCDETechnique** itT = techniques.begin(); itT != techniques.end(); ++itT)
	{
		FUAssert((*itT)->GetPluginObject() != NULL, continue);
		while ((*itT)->GetChildNodeCount() != 0) (*itT)->GetChildNode((*itT)->GetChildNodeCount() - 1)->Release();
	}
}
