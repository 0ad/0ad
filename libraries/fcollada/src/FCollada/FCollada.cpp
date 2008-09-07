/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDPlaceHolder.h"
#include "FUtils/FUTestBed.h"
#include "FColladaPlugin.h"
#include "FCollada.h"

// This function is defined for all standard plug-ins.
// Create at least one default archiving plug-in.
// For us, that's XML archiving, through COLLADA.
extern "C"
{
	extern
#ifdef WIN32
	__declspec(dllexport)
//#else 
//	FCOLLADA_EXPORT
#endif // WIN32
	FUPlugin* CreatePlugin(uint32);
}

namespace FCollada
{
	static size_t libraryInitializationCount = 0;
	static FUTrackedList<FCDocument> topDocuments;
	static bool dereferenceFlag = true;
	FColladaPluginManager* pluginManager = NULL; // Externed in FCDExtra.cpp.
	CancelLoadingCallback cancelLoadingCallback = NULL;

	FCOLLADA_EXPORT FColladaPluginManager* GetPluginManager() { return pluginManager; }

	FCOLLADA_EXPORT unsigned long GetVersion() { return FCOLLADA_VERSION; }

	FCOLLADA_EXPORT void Initialize()
	{
		if (pluginManager == NULL)
		{
			pluginManager = new FColladaPluginManager();
			pluginManager->RegisterPlugin(CreatePlugin(0));
		}
		++libraryInitializationCount;
	}

	FCOLLADA_EXPORT size_t Release()
	{
		FUAssert(libraryInitializationCount > 0, return 0);

		if (--libraryInitializationCount == 0)
		{
			// Detach all the plug-ins.
			SAFE_RELEASE(pluginManager);

			FUAssert(topDocuments.empty(),);
			while (!topDocuments.empty()) topDocuments.back()->Release();
		}
		return libraryInitializationCount;
	}

	FCOLLADA_EXPORT FCDocument* NewTopDocument()
	{
		// Just add the top documents to the above tracker: this will add one global tracker and the
		// document will not be released automatically by the document placeholders.
		FCDocument* document = new FCDocument();
		topDocuments.push_back(document);
		return document;
	}

	FCOLLADA_EXPORT FCDocument* NewDocument()
	{
		return new FCDocument;
	}

	FCOLLADA_EXPORT size_t GetTopDocumentCount() { return topDocuments.size(); }
	FCOLLADA_EXPORT FCDocument* GetTopDocument(size_t index) { FUAssert(index < topDocuments.size(), return NULL); return topDocuments.at(index); }
	FCOLLADA_EXPORT bool IsTopDocument(FCDocument* document) { return topDocuments.contains(document); }

	FCOLLADA_EXPORT void GetAllDocuments(FCDocumentList& documents)
	{
		documents.clear();
		documents.insert(documents.end(), topDocuments.begin(), topDocuments.end());
		for (size_t index = 0; index < topDocuments.size(); ++index)
		{
			FCDocument* document = documents[index];
			
			FCDExternalReferenceManager* xrefManager = document->GetExternalReferenceManager();
			size_t placeHolderCount = xrefManager->GetPlaceHolderCount();
			for (size_t p = 0; p < placeHolderCount; ++p)
			{
				FCDPlaceHolder* placeHolder = xrefManager->GetPlaceHolder(p);
				FCDocument* target = placeHolder->GetTarget(false);
				if (target != NULL && !documents.contains(target)) documents.push_back(target);
			}
		}
	}

	FCOLLADA_EXPORT FCDocument* LoadDocumentFromFile(const fchar* filename)
	{
		FUAssert(pluginManager != NULL, return NULL);
		FCDocument* theDocument = FCollada::NewTopDocument();
		
		if (!LoadDocumentFromFile(theDocument, filename))
		{
			SAFE_DELETE(theDocument);
		}
		return theDocument;
	}

	FCOLLADA_EXPORT bool LoadDocumentFromFile(FCDocument* document, const fchar* filename)
	{
		FUAssert(pluginManager != NULL, return false);
		return pluginManager->LoadDocumentFromFile(document, filename);
	}

	FCOLLADA_EXPORT FCDocument* LoadDocument(const fchar* filename)
	{
		// This function is deprecated.
		FCDocument* document = FCollada::NewTopDocument();
		if (!LoadDocumentFromFile(document, filename)) document->Release();
		return document;
	}

	FCOLLADA_EXPORT bool LoadDocumentFromMemory(const fchar* filename, FCDocument* document, void* data, size_t length)
	{
		FUAssert(pluginManager != NULL, return false);
		return pluginManager->LoadDocumentFromMemory(filename, document, data, length);
	}

	FCOLLADA_EXPORT bool SaveDocument(FCDocument* document, const fchar* filename)
	{
		FUAssert(pluginManager != NULL, return false);
		return pluginManager->SaveDocumentToFile(document, filename);
	}

	FCOLLADA_EXPORT bool GetDereferenceFlag()
	{
		return dereferenceFlag;
	}

	FCOLLADA_EXPORT void SetDereferenceFlag(bool flag)
	{
		dereferenceFlag = flag;
	}

	FCOLLADA_EXPORT bool RegisterPlugin(FUPlugin* plugin)
	{
		// This function is deprecated.
		FUAssert(pluginManager != NULL, return false);
		return pluginManager->RegisterPlugin(plugin);
	}

	FCOLLADA_EXPORT void SetCancelLoadingCallback(CancelLoadingCallback callback)
	{
		cancelLoadingCallback = callback;
	}

	FCOLLADA_EXPORT bool CancelLoading()
	{
		if (cancelLoadingCallback) return (*cancelLoadingCallback)();
		return false;
	}
};

#ifndef RETAIL
extern FUTestSuite* _testFMArray,* _testFMTree, * _testFMQuaternion;
extern FUTestSuite* _testFUObject, * _testFUCrc32, * _testFUFunctor;
extern FUTestSuite* _testFUEvent, * _testFUString, * _testFUFileManager;
extern FUTestSuite* _testFUBoundingTest;

namespace FCollada
{
	FCOLLADA_EXPORT void RunTests(FUTestBed& testBed)
	{
		// FMath tests
		testBed.RunTestSuite(::_testFMArray);
		testBed.RunTestSuite(::_testFMTree);
		testBed.RunTestSuite(::_testFMQuaternion);

		// FUtils tests
		testBed.RunTestSuite(::_testFUObject);
		testBed.RunTestSuite(::_testFUCrc32);
		testBed.RunTestSuite(::_testFUFunctor);
		testBed.RunTestSuite(::_testFUEvent);
		testBed.RunTestSuite(::_testFUString);
		testBed.RunTestSuite(::_testFUFileManager);
		testBed.RunTestSuite(::_testFUBoundingTest);
	}
};
#endif // RETAIL
