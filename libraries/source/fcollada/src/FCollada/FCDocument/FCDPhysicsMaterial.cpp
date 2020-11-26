/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FUtils/FUStringConversion.h"

ImplementObjectType(FCDPhysicsMaterial);

FCDPhysicsMaterial::FCDPhysicsMaterial(FCDocument* document) : FCDEntity(document, "PhysicsMaterial")
{
	staticFriction = 0.f;
	dynamicFriction = 0.f;
	restitution = 0.f;
}

FCDPhysicsMaterial::~FCDPhysicsMaterial()
{
}

// Cloning
FCDEntity* FCDPhysicsMaterial::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPhysicsMaterial* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsMaterial(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPhysicsMaterial::GetClassType())) clone = (FCDPhysicsMaterial*) _clone;
	
	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->SetStaticFriction(staticFriction);
		clone->SetDynamicFriction(dynamicFriction);
		clone->SetRestitution(restitution);
	}
	return _clone;
}
