/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEmitter.h"
#include "FCDocument/FCDEmitterInstance.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDMaterialInstance.h"

//
// FCDEmitterInstance
//

ImplementObjectType(FCDEmitterInstance);
ImplementParameterObjectNoCtr(FCDEmitterInstance, FCDEntityInstance, forceInstances);
ImplementParameterObjectNoCtr(FCDEmitterInstance, FCDEntityInstance, emittedInstances);

FCDEmitterInstance::FCDEmitterInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType)
:	FCDEntityInstance(document, parent, entityType)
{
}

FCDEmitterInstance::~FCDEmitterInstance()
{
}

