/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDPlaceHolder.h"
#include "FUtils/FUFileManager.h"

//
// FCDExternalReferenceManager
//

ImplementObjectType(FCDExternalReferenceManager);

FCDExternalReferenceManager::FCDExternalReferenceManager(FCDocument* document)
:	FCDObject(document)
{
}

FCDExternalReferenceManager::~FCDExternalReferenceManager()
{
}

FCDPlaceHolder* FCDExternalReferenceManager::AddPlaceHolder(const fstring& _fileUrl)
{
	fstring fileUrl = GetDocument()->GetFileManager()->GetCurrentUri().MakeAbsolute(_fileUrl);
	FCDPlaceHolder* holder = placeHolders.Add(GetDocument());
	holder->SetFileUrl(fileUrl);
	SetNewChildFlag();
	return holder;
}

const FCDPlaceHolder* FCDExternalReferenceManager::FindPlaceHolder(const fstring& _fileUrl) const
{
	fstring fileUrl = GetDocument()->GetFileManager()->GetCurrentUri().MakeAbsolute(_fileUrl);
	for (const FCDPlaceHolder** it = placeHolders.begin(); it != placeHolders.end(); ++it)
	{
		if ((*it)->GetFileUrl() == fileUrl) return *it;
	}
	return NULL;
}

FCDPlaceHolder* FCDExternalReferenceManager::AddPlaceHolder(FCDocument* document)
{
	FCDPlaceHolder* placeHolder = placeHolders.Add(GetDocument(), document);
	SetNewChildFlag();
	return placeHolder;
}

const FCDPlaceHolder* FCDExternalReferenceManager::FindPlaceHolder(const FCDocument* document) const
{
	for (const FCDPlaceHolder** it = placeHolders.begin(); it != placeHolders.end(); ++it)
	{
		if ((*it)->GetTarget() == document) return *it;
	}
	return NULL;
}

void FCDExternalReferenceManager::RegisterLoadedDocument(FCDocument* document)
{
	fm::pvector<FCDocument> allDocuments;
	FCollada::GetAllDocuments(allDocuments);
	for (FCDocument** it = allDocuments.begin(); it != allDocuments.end(); ++it)
	{
		if ((*it) != document)
		{
			FCDExternalReferenceManager* xrefManager = (*it)->GetExternalReferenceManager();

			for (FCDPlaceHolder** itP = xrefManager->placeHolders.begin(); itP != xrefManager->placeHolders.end(); ++itP)
			{
				// Set the document to the placeholders that targets it.
				if ((*itP)->GetFileUrl() == document->GetFileUrl()) (*itP)->LoadTarget(document);
			}
		}
	}

	// On the newly-loaded document, there may be placeholders to process.
	FCDExternalReferenceManager* xrefManager = document->GetExternalReferenceManager();
	for (FCDPlaceHolder** itP = xrefManager->placeHolders.begin(); itP != xrefManager->placeHolders.end(); ++itP)
	{
		// Set the document to the placeholders that targets it.
		for (FCDocument** itD = allDocuments.begin(); itD != allDocuments.end(); ++itD)
		{
			if ((*itP)->GetFileUrl() == (*itD)->GetFileUrl()) (*itP)->LoadTarget(*itD);
		}
	}
}
