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
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTargetedEntity.h"

//
// FCDTargetedEntity
//

ImplementObjectType(FCDTargetedEntity);
ImplementParameterObjectNoCtr(FCDTargetedEntity, FCDSceneNode, targetNode)

FCDTargetedEntity::FCDTargetedEntity(FCDocument* document, const char* className)
:	FCDEntity(document, className)
,	InitializeParameterNoArg(targetNode)
{
}

FCDTargetedEntity::~FCDTargetedEntity()
{
}

// Sets a new target
void FCDTargetedEntity::SetTargetNode(FCDSceneNode* target)
{
	if (targetNode != NULL)
	{
		targetNode->DecrementTargetCount();
	}

	targetNode = target;

	if (targetNode != NULL)
	{
		targetNode->IncrementTargetCount();
	}

	SetNewChildFlag();
}

FCDEntity* FCDTargetedEntity::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	if (_clone == NULL)
	{
		_clone = new FCDTargetedEntity(const_cast<FCDocument*>(GetDocument()), "TargetedEntity");
	}

	// Clone the base class variables
	FCDEntity::Clone(_clone, cloneChildren);

	if (_clone->HasType(FCDTargetedEntity::GetClassType()))
	{
		FCDTargetedEntity* clone = (FCDTargetedEntity*) _clone;
		// Copy the target information over.
		clone->SetTargetNode(const_cast<FCDSceneNode*>((const FCDSceneNode*)targetNode));
	}

	return _clone;
}
