/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationClip.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDCamera.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEmitter.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDImage.h"
#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsScene.h"
#include "FCDocument/FCDSceneNode.h"
#ifndef __APPLE__
#include "FCDocument/FCDLibrary.hpp"
#endif // __APPLE__

//
// FCDLibrary
//

typedef FCDLibrary<FCDAnimation> FCDAnimationLibrary; 
typedef FCDLibrary<FCDAnimationClip> FCDAnimationClipLibrary; 
typedef FCDLibrary<FCDCamera> FCDCameraLibrary; 
typedef FCDLibrary<FCDController> FCDControllerLibrary; 
typedef FCDLibrary<FCDEffect> FCDEffectLibrary; 
typedef FCDLibrary<FCDEmitter> FCDEmitterLibrary; 
typedef	FCDLibrary<FCDForceField> FCDForceFieldLibrary; 
typedef FCDLibrary<FCDGeometry> FCDGeometryLibrary; 
typedef FCDLibrary<FCDImage> FCDImageLibrary; 
typedef FCDLibrary<FCDLight> FCDLightLibrary; 
typedef FCDLibrary<FCDMaterial> FCDMaterialLibrary; 
typedef FCDLibrary<FCDSceneNode> FCDSceneNodeLibrary; 
typedef FCDLibrary<FCDPhysicsModel> FCDPhysicsModelLibrary; 
typedef FCDLibrary<FCDPhysicsMaterial> FCDPhysicsMaterialLibrary; 
typedef	FCDLibrary<FCDPhysicsScene> FCDPhysicsSceneLibrary;

ImplementObjectTypeT(FCDAnimationLibrary);
ImplementObjectTypeT(FCDAnimationClipLibrary);
ImplementObjectTypeT(FCDCameraLibrary);
ImplementObjectTypeT(FCDControllerLibrary);
ImplementObjectTypeT(FCDEffectLibrary);
ImplementObjectTypeT(FCDEmitterLibrary);
ImplementObjectTypeT(FCDForceFieldLibrary);
ImplementObjectTypeT(FCDGeometryLibrary);
ImplementObjectTypeT(FCDImageLibrary);
ImplementObjectTypeT(FCDLightLibrary);
ImplementObjectTypeT(FCDMaterialLibrary);
ImplementObjectTypeT(FCDSceneNodeLibrary);
ImplementObjectTypeT(FCDPhysicsModelLibrary);
ImplementObjectTypeT(FCDPhysicsMaterialLibrary);
ImplementObjectTypeT(FCDPhysicsSceneLibrary);

ImplementParameterObjectT(FCDAnimationLibrary, FCDAnimation, entities, new FCDAnimation(parent->GetDocument()));
ImplementParameterObjectT(FCDAnimationClipLibrary, FCDAnimationClip, entities, new FCDAnimationClip(parent->GetDocument()));
ImplementParameterObjectT(FCDCameraLibrary, FCDCamera, entities, new FCDCamera(parent->GetDocument()));
ImplementParameterObjectT(FCDControllerLibrary, FCDController, entities, new FCDController(parent->GetDocument()));
ImplementParameterObjectT(FCDEffectLibrary, FCDEffect, entities, new FCDEffect(parent->GetDocument()));
ImplementParameterObjectT(FCDEmitterLibrary, FCDEmitter, entities, new FCDEmitter(parent->GetDocument()));
ImplementParameterObjectT(FCDForceFieldLibrary, FCDForceField, entities, new FCDForceField(parent->GetDocument()));
ImplementParameterObjectT(FCDGeometryLibrary, FCDGeometry, entities, new FCDGeometry(parent->GetDocument()));
ImplementParameterObjectT(FCDImageLibrary, FCDImage, entities, new FCDImage(parent->GetDocument()));
ImplementParameterObjectT(FCDLightLibrary, FCDLight, entities, new FCDLight(parent->GetDocument()));
ImplementParameterObjectT(FCDMaterialLibrary, FCDMaterial, entities, new FCDMaterial(parent->GetDocument()));
ImplementParameterObjectT(FCDSceneNodeLibrary, FCDSceneNode, entities, new FCDSceneNode(parent->GetDocument()));
ImplementParameterObjectT(FCDPhysicsModelLibrary, FCDPhysicsModel, entities, new FCDPhysicsModel(parent->GetDocument()));
ImplementParameterObjectT(FCDPhysicsMaterialLibrary, FCDPhysicsMaterial, entities, new FCDPhysicsMaterial(parent->GetDocument()));
ImplementParameterObjectT(FCDPhysicsSceneLibrary, FCDPhysicsScene, entities, new FCDPhysicsScene(parent->GetDocument()));

ImplementParameterObjectT(FCDAnimationLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDAnimationClipLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDCameraLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDControllerLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDEffectLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDEmitterLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDForceFieldLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDGeometryLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDImageLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDLightLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDMaterialLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDSceneNodeLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDPhysicsModelLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDPhysicsMaterialLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObjectT(FCDPhysicsSceneLibrary, FCDAsset, asset, new FCDAsset(parent->GetDocument()));

ImplementParameterObjectT(FCDAnimationLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDAnimationClipLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDCameraLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDControllerLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDEffectLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDEmitterLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDForceFieldLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDGeometryLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDImageLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDLightLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDMaterialLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDSceneNodeLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDPhysicsModelLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDPhysicsMaterialLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));
ImplementParameterObjectT(FCDPhysicsSceneLibrary, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));

template<class T>
inline void LibraryExport()
{
	FCDLibrary<T>* l1 = new FCDLibrary<T>(NULL); 
	T* ptr = l1->AddEntity(); 
	l1->AddEntity(ptr);
	bool b = l1->IsEmpty(); 
	if (b) { ptr = l1->FindDaeId(emptyCharString); } 
	ptr = l1->GetEntity(23); 
	const T* cptr = ((const FCDLibrary<T>*)l1)->GetEntity(0);
	cptr = ptr;
	FCDAsset* asset = l1->GetAsset();
	asset->SetFlag(11);
}

FCOLLADA_EXPORT void TrickLinkerFCDLibrary()
{
	LibraryExport<FCDAnimation>();
	LibraryExport<FCDAnimationClip>();
	LibraryExport<FCDCamera>();
	LibraryExport<FCDController>();
	LibraryExport<FCDEffect>();
	LibraryExport<FCDEmitter>();
	LibraryExport<FCDForceField>();
	LibraryExport<FCDGeometry>();
	LibraryExport<FCDImage>();
	LibraryExport<FCDLight>();
	LibraryExport<FCDMaterial>();
	LibraryExport<FCDSceneNode>();
	LibraryExport<FCDPhysicsModel>();
	LibraryExport<FCDPhysicsMaterial>();
	LibraryExport<FCDPhysicsScene>();
}
