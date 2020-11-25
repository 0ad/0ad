/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDPlaceHolder.h"
#include "FUtils/FUFileManager.h"

//
// FCDPlaceHolder
//

ImplementObjectType(FCDPlaceHolder);

FCDPlaceHolder::FCDPlaceHolder(FCDocument* document, FCDocument* _target)
:	FCDObject(document)
,	target(_target)
{
	if (target != NULL)
	{
		TrackObject(target);
		fileUrl = target->GetFileUrl();
	}
}

FCDPlaceHolder::~FCDPlaceHolder()
{
	if (target != NULL)
	{
		UntrackObject(target);
		if (target->GetTrackerCount() == 0)
		{
			target->Release();
		}
	}
}

const fstring& FCDPlaceHolder::GetFileUrl() const 
{
	return (target != NULL) ? target->GetFileUrl() : fileUrl;
}

void FCDPlaceHolder::SetFileUrl(const fstring& url)
{
	fileUrl = url;
	SetDirtyFlag();
}

FCDocument* FCDPlaceHolder::GetTarget(bool loadIfMissing)
{
	if (target == NULL && loadIfMissing) LoadTarget(NULL);
	return target;
}

void FCDPlaceHolder::LoadTarget(FCDocument* newTarget)
{
	if (target == NULL)
	{
		if (newTarget == NULL)
		{
			newTarget = new FCDocument();
			FUUri uri(GetDocument()->GetFileManager()->GetCurrentUri().MakeAbsolute(fileUrl));
			fstring filename = uri.GetAbsolutePath();

#ifdef _DEBUG
			// Check for circular dependencies.
			FCDocumentList documents;
			FCollada::GetAllDocuments(documents);
			for (FCDocument** it = documents.begin(); it != documents.end(); ++it)
			{
				// If the following asset triggers, you are wrongly forcing XRefs to load during archiving ?
				FUAssert(!IsEquivalent((*it)->GetFileUrl(), fileUrl),);
			}
#endif // _DEBUG

			// Now, we have to copy over our callback schemes from our document to the new
			// one (to ensure that the new document can handle the scheme we was loaded under)
			FCDocument* curDoc = GetDocument();
			newTarget->GetFileManager()->CloneSchemeCallbacks(curDoc->GetFileManager());

			bool loadStatus = FCollada::LoadDocumentFromFile(newTarget, filename.c_str());
			if (!loadStatus)
			{
				SAFE_DELETE(newTarget);
			}
		}

		if (newTarget != NULL)
		{
			if (target != NULL)
			{
				fileUrl = target->GetFileUrl();
				UntrackObject(target);
				target = NULL;
			}
			target = newTarget;
			TrackObject(target);
		}
		SetNewChildFlag();
	}
}

void FCDPlaceHolder::UnloadTarget()
{
	SAFE_RELEASE(target);
	SetNewChildFlag();
}

void FCDPlaceHolder::OnObjectReleased(FUTrackable* object)
{
	if (object == target)
	{
		fileUrl = target->GetFileUrl();
		target = NULL;
	}
}
