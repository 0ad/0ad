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
#include "FCDocument/FCDPhysicsForceFieldInstance.h"

ImplementObjectType(FCDPhysicsForceFieldInstance);

FCDPhysicsForceFieldInstance::FCDPhysicsForceFieldInstance(
		FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType)
:	FCDEntityInstance(document, parent, entityType)
{
}

FCDPhysicsForceFieldInstance::~FCDPhysicsForceFieldInstance()
{
}

FCDEntityInstance* FCDPhysicsForceFieldInstance::Clone(
		FCDEntityInstance* _clone) const
{
	FCDPhysicsForceFieldInstance* clone = NULL;
	if (_clone == NULL) clone = new FCDPhysicsForceFieldInstance(
			const_cast<FCDocument*>(GetDocument()), 
			const_cast<FCDSceneNode*>(GetParent()), GetEntityType());
	else if (!_clone->HasType(FCDPhysicsForceFieldInstance::GetClassType())) 
		return Parent::Clone(_clone);
	else clone = (FCDPhysicsForceFieldInstance*) _clone;

	Parent::Clone(clone);

	// nothing interesting in force field instance to copy

	return clone;
}
