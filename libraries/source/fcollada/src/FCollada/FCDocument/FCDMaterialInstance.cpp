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
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDMaterialInstance.h"

//
// FCDMaterialInstanceBind
//

ImplementObjectType(FCDMaterialInstanceBind);

FCDMaterialInstanceBind::FCDMaterialInstanceBind()
:	FUParameterizable()
,	InitializeParameterNoArg(semantic)
,	InitializeParameterNoArg(target)
{
}

FCDMaterialInstanceBind::~FCDMaterialInstanceBind()
{
}

//
// FCDMaterialInstanceBindVertexInput
//

ImplementObjectType(FCDMaterialInstanceBindVertexInput);

FCDMaterialInstanceBindVertexInput::FCDMaterialInstanceBindVertexInput()
:	FUParameterizable()
,	InitializeParameterNoArg(semantic)
,	InitializeParameter(inputSemantic, FUDaeGeometryInput::TEXCOORD)
,	InitializeParameter(inputSet, 0)
{
}

FCDMaterialInstanceBindVertexInput::~FCDMaterialInstanceBindVertexInput()
{
}

//
// FCDMaterialInstance
//

ImplementObjectType(FCDMaterialInstance);
ImplementParameterObjectNoArg(FCDMaterialInstance, FCDMaterialInstanceBind, bindings)
ImplementParameterObjectNoArg(FCDMaterialInstance, FCDMaterialInstanceBindVertexInput, vertexBindings)
ImplementParameterObjectNoArg(FCDMaterialInstance, FCDMaterialInstanceBindTextureSurface, texSurfBindings)

FCDMaterialInstance::FCDMaterialInstance(FCDocument* document, FCDEntityInstance* _parent)
:	FCDEntityInstance(document, _parent->GetParent(), FCDEntity::MATERIAL), parent(_parent)
,	InitializeParameterNoArg(semantic)
,	InitializeParameterNoArg(bindings)
,	InitializeParameterNoArg(vertexBindings)
{
}

FCDMaterialInstance::~FCDMaterialInstance()
{
	parent = NULL;
}

FCDObject* FCDMaterialInstance::GetGeometryTarget()
{
	if (parent != NULL && parent->GetEntity() != NULL)
	{
		FCDEntity* e = parent->GetEntity();
		if (e->HasType(FCDController::GetClassType()))
		{
			e = ((FCDController*) e)->GetBaseGeometry();
		}
		if (e->HasType(FCDGeometry::GetClassType()))
		{
			FCDGeometry* geometry = (FCDGeometry*) e;
			if (geometry->IsMesh())
			{
				FCDGeometryMesh* mesh = geometry->GetMesh();
				size_t polygonsCount = mesh->GetPolygonsCount();
				for (size_t i = 0; i < polygonsCount; ++i)
				{
					FCDGeometryPolygons* polygons = mesh->GetPolygons(i);
					if (IsEquivalent(polygons->GetMaterialSemantic(), semantic))
					{
						return polygons;
					}
				}
			}
		}
	}
	return NULL;
}


const FCDMaterialInstanceBind* FCDMaterialInstance::FindBinding(const char* semantic)
{
	for (const FCDMaterialInstanceBind** it = (const FCDMaterialInstanceBind**) bindings.begin(); it != bindings.end(); ++it)
	{
		if (IsEquivalent((*it)->semantic, semantic)) return (*it);
	}
	return NULL;
}

FCDMaterialInstanceBindVertexInput* FCDMaterialInstance::AddVertexInputBinding()
{
	FCDMaterialInstanceBindVertexInput* out = new FCDMaterialInstanceBindVertexInput();
	vertexBindings.push_back(out);
	SetNewChildFlag();
	return vertexBindings.back();
}

FCDMaterialInstanceBindVertexInput* FCDMaterialInstance::AddVertexInputBinding(const char* semantic, FUDaeGeometryInput::Semantic inputSemantic, int32 inputSet)
{
	FCDMaterialInstanceBindVertexInput* vbinding = AddVertexInputBinding();
	vbinding->semantic = semantic;
	vbinding->inputSemantic = inputSemantic;
	vbinding->inputSet = inputSet;
	return vbinding;
}

const FCDMaterialInstanceBindVertexInput* FCDMaterialInstance::FindVertexInputBinding(const char* semantic) const
{
	for (const FCDMaterialInstanceBindVertexInput** it = vertexBindings.begin(); it != vertexBindings.end(); ++it)
	{
		if (IsEquivalent((*it)->semantic, semantic)) return (*it);
	}
	return NULL;
}

FCDMaterialInstanceBind* FCDMaterialInstance::AddBinding()
{
	FCDMaterialInstanceBind* out = new FCDMaterialInstanceBind();
	bindings.push_back(out);
	SetNewChildFlag();
	return bindings.back();
}

FCDMaterialInstanceBind* FCDMaterialInstance::AddBinding(const char* semantic, const char* target)
{
	FCDMaterialInstanceBind* binding = AddBinding();
	binding->semantic = semantic;
	binding->target = target;
	return binding;
}

void FCDMaterialInstance::RemoveBinding(size_t index)
{
	FUAssert(index < bindings.size(), return);
	bindings.erase(index);
}

FCDEntityInstance* FCDMaterialInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDMaterialInstance* clone = NULL;
	if (_clone == NULL) clone = new FCDMaterialInstance(const_cast<FCDocument*>(GetDocument()), NULL);
	else if (!_clone->HasType(FCDMaterialInstance::GetClassType())) return Parent::Clone(_clone);
	else clone = (FCDMaterialInstance*) _clone;

	Parent::Clone(clone);

	// Clone the bindings and the semantic information.
	clone->semantic = semantic;
	size_t bindingCount = bindings.size();
	for (size_t b = 0; b < bindingCount; ++b)
	{
		const FCDMaterialInstanceBind* bind = bindings[b];
		clone->AddBinding(*bind->semantic, *bind->target);
	}
	bindingCount = vertexBindings.size();
	for (size_t b = 0; b < bindingCount; ++b)
	{
		const FCDMaterialInstanceBindVertexInput* bind = vertexBindings[b];
		clone->AddVertexInputBinding(*bind->semantic, (FUDaeGeometryInput::Semantic) *bind->inputSemantic, *bind->inputSet);
	}
	return clone;
}
