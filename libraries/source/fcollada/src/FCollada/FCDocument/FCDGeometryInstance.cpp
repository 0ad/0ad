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
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FCDocument/FCDGeometryNURBSSurface.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDMaterialInstance.h"
#include "FUtils/FUUniqueStringMap.h"

//
// FCDGeometryInstance
//

ImplementObjectType(FCDGeometryInstance);
ImplementParameterObject(FCDGeometryInstance, FCDMaterialInstance, materials, new FCDMaterialInstance(parent->GetDocument(), parent))
ImplementParameterObjectNoCtr(FCDGeometryInstance, FCDEffectParameter, parameters)

FCDGeometryInstance::FCDGeometryInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType)
:	FCDEntityInstance(document, parent, entityType)
,	InitializeParameterNoArg(materials)
,	InitializeParameterNoArg(parameters)
{
}

FCDGeometryInstance::~FCDGeometryInstance()
{
}

FCDEffectParameter* FCDGeometryInstance::AddEffectParameter(uint32 type)
{
	FCDEffectParameter* parameter = FCDEffectParameterFactory::Create(GetDocument(), type);
	parameters.push_back(parameter);
	SetNewChildFlag();
	return parameter;
}

// Access Bound Materials
const FCDMaterialInstance* FCDGeometryInstance::FindMaterialInstance(const fchar* semantic) const
{
	for (const FCDMaterialInstance** itB = materials.begin(); itB != materials.end(); ++itB)
	{
		if ((*itB)->GetSemantic() == semantic) return (*itB);
	}
	return NULL;
}

FCDMaterialInstance* FCDGeometryInstance::AddMaterialInstance()
{
	FCDMaterialInstance* instance = new FCDMaterialInstance(GetDocument(), this);
	materials.push_back(instance);
	SetNewChildFlag();
	return instance;
}

FCDMaterialInstance* FCDGeometryInstance::AddMaterialInstance(FCDMaterial* material, FCDGeometryPolygons* polygons)
{
	FCDMaterialInstance* instance = AddMaterialInstance();
	instance->SetMaterial(material);
	if (polygons != NULL)
	{
		const fstring& semantic = polygons->GetMaterialSemantic();
		if (!semantic.empty())
		{
			instance->SetSemantic(polygons->GetMaterialSemantic());
		}
		else
		{
			// Generate a semantic.
			fstring semantic = TO_FSTRING(material->GetDaeId()) + TO_FSTRING(polygons->GetFaceOffset());
			polygons->SetMaterialSemantic(semantic);
			instance->SetSemantic(semantic);
		}
	}
	return instance;
}


FCDMaterialInstance* FCDGeometryInstance::AddMaterialInstance(FCDMaterial* material, const fchar* semantic)
{
	FCDMaterialInstance* instance = AddMaterialInstance();
	instance->SetMaterial(material);
	instance->SetSemantic(semantic);
	return instance;
}

FCDEntityInstance* FCDGeometryInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDGeometryInstance* clone = NULL;
	if (_clone == NULL) clone = new FCDGeometryInstance(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(GetParent()), GetEntityType());
	else if (!_clone->HasType(FCDGeometryInstance::GetClassType())) return Parent::Clone(_clone);
	else clone = (FCDGeometryInstance*) _clone;

	Parent::Clone(clone);

	size_t parameterCount = parameters.size();
	for (size_t p = 0; p < parameterCount; ++p)
	{
		FCDEffectParameter* clonedParameter = clone->AddEffectParameter(parameters[p]->GetType());
		parameters[p]->Clone(clonedParameter);
	}

	// Clone the material instances.
	for (const FCDMaterialInstance** it = materials.begin(); it != materials.end(); ++it)
	{
		FCDMaterialInstance* materialInstance = clone->AddMaterialInstance();
		(*it)->Clone(materialInstance);
	}

	return clone;
}

void FCDGeometryInstance::CleanSubId(FUSUniqueStringMap* parentStringMap)
{
	Parent::CleanSubId(parentStringMap);
	FUSUniqueStringMap myStringMap;

	size_t materialCount = materials.size();
	for (size_t i = 0; i < materialCount; ++i)
	{
		materials[i]->CleanSubId(&myStringMap);
	}
}
