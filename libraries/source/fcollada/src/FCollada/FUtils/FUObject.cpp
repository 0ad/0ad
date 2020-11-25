/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUObject.h"
#include "FUObjectType.h"

//
// FUObject
//

FUObject::FUObject()
:	objectOwner(NULL)
{
}

FUObject::~FUObject()
{
	// If you trigger this assert, you are NOT using ->Release() properly.
	FUAssert(objectOwner == NULL, Detach());
}

// Releases this object. This function essentially calls the destructor.
void FUObject::Release()
{
	Detach();
	delete this;
}

void FUObject::Detach()
{
	if (objectOwner != NULL)
	{
		objectOwner->OnOwnedObjectReleased(this);
		objectOwner = NULL;
	}
}

FUObjectType __baseObjectType("FUObject");
FUObjectType* FUObject::baseObjectType = &__baseObjectType;

