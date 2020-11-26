/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDPlaceHolder.h"
#include "FUtils/FUFileManager.h"

//
// FCDEntityReference
//

ImplementObjectType(FCDEntityReference);

FCDEntityReference::FCDEntityReference(FCDocument* document, FCDObjectWithId* _parent)
:	FCDObject(document)
,	entity(NULL)
,	placeHolder(NULL)
,	baseObject(_parent)
{
	// No need to track base object - it must track us!
	// If it is deleted, it will delete us
	// (Also, we never use that ptr)
}

FCDEntityReference::~FCDEntityReference()
{
	SetPlaceHolder(NULL);

	UntrackObject(entity);
	entity = NULL;
}

FUUri FCDEntityReference::GetUri() const
{
	fstring path;
	if (placeHolder != NULL)
	{
		FUUri uri(placeHolder->GetFileUrl());
		path = uri.GetAbsoluteUri();
	}
	path.append(FC("#"));
	if (entity != NULL) path.append(TO_FSTRING(entity->GetDaeId()));
	else path.append(TO_FSTRING(entityId));
	return FUUri(path);
}

const FCDEntity* FCDEntityReference::GetEntity() const
{
	if (entity == NULL)
	{
		// This is the part Stephen doesn't like..
		const_cast<FCDEntityReference*>(this)->LoadEntity();
	}
	return entity;
}

void FCDEntityReference::SetUri(const FUUri& uri)
{
	entityId = TO_STRING(uri.GetFragment());
    entityId = FCDObjectWithId::CleanId(entityId);
	FCDPlaceHolder* documentPlaceHolder = NULL;
	
	if (uri.IsFile())
	{
		fstring fileUrl = GetDocument()->GetFileManager()->GetCurrentUri().MakeAbsolute(uri.GetAbsolutePath());
	
		documentPlaceHolder = GetDocument()->GetExternalReferenceManager()->FindPlaceHolder(fileUrl);
		if (documentPlaceHolder == NULL)
		{
			documentPlaceHolder = GetDocument()->GetExternalReferenceManager()->AddPlaceHolder(fileUrl);
		}
	}
	SetPlaceHolder(documentPlaceHolder);
	SetDirtyFlag();
}



void FCDEntityReference::SetEntity(FCDEntity* _entity)
{
	// Stop tracking the old entity
	if (entity != NULL) UntrackObject(entity);

	// Track the new entity
	entity = _entity;

	if (_entity != NULL)
	{
		TrackObject(_entity);
		entityId = _entity->GetDaeId();
		// Update the external references (this takes care of the placeHolder reference)
		SetEntityDocument(_entity->GetDocument());
	}
	else
	{
		SetEntityDocument(NULL);
	}

	SetNewChildFlag();
}


void FCDEntityReference::SetPlaceHolder(FCDPlaceHolder* _placeHolder)
{
	if (_placeHolder != placeHolder)
	{
		if (placeHolder != NULL)
		{
			placeHolder->RemoveExternalReference(this);
			UntrackObject(placeHolder);
			if (placeHolder->GetExternalReferenceCount() == 0)
			{
				SAFE_RELEASE(placeHolder);
			}
		}
		placeHolder = _placeHolder;
		if (placeHolder != NULL)
		{
			placeHolder->AddExternalReference(this);
			TrackObject(placeHolder);
		}
		SetNewChildFlag();
	}
}

void FCDEntityReference::LoadEntity()
{
	FCDocument* entityDocument;
	if (placeHolder == NULL) entityDocument = GetDocument();
	else
	{
		entityDocument = placeHolder->GetTarget(FCollada::GetDereferenceFlag());
	}

	if (entityDocument == NULL)
	{
		if (FCollada::GetDereferenceFlag())
		{
			FUError::Error(FUError::ERROR_LEVEL, FUError::WARNING_MISSING_URI_TARGET);
			FUFail(;);
		}
		return;
	}
	
	if (!entityId.empty())
	{
		entity = entityDocument->FindEntity(entityId);
		if (entity != NULL) TrackObject(entity);
		else
		{
			FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_INVALID_URI);
			FUFail(return);
		}
	}
}

void FCDEntityReference::SetEntityDocument(FCDocument* document)
{
	FCDPlaceHolder* documentPlaceHolder = NULL;
	if (document != NULL && document != GetDocument())
	{
		documentPlaceHolder = GetDocument()->GetExternalReferenceManager()->FindPlaceHolder(document);
		if (documentPlaceHolder == NULL)
		{
			documentPlaceHolder = GetDocument()->GetExternalReferenceManager()->AddPlaceHolder(document);
		}
	}
	SetPlaceHolder(documentPlaceHolder);
}

void FCDEntityReference::OnObjectReleased(FUTrackable* object)
{
	if (placeHolder == object)
	{
		placeHolder = NULL;
	}
	else if (entity == object)
	{
		if (placeHolder == NULL) entityId.clear();
		else entityId = ((FCDObjectWithId*) object)->GetDaeId();
		entity = NULL;
	}
	else { FUBreak; }
}
