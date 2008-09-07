/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDObject.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationKey.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationMultiCurve.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectCode.h"
#include "FCDocument/FCDEffectParameterSampler.h"
#include "FCDocument/FCDEffectParameterSurface.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectPass.h"
#include "FCDocument/FCDEffectPassShader.h"
#include "FCDocument/FCDEffectPassState.h"
#include "FCDocument/FCDEffectProfile.h"
#include "FCDocument/FCDEffectProfileFX.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDEffectTechnique.h"
#include "FCDocument/FCDEmitter.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDEmitterInstance.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDMaterialInstance.h"
#include "FCDocument/FCDPhysicsForceFieldInstance.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDPhysicsRigidConstraintInstance.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FCDocument/FCDMorphController.h"
#include "FCDocument/FCDObject.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationClip.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDImage.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDPhysicsAnalyticalGeometry.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidConstraint.h"
#include "FCDocument/FCDPhysicsScene.h"
#include "FCDocument/FCDPlaceHolder.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTargetedEntity.h"
#include "FCDocument/FCDCamera.h"
#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDParticleModifier.h"
#include "FCDocument/FCDPhysicsShape.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDTexture.h"
#include "FCDocument/FCDTransform.h"
#include "FCDocument/FCDParticleModifier.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDVersion.h"
#include "FUtils/FUXmlDocument.h"


//
// Constants
//

static const char* kArchivePluginExtensions[NUM_EXTENSIONS] = { "dae", "xml" };

//
// FArchiveXML
//

ImplementObjectType(FArchiveXML)
XMLLoadFuncMap FArchiveXML::xmlLoadFuncs;
XMLWriteFuncMap FArchiveXML::xmlWriteFuncs;

DocumentLinkDataMap FArchiveXML::documentLinkDataMap;
int FArchiveXML::loadedDocumentCount = 0;

FArchiveXML::FArchiveXML(void)
{
	Initialize();
}

FArchiveXML::~FArchiveXML(void)
{
	xmlLoadFuncs.clear();
	xmlWriteFuncs.clear();
}

bool FArchiveXML::AddExtraExtension(const char* ext)
{
	if (!IsExtensionSupported(ext))
	{
		extraExtensions.push_back(fm::string(ext));
		return true;
	}
	return false;
}

bool FArchiveXML::RemoveExtraExtension(const char* ext)
{
	StringList::iterator it = extraExtensions.begin();
	for (; it != extraExtensions.end(); ++it)
	{
		if (IsEquivalent(it->c_str(), ext))
		{
			extraExtensions.erase(it);
			return true;
		}
	}
	return false;
}

bool FArchiveXML::IsExtensionSupported(const char* ext)
{
	int extCount = NUM_EXTENSIONS + (int)extraExtensions.size();
	for (int i = 0; i < extCount; ++i)
	{
		const char* supported = GetSupportedExtensionAt(i);
		if (IsEquivalent(ext, supported))
			return true;
	}
	return false;
}

const char* FArchiveXML::GetSupportedExtensionAt(int index)
{
	if (index < NUM_EXTENSIONS)
	{
		return kArchivePluginExtensions[index];
	}
	else
	{
		index -= NUM_EXTENSIONS;
		if (index < (int)extraExtensions.size())
		{
			return extraExtensions[index].c_str();
		}
		else
		{
			return NULL;
		}
	}
}

void FArchiveXML::Initialize()
{
	if (xmlLoadFuncs.empty())
	{
		xmlLoadFuncs.insert(&FCDObject::GetClassType(), FArchiveXML::LoadObject);
		xmlLoadFuncs.insert(&FCDExtra::GetClassType(), FArchiveXML::LoadExtra);
		xmlLoadFuncs.insert(&FCDENode::GetClassType(), FArchiveXML::LoadExtraNode);
		xmlLoadFuncs.insert(&FCDETechnique::GetClassType(), FArchiveXML::LoadExtraTechnique);
		xmlLoadFuncs.insert(&FCDEType::GetClassType(), FArchiveXML::LoadExtraType);
		xmlLoadFuncs.insert(&FCDAsset::GetClassType(), FArchiveXML::LoadAsset);
		xmlLoadFuncs.insert(&FCDAssetContributor::GetClassType(), FArchiveXML::LoadAssetContributor);
		xmlLoadFuncs.insert(&FCDEntityReference::GetClassType(), FArchiveXML::LoadEntityReference);
		xmlLoadFuncs.insert(&FCDExternalReferenceManager::GetClassType(), FArchiveXML::LoadExternalReferenceManager);
		xmlLoadFuncs.insert(&FCDPlaceHolder::GetClassType(), FArchiveXML::LoadPlaceHolder);

		xmlLoadFuncs.insert(&FCDEntity::GetClassType(), FArchiveXML::LoadEntity);
		xmlLoadFuncs.insert(&FCDTargetedEntity::GetClassType(), FArchiveXML::LoadTargetedEntity);
		xmlLoadFuncs.insert(&FCDSceneNode::GetClassType(), FArchiveXML::LoadSceneNode);
		xmlLoadFuncs.insert(&FCDTransform::GetClassType(), FArchiveXML::LoadTransform);
		xmlLoadFuncs.insert(&FCDTLookAt::GetClassType(), FArchiveXML::LoadTransformLookAt);
		xmlLoadFuncs.insert(&FCDTMatrix::GetClassType(), FArchiveXML::LoadTransformMatrix);
		xmlLoadFuncs.insert(&FCDTRotation::GetClassType(), FArchiveXML::LoadTransformRotation);
		xmlLoadFuncs.insert(&FCDTScale::GetClassType(), FArchiveXML::LoadTransformScale);
		xmlLoadFuncs.insert(&FCDTSkew::GetClassType(), FArchiveXML::LoadTransformSkew);
		xmlLoadFuncs.insert(&FCDTTranslation::GetClassType(), FArchiveXML::LoadTransformTranslation);

		xmlLoadFuncs.insert(&FCDGeometrySource::GetClassType(), FArchiveXML::LoadGeometrySource);
		xmlLoadFuncs.insert(&FCDGeometryMesh::GetClassType(), FArchiveXML::LoadGeometryMesh);
		xmlLoadFuncs.insert(&FCDGeometry::GetClassType(), FArchiveXML::LoadGeometry);
		xmlLoadFuncs.insert(&FCDGeometryPolygons::GetClassType(), FArchiveXML::LoadGeometryPolygons);
		xmlLoadFuncs.insert(&FCDGeometrySpline::GetClassType(), FArchiveXML::LoadGeometrySpline);

		xmlLoadFuncs.insert(&FCDMorphController::GetClassType(), FArchiveXML::LoadMorphController);
		xmlLoadFuncs.insert(&FCDController::GetClassType(), FArchiveXML::LoadController);
		xmlLoadFuncs.insert(&FCDSkinController::GetClassType(), FArchiveXML::LoadSkinController);

		xmlLoadFuncs.insert(&FCDEntityInstance::GetClassType(), FArchiveXML::LoadEntityInstance);
		xmlLoadFuncs.insert(&FCDEmitterInstance::GetClassType(), FArchiveXML::LoadEmitterInstance);
		xmlLoadFuncs.insert(&FCDGeometryInstance::GetClassType(), FArchiveXML::LoadGeometryInstance);
		xmlLoadFuncs.insert(&FCDControllerInstance::GetClassType(), FArchiveXML::LoadControllerInstance);
		xmlLoadFuncs.insert(&FCDMaterialInstance::GetClassType(), FArchiveXML::LoadMaterialInstance);
		xmlLoadFuncs.insert(&FCDPhysicsForceFieldInstance::GetClassType(), FArchiveXML::LoadPhysicsForceFieldInstance);
		xmlLoadFuncs.insert(&FCDPhysicsModelInstance::GetClassType(), FArchiveXML::LoadPhysicsModelInstance);
		xmlLoadFuncs.insert(&FCDPhysicsRigidBodyInstance::GetClassType(), FArchiveXML::LoadPhysicsRigidBodyInstance);
		xmlLoadFuncs.insert(&FCDPhysicsRigidConstraintInstance::GetClassType(), FArchiveXML::LoadPhysicsRigidConstraintInstance);

		xmlLoadFuncs.insert(&FCDAnimationChannel::GetClassType(), FArchiveXML::LoadAnimationChannel);
		xmlLoadFuncs.insert(&FCDAnimationCurve::GetClassType(), FArchiveXML::LoadAnimationCurve);
		xmlLoadFuncs.insert(&FCDAnimationMultiCurve::GetClassType(), FArchiveXML::LoadAnimationMultiCurve);
		xmlLoadFuncs.insert(&FCDAnimation::GetClassType(), FArchiveXML::LoadAnimation);
		xmlLoadFuncs.insert(&FCDAnimationClip::GetClassType(), FArchiveXML::LoadAnimationClip);

		xmlLoadFuncs.insert(&FCDCamera::GetClassType(), FArchiveXML::LoadCamera);

		xmlLoadFuncs.insert(&FCDEffect::GetClassType(), FArchiveXML::LoadEffect);
		xmlLoadFuncs.insert(&FCDEffectCode::GetClassType(), FArchiveXML::LoadEffectCode);
		xmlLoadFuncs.insert(&FCDEffectParameter::GetClassType(), FArchiveXML::LoadEffectParameter);
		xmlLoadFuncs.insert(&FCDEffectParameterBool::GetClassType(), FArchiveXML::LoadEffectParameterBool);
		xmlLoadFuncs.insert(&FCDEffectParameterFloat::GetClassType(), FArchiveXML::LoadEffectParameterFloat);
		xmlLoadFuncs.insert(&FCDEffectParameterFloat2::GetClassType(), FArchiveXML::LoadEffectParameterFloat2);
		xmlLoadFuncs.insert(&FCDEffectParameterFloat3::GetClassType(), FArchiveXML::LoadEffectParameterFloat3);
		xmlLoadFuncs.insert(&FCDEffectParameterColor3::GetClassType(), FArchiveXML::LoadEffectParameterFloat3);
		xmlLoadFuncs.insert(&FCDEffectParameterInt::GetClassType(), FArchiveXML::LoadEffectParameterInt);
		xmlLoadFuncs.insert(&FCDEffectParameterMatrix::GetClassType(), FArchiveXML::LoadEffectParameterMatrix);
		xmlLoadFuncs.insert(&FCDEffectParameterSampler::GetClassType(), FArchiveXML::LoadEffectParameterSampler);
		xmlLoadFuncs.insert(&FCDEffectParameterString::GetClassType(), FArchiveXML::LoadEffectParameterString);
		xmlLoadFuncs.insert(&FCDEffectParameterSurface::GetClassType(), FArchiveXML::LoadEffectParameterSurface);
		xmlLoadFuncs.insert(&FCDEffectParameterVector::GetClassType(), FArchiveXML::LoadEffectParameterVector);
		xmlLoadFuncs.insert(&FCDEffectParameterColor4::GetClassType(), FArchiveXML::LoadEffectParameterVector);
		xmlLoadFuncs.insert(&FCDEffectPass::GetClassType(), FArchiveXML::LoadEffectPass);
		xmlLoadFuncs.insert(&FCDEffectPassShader::GetClassType(), FArchiveXML::LoadEffectPassShader);
		xmlLoadFuncs.insert(&FCDEffectPassState::GetClassType(), FArchiveXML::LoadEffectPassState);
		xmlLoadFuncs.insert(&FCDEffectProfile::GetClassType(), FArchiveXML::LoadEffectProfile);
		xmlLoadFuncs.insert(&FCDEffectProfileFX::GetClassType(), FArchiveXML::LoadEffectProfileFX);
		xmlLoadFuncs.insert(&FCDEffectStandard::GetClassType(), FArchiveXML::LoadEffectStandard);
		xmlLoadFuncs.insert(&FCDEffectTechnique::GetClassType(), FArchiveXML::LoadEffectTechnique);
		xmlLoadFuncs.insert(&FCDTexture::GetClassType(), FArchiveXML::LoadTexture);
		xmlLoadFuncs.insert(&FCDImage::GetClassType(), FArchiveXML::LoadImage);
		xmlLoadFuncs.insert(&FCDMaterial::GetClassType(), FArchiveXML::LoadMaterial);

		xmlLoadFuncs.insert(&FCDEmitter::GetClassType(), FArchiveXML::LoadEmitter);
		xmlLoadFuncs.insert(&FCDForceField::GetClassType(), FArchiveXML::LoadForceField);

		xmlLoadFuncs.insert(&FCDPhysicsAnalyticalGeometry::GetClassType(), FArchiveXML::LoadPhysicsAnalyticalGeometry);
		xmlLoadFuncs.insert(&FCDPASBox::GetClassType(), FArchiveXML::LoadPASBox);
		xmlLoadFuncs.insert(&FCDPASCapsule::GetClassType(), FArchiveXML::LoadPASCapsule);
		xmlLoadFuncs.insert(&FCDPASTaperedCapsule::GetClassType(), FArchiveXML::LoadPASTaperedCapsule);
		xmlLoadFuncs.insert(&FCDPASCylinder::GetClassType(), FArchiveXML::LoadPASCylinder);
		xmlLoadFuncs.insert(&FCDPASTaperedCylinder::GetClassType(), FArchiveXML::LoadPASTaperedCylinder);
		xmlLoadFuncs.insert(&FCDPASPlane::GetClassType(), FArchiveXML::LoadPASPlane);
		xmlLoadFuncs.insert(&FCDPASSphere::GetClassType(), FArchiveXML::LoadPASSphere);
		xmlLoadFuncs.insert(&FCDPhysicsMaterial::GetClassType(), FArchiveXML::LoadPhysicsMaterial);
		xmlLoadFuncs.insert(&FCDPhysicsModel::GetClassType(), FArchiveXML::LoadPhysicsModel);
		xmlLoadFuncs.insert(&FCDPhysicsRigidBody::GetClassType(), FArchiveXML::LoadPhysicsRigidBody);
		xmlLoadFuncs.insert(&FCDPhysicsRigidConstraint::GetClassType(), FArchiveXML::LoadPhysicsRigidConstraint);
		xmlLoadFuncs.insert(&FCDPhysicsScene::GetClassType(), FArchiveXML::LoadPhysicsScene);
		xmlLoadFuncs.insert(&FCDPhysicsShape::GetClassType(), FArchiveXML::LoadPhysicsShape);
		xmlLoadFuncs.insert(&FCDSpline::GetClassType(), FArchiveXML::LoadSpline);
		xmlLoadFuncs.insert(&FCDBezierSpline::GetClassType(), FArchiveXML::LoadBezierSpline);
		xmlLoadFuncs.insert(&FCDLinearSpline::GetClassType(), FArchiveXML::LoadLinearSpline);
		xmlLoadFuncs.insert(&FCDNURBSSpline::GetClassType(), FArchiveXML::LoadNURBSSpline);

		xmlLoadFuncs.insert(&FCDLight::GetClassType(), FArchiveXML::LoadLight);

	}

	if (xmlWriteFuncs.empty())
	{
		xmlWriteFuncs.insert(&FCDObject::GetClassType(), FArchiveXML::WriteObject);
		xmlWriteFuncs.insert(&FCDExtra::GetClassType(), FArchiveXML::WriteExtra);
		xmlWriteFuncs.insert(&FCDENode::GetClassType(), FArchiveXML::WriteExtraNode);
		xmlWriteFuncs.insert(&FCDETechnique::GetClassType(), FArchiveXML::WriteExtraTechnique);
		xmlWriteFuncs.insert(&FCDEType::GetClassType(), FArchiveXML::WriteExtraType);
		xmlWriteFuncs.insert(&FCDAsset::GetClassType(), FArchiveXML::WriteAsset);
		xmlWriteFuncs.insert(&FCDAssetContributor::GetClassType(), FArchiveXML::WriteAssetContributor);
		xmlWriteFuncs.insert(&FCDEntityReference::GetClassType(), FArchiveXML::WriteEntityReference);
		xmlWriteFuncs.insert(&FCDExternalReferenceManager::GetClassType(), FArchiveXML::WriteExternalReferenceManager);
		xmlWriteFuncs.insert(&FCDPlaceHolder::GetClassType(), FArchiveXML::WritePlaceHolder);

		xmlWriteFuncs.insert(&FCDEntity::GetClassType(), FArchiveXML::WriteEntity);
		xmlWriteFuncs.insert(&FCDTargetedEntity::GetClassType(), FArchiveXML::WriteTargetedEntity);
		xmlWriteFuncs.insert(&FCDSceneNode::GetClassType(), FArchiveXML::WriteSceneNode);
		xmlWriteFuncs.insert(&FCDTransform::GetClassType(), FArchiveXML::WriteTransform);
		xmlWriteFuncs.insert(&FCDTLookAt::GetClassType(), FArchiveXML::WriteTransformLookAt);
		xmlWriteFuncs.insert(&FCDTMatrix::GetClassType(), FArchiveXML::WriteTransformMatrix);
		xmlWriteFuncs.insert(&FCDTRotation::GetClassType(), FArchiveXML::WriteTransformRotation);
		xmlWriteFuncs.insert(&FCDTScale::GetClassType(), FArchiveXML::WriteTransformScale);
		xmlWriteFuncs.insert(&FCDTSkew::GetClassType(), FArchiveXML::WriteTransformSkew);
		xmlWriteFuncs.insert(&FCDTTranslation::GetClassType(), FArchiveXML::WriteTransformTranslation);

		xmlWriteFuncs.insert(&FCDGeometrySource::GetClassType(), FArchiveXML::WriteGeometrySource);
		xmlWriteFuncs.insert(&FCDGeometryMesh::GetClassType(), FArchiveXML::WriteGeometryMesh);
		xmlWriteFuncs.insert(&FCDGeometry::GetClassType(), FArchiveXML::WriteGeometry);
		xmlWriteFuncs.insert(&FCDGeometryPolygons::GetClassType(), FArchiveXML::WriteGeometryPolygons);
		xmlWriteFuncs.insert(&FCDGeometrySpline::GetClassType(), FArchiveXML::WriteGeometrySpline);

		xmlWriteFuncs.insert(&FCDMorphController::GetClassType(), FArchiveXML::WriteMorphController);
		xmlWriteFuncs.insert(&FCDController::GetClassType(), FArchiveXML::WriteController);
		xmlWriteFuncs.insert(&FCDSkinController::GetClassType(), FArchiveXML::WriteSkinController);

		xmlWriteFuncs.insert(&FCDEntityInstance::GetClassType(), FArchiveXML::WriteEntityInstance);
		xmlWriteFuncs.insert(&FCDEmitterInstance::GetClassType(), FArchiveXML::WriteEmitterInstance);
		xmlWriteFuncs.insert(&FCDGeometryInstance::GetClassType(), FArchiveXML::WriteGeometryInstance);
		xmlWriteFuncs.insert(&FCDControllerInstance::GetClassType(), FArchiveXML::WriteControllerInstance);
		xmlWriteFuncs.insert(&FCDMaterialInstance::GetClassType(), FArchiveXML::WriteMaterialInstance);
		xmlWriteFuncs.insert(&FCDPhysicsForceFieldInstance::GetClassType(), FArchiveXML::WritePhysicsForceFieldInstance);
		xmlWriteFuncs.insert(&FCDPhysicsModelInstance::GetClassType(), FArchiveXML::WritePhysicsModelInstance);
		xmlWriteFuncs.insert(&FCDPhysicsRigidBodyInstance::GetClassType(), FArchiveXML::WritePhysicsRigidBodyInstance);
		xmlWriteFuncs.insert(&FCDPhysicsRigidConstraintInstance::GetClassType(), FArchiveXML::WritePhysicsRigidConstraintInstance);

		xmlWriteFuncs.insert(&FCDAnimationChannel::GetClassType(), FArchiveXML::WriteAnimationChannel);
		xmlWriteFuncs.insert(&FCDAnimationCurve::GetClassType(), FArchiveXML::WriteAnimationCurve);
		xmlWriteFuncs.insert(&FCDAnimationMultiCurve::GetClassType(), FArchiveXML::WriteAnimationMultiCurve);
		xmlWriteFuncs.insert(&FCDAnimation::GetClassType(), FArchiveXML::WriteAnimation);
		xmlWriteFuncs.insert(&FCDAnimationClip::GetClassType(), FArchiveXML::WriteAnimationClip);

		xmlWriteFuncs.insert(&FCDCamera::GetClassType(), FArchiveXML::WriteCamera);

		xmlWriteFuncs.insert(&FCDEffect::GetClassType(), FArchiveXML::WriteEffect);
		xmlWriteFuncs.insert(&FCDEffectCode::GetClassType(), FArchiveXML::WriteEffectCode);
		xmlWriteFuncs.insert(&FCDEffectParameter::GetClassType(), FArchiveXML::WriteEffectParameter);
		xmlWriteFuncs.insert(&FCDEffectParameterBool::GetClassType(), FArchiveXML::WriteEffectParameterBool);
		xmlWriteFuncs.insert(&FCDEffectParameterFloat::GetClassType(), FArchiveXML::WriteEffectParameterFloat);
		xmlWriteFuncs.insert(&FCDEffectParameterFloat2::GetClassType(), FArchiveXML::WriteEffectParameterFloat2);
		xmlWriteFuncs.insert(&FCDEffectParameterFloat3::GetClassType(), FArchiveXML::WriteEffectParameterFloat3);
		xmlWriteFuncs.insert(&FCDEffectParameterColor3::GetClassType(), FArchiveXML::WriteEffectParameterFloat3);
		xmlWriteFuncs.insert(&FCDEffectParameterInt::GetClassType(), FArchiveXML::WriteEffectParameterInt);
		xmlWriteFuncs.insert(&FCDEffectParameterMatrix::GetClassType(), FArchiveXML::WriteEffectParameterMatrix);
		xmlWriteFuncs.insert(&FCDEffectParameterSampler::GetClassType(), FArchiveXML::WriteEffectParameterSampler);
		xmlWriteFuncs.insert(&FCDEffectParameterString::GetClassType(), FArchiveXML::WriteEffectParameterString);
		xmlWriteFuncs.insert(&FCDEffectParameterSurface::GetClassType(), FArchiveXML::WriteEffectParameterSurface);
		xmlWriteFuncs.insert(&FCDEffectParameterVector::GetClassType(), FArchiveXML::WriteEffectParameterVector);
		xmlWriteFuncs.insert(&FCDEffectParameterColor4::GetClassType(), FArchiveXML::WriteEffectParameterVector);
		xmlWriteFuncs.insert(&FCDEffectPass::GetClassType(), FArchiveXML::WriteEffectPass);
		xmlWriteFuncs.insert(&FCDEffectPassShader::GetClassType(), FArchiveXML::WriteEffectPassShader);
		xmlWriteFuncs.insert(&FCDEffectPassState::GetClassType(), FArchiveXML::WriteEffectPassState);
		xmlWriteFuncs.insert(&FCDEffectProfile::GetClassType(), FArchiveXML::WriteEffectProfile);
		xmlWriteFuncs.insert(&FCDEffectProfileFX::GetClassType(), FArchiveXML::WriteEffectProfileFX);
		xmlWriteFuncs.insert(&FCDEffectStandard::GetClassType(), FArchiveXML::WriteEffectStandard);
		xmlWriteFuncs.insert(&FCDEffectTechnique::GetClassType(), FArchiveXML::WriteEffectTechnique);
		xmlWriteFuncs.insert(&FCDTexture::GetClassType(), FArchiveXML::WriteTexture);
		xmlWriteFuncs.insert(&FCDImage::GetClassType(), FArchiveXML::WriteImage);
		xmlWriteFuncs.insert(&FCDMaterial::GetClassType(), FArchiveXML::WriteMaterial);

		xmlWriteFuncs.insert(&FCDEmitter::GetClassType(), FArchiveXML::WriteEmitter);
		xmlWriteFuncs.insert(&FCDForceField::GetClassType(), FArchiveXML::WriteForceField);

		xmlWriteFuncs.insert(&FCDPhysicsAnalyticalGeometry::GetClassType(), FArchiveXML::WritePhysicsAnalyticalGeometry);
		xmlWriteFuncs.insert(&FCDPASBox::GetClassType(), FArchiveXML::WritePASBox);
		xmlWriteFuncs.insert(&FCDPASCapsule::GetClassType(), FArchiveXML::WritePASCapsule);
		xmlWriteFuncs.insert(&FCDPASTaperedCapsule::GetClassType(), FArchiveXML::WritePASTaperedCapsule);
		xmlWriteFuncs.insert(&FCDPASCylinder::GetClassType(), FArchiveXML::WritePASCylinder);
		xmlWriteFuncs.insert(&FCDPASTaperedCylinder::GetClassType(), FArchiveXML::WritePASTaperedCylinder);
		xmlWriteFuncs.insert(&FCDPASPlane::GetClassType(), FArchiveXML::WritePASPlane);
		xmlWriteFuncs.insert(&FCDPASSphere::GetClassType(), FArchiveXML::WritePASSphere);
		xmlWriteFuncs.insert(&FCDPhysicsMaterial::GetClassType(), FArchiveXML::WritePhysicsMaterial);
		xmlWriteFuncs.insert(&FCDPhysicsModel::GetClassType(), FArchiveXML::WritePhysicsModel);
		xmlWriteFuncs.insert(&FCDPhysicsRigidBody::GetClassType(), FArchiveXML::WritePhysicsRigidBody);
		xmlWriteFuncs.insert(&FCDPhysicsRigidConstraint::GetClassType(), FArchiveXML::WritePhysicsRigidConstraint);
		xmlWriteFuncs.insert(&FCDPhysicsScene::GetClassType(), FArchiveXML::WritePhysicsScene);
		xmlWriteFuncs.insert(&FCDPhysicsShape::GetClassType(), FArchiveXML::WritePhysicsShape);

		xmlWriteFuncs.insert(&FCDLight::GetClassType(), FArchiveXML::WriteLight);

	}
}

void FArchiveXML::ClearIntermediateData()
{
	FArchiveXML::documentLinkDataMap.clear();
}

bool FArchiveXML::ImportFile(const fchar* filePath, FCDocument* fcdocument)
{
	bool status = true;

	fcdocument->SetFileUrl(fstring(filePath));

	_FTRY
	{
		// Parse the document into a XML tree
		FUXmlDocument daeDocument(fcdocument->GetFileManager(), fcdocument->GetFileUrl(), true);
		xmlNode* rootNode = daeDocument.GetRootNode();
		if (rootNode != NULL)
		{
			//fcdocument->GetFileManager()->PushRootFile(filePath);
			// Read in the whole document from the root node
			status &= (Import(fcdocument, rootNode));
			//fcdocument->GetFileManager()->PopRootFile();
		}
		else
		{
			status = false;
    		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MALFORMED_XML);
		}

		// Clean-up the XML reader
		xmlCleanupParser();
		FArchiveXML::ClearIntermediateData();
	}
	_FCATCH_ALL
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_PARSING_FAILED);
	}

	if (status) FUError::Error(FUError::DEBUG_LEVEL, FUError::DEBUG_LOAD_SUCCESSFUL);
	return status;
}

bool FArchiveXML::ImportFileFromMemory(const fchar* filePath, FCDocument* fcdocument, const void* contents, size_t length)
{
	bool status = true;

    _FTRY
    {
		fcdocument->SetFileUrl(fstring(filePath));

		fm::string textBuffer;
		const xmlChar* text = (const xmlChar*) contents;
		if (length != 0)
		{
			textBuffer = fm::string((const char*)contents, length);
			text = (const xmlChar*) textBuffer.c_str();
		}

		// Parse the document into a XML tree
		FUXmlDocument daeDocument((const char*) contents, length);
		xmlNode* rootNode = daeDocument.GetRootNode();
		if (rootNode != NULL)
		{
			// Read in the whole document from the root node
			status &= (Import(fcdocument, rootNode));
		}
		else
		{
			status = false;
			FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MALFORMED_XML);
		}

		// Clean-up the XML reader
		xmlCleanupParser();
		FArchiveXML::ClearIntermediateData();
    }
    _FCATCH_ALL
    {
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_PARSING_FAILED);
        status = false;
	}

	if (status) FUError::Error(FUError::DEBUG_LEVEL, FUError::DEBUG_LOAD_SUCCESSFUL);
	return status;	
}

bool FArchiveXML::ExportFile(FCDocument* fcdocument, const fchar* filePath)
{
	bool status = true;

	fcdocument->SetFileUrl(fstring(filePath));

	_FTRY
	{
		// Create a new XML document
		FUXmlDocument daeDocument(NULL, filePath, false);
		xmlNode* rootNode = daeDocument.CreateRootNode(DAE_COLLADA_ELEMENT);
		status = ExportDocument(fcdocument, rootNode);
		if (status)
		{
			// Create the XML document and write it out to the given filename
			if (!daeDocument.Write())
			{
				FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_WRITE_FILE, rootNode->line);
			}
			else
			{
				FUError::Error(FUError::DEBUG_LEVEL, FUError::DEBUG_WRITE_SUCCESSFUL);
			}
		}
	}
    _FCATCH_ALL
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_PARSING_FAILED);
	}

	return status;
}

// TODO: where should this go?
static FUXmlDocument daeDocument(NULL, NULL, false);

bool FArchiveXML::StartExport(const fchar* UNUSED(filePath))
{
	FUAssert(daeDocument.GetRootNode() == NULL, return false);

	daeDocument.CreateRootNode(DAE_COLLADA_ELEMENT);
	return true;
}

bool FArchiveXML::ExportObject(FCDObject* object)
{
	if (object == NULL) return false;
	FUAssert(daeDocument.GetRootNode() != NULL, return false);

    _FTRY
    {
		return WriteSwitch(object, &object->GetObjectType(), daeDocument.GetRootNode()) != NULL;
    }
    _FCATCH_ALL
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_PARSING_FAILED);
	}

	// If we get here, we've thrown an exception.
	return false;
}

bool FArchiveXML::EndExport(fm::vector<uint8>& outData)
{
	xmlNode* rootNode = daeDocument.GetRootNode();
	FUAssert(rootNode != NULL, return false);

	xmlOutputBufferPtr buf = xmlAllocOutputBuffer(NULL);
	xmlNodeDumpOutput(buf, rootNode->doc, rootNode, 0, 0, NULL);

	outData.resize(buf->buffer->use * sizeof(xmlChar));
	memcpy(outData.begin(), buf->buffer->content, outData.size());

	xmlOutputBufferClose(buf);
	daeDocument.ReleaseXmlData();
	return true;
}
bool FArchiveXML::EndExport(const fchar* UNUSED(filePath))
{
	return false;
}

bool FArchiveXML::ImportObject(FCDObject* object, const fm::vector<uint8>& data)
{
	FUXmlDocument loadDocument((const char*) data.begin(), data.size());
	bool retVal = LoadSwitch(object, &object->GetObjectType(), loadDocument.GetRootNode());
	if (FArchiveXML::loadedDocumentCount == 0)
		FArchiveXML::ClearIntermediateData();
	return retVal;
}

// Structure and enumeration used to order the libraries
enum nodeOrder { ANIMATION=0, ANIMATION_CLIP, IMAGE, EFFECT, MATERIAL, GEOMETRY, CONTROLLER, CAMERA, LIGHT, FORCE_FIELD, EMITTER, VISUAL_SCENE, PHYSICS_MATERIAL, PHYSICS_MODEL, PHYSICS_SCENE, UNKNOWN };
struct xmlOrderedNode { xmlNode* node; nodeOrder order; };
typedef fm::vector<xmlOrderedNode> xmlOrderedNodeList;

bool FArchiveXML::Import(FCDocument* theDocument, xmlNode* colladaNode)
{
	bool status = true;

	if (FArchiveXML::loadedDocumentCount == 0)
		FArchiveXML::ClearIntermediateData();
	++FArchiveXML::loadedDocumentCount;

	// The only root node supported is "COLLADA"
	if (!IsEquivalent(colladaNode->name, DAE_COLLADA_ELEMENT))
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_INVALID_ELEMENT, colladaNode->line);
		return status = false;
	}

	fm::string strVersion = ReadNodeProperty(colladaNode, DAE_VERSION_ATTRIBUTE);
	theDocument->GetVersion().ParseVersionNumbers(strVersion);

	// Bucket the libraries, so that we can read them in our specific order
	// COLLADA 1.4: the libraries are now strongly-typed, so process all the elements
	xmlNode* sceneNode = NULL;
	xmlOrderedNodeList orderedLibraryNodes;
	xmlNodeList extraNodes;
	for (xmlNode* child = colladaNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		xmlOrderedNode n;
		n.node = child;
		n.order = UNKNOWN;
		if (IsEquivalent(child->name, DAE_LIBRARY_ANIMATION_ELEMENT)) n.order = ANIMATION;
		else if (IsEquivalent(child->name, DAE_LIBRARY_ANIMATION_CLIP_ELEMENT)) n.order = ANIMATION_CLIP;
		else if (IsEquivalent(child->name, DAE_LIBRARY_CAMERA_ELEMENT)) n.order = CAMERA;
		else if (IsEquivalent(child->name, DAE_LIBRARY_CONTROLLER_ELEMENT)) n.order = CONTROLLER;
		else if (IsEquivalent(child->name, DAE_LIBRARY_EFFECT_ELEMENT)) n.order = EFFECT;
		else if (IsEquivalent(child->name, DAE_LIBRARY_GEOMETRY_ELEMENT)) n.order = GEOMETRY;
		else if (IsEquivalent(child->name, DAE_LIBRARY_IMAGE_ELEMENT)) n.order = IMAGE;
		else if (IsEquivalent(child->name, DAE_LIBRARY_LIGHT_ELEMENT)) n.order = LIGHT;
		else if (IsEquivalent(child->name, DAE_LIBRARY_MATERIAL_ELEMENT)) n.order = MATERIAL;
		else if (IsEquivalent(child->name, DAE_LIBRARY_VSCENE_ELEMENT)) n.order = VISUAL_SCENE;
		else if (IsEquivalent(child->name, DAE_LIBRARY_FFIELDS_ELEMENT)) n.order = FORCE_FIELD;
		else if (IsEquivalent(child->name, DAE_LIBRARY_NODE_ELEMENT)) n.order = VISUAL_SCENE; // Process them as visual scenes.
		else if (IsEquivalent(child->name, DAE_LIBRARY_PMATERIAL_ELEMENT)) n.order = PHYSICS_MATERIAL;
		else if (IsEquivalent(child->name, DAE_LIBRARY_PMODEL_ELEMENT)) n.order = PHYSICS_MODEL;
		else if (IsEquivalent(child->name, DAE_LIBRARY_PSCENE_ELEMENT)) n.order = PHYSICS_SCENE;
		else if (IsEquivalent(child->name, DAE_ASSET_ELEMENT)) 
		{
			// Read in the asset information
			status &= (FArchiveXML::LoadAsset(theDocument->GetAsset(), child));
			continue;
		}
		else if (IsEquivalent(child->name, DAE_SCENE_ELEMENT))
		{
			// The <scene> element should be the last element of the document
			sceneNode = child;
			continue;
		}
		else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT))
		{
			extraNodes.push_back(child);
		}
		else
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_BASE_NODE_TYPE, child->line);
			continue;
		}

		xmlOrderedNodeList::iterator it;
		for (it = orderedLibraryNodes.begin(); it != orderedLibraryNodes.end(); ++it)
		{
			if ((uint32) n.order < (uint32) (*it).order) break;
		}
		orderedLibraryNodes.insert(it, n);
	}

	// Look in the <extra> element for libaries
	for (xmlNodeList::iterator it = extraNodes.begin(); it != extraNodes.end(); ++it)
	{
		fm::string extraType = ReadNodeProperty(*it, DAE_TYPE_ATTRIBUTE);
		if (IsEquivalent(extraType, DAEFC_LIBRARIES_TYPE))
		{
			xmlNodeList techniqueNodes;
			FindChildrenByType(*it, DAE_TECHNIQUE_ELEMENT, techniqueNodes);
			for (xmlNodeList::iterator itT = techniqueNodes.begin(); itT != techniqueNodes.end(); ++itT)
			{
				for (xmlNode* child = (*itT)->children; child != NULL; child = child->next)
				{
					if (child->type != XML_ELEMENT_NODE) continue;

					xmlOrderedNode n;
					n.node = child;
					n.order = UNKNOWN;

					if (IsEquivalent(child->name, DAE_LIBRARY_EMITTER_ELEMENT)) n.order = EMITTER;
					else continue; // drop the node

					// Insert the library node at the correct place in the ordered list.
					xmlOrderedNodeList::iterator it;
					for (it = orderedLibraryNodes.begin(); it != orderedLibraryNodes.end(); ++it)
					{
						if ((uint32) n.order < (uint32) (*it).order) break;
					}
					orderedLibraryNodes.insert(it, n);
				}
			}
		}
		else
		{
			// Dump this extra in the document's extra.
			FArchiveXML::LoadExtra(theDocument->GetExtra(), *it);
		}
	}

	// Process the ordered libraries
	size_t libraryNodeCount = orderedLibraryNodes.size();
	for (size_t i = 0; i < libraryNodeCount; ++i)
	{
		if (FCollada::CancelLoading()) return false;

		xmlOrderedNode& n = orderedLibraryNodes[i];
		switch (n.order)
		{
		case ANIMATION: status &= (FArchiveXML::LoadAnimationLibrary(theDocument->GetAnimationLibrary(), n.node)); break;
		case ANIMATION_CLIP: status &= (FArchiveXML::LoadAnimationClipLibrary(theDocument->GetAnimationClipLibrary(), n.node)); break;
		case CAMERA: status &= (FArchiveXML::LoadCameraLibrary(theDocument->GetCameraLibrary(), n.node)); break;
		case CONTROLLER: status &= (FArchiveXML::LoadControllerLibrary(theDocument->GetControllerLibrary(), n.node)); break;
		case EFFECT: status &= (FArchiveXML::LoadEffectLibrary(theDocument->GetEffectLibrary(), n.node)); break;
		case EMITTER: status &= (FArchiveXML::LoadEmitterLibrary(theDocument->GetEmitterLibrary(), n.node)); break;
		case FORCE_FIELD: status &= (FArchiveXML::LoadForceFieldLibrary(theDocument->GetForceFieldLibrary(), n.node)); break;
		case GEOMETRY: status &= (FArchiveXML::LoadGeometryLibrary(theDocument->GetGeometryLibrary(), n.node)); break;
		case IMAGE: status &= (FArchiveXML::LoadImageLibrary(theDocument->GetImageLibrary(), n.node)); break;
		case LIGHT: status &= (FArchiveXML::LoadLightLibrary(theDocument->GetLightLibrary(), n.node)); break;
		case MATERIAL: status &= (FArchiveXML::LoadMaterialLibrary(theDocument->GetMaterialLibrary(), n.node)); break;
		case PHYSICS_MODEL: 
			{
				status &= (FArchiveXML::LoadPhysicsModelLibrary(theDocument->GetPhysicsModelLibrary(), n.node)); 
				size_t physicsModelCount = theDocument->GetPhysicsModelLibrary()->GetEntityCount();
				for (size_t physicsModelCounter = 0; physicsModelCounter < physicsModelCount; physicsModelCounter++)
				{
					FCDPhysicsModel* model = theDocument->GetPhysicsModelLibrary()->GetEntity(physicsModelCounter);
					status &= FArchiveXML::AttachModelInstancesFCDPhysicsModel(model);
				}
				break;
			}
		case PHYSICS_MATERIAL: status &= (FArchiveXML::LoadPhysicsMaterialLibrary(theDocument->GetPhysicsMaterialLibrary(), n.node)); break;
		case PHYSICS_SCENE: status &= (FArchiveXML::LoadPhysicsSceneLibrary(theDocument->GetPhysicsSceneLibrary(), n.node)); break;
		case VISUAL_SCENE: status &= (FArchiveXML::LoadVisualSceneNodeLibrary(theDocument->GetVisualSceneLibrary(), n.node)); break;
		case UNKNOWN: default: break;
		}
	}

	// Read in the <scene> element
	if (sceneNode != NULL)
	{
		bool oneVisualSceneInstanceFound = false;
		for (xmlNode* child = sceneNode->children; child != NULL; child = child->next)
		{
			if (child->type != XML_ELEMENT_NODE) continue;
			bool isVisualSceneInstance = IsEquivalent(child->name, DAE_INSTANCE_VSCENE_ELEMENT) && !oneVisualSceneInstanceFound;
			bool isPhysicsSceneInstance = IsEquivalent(child->name, DAE_INSTANCE_PHYSICS_SCENE_ELEMENT);
			if (!isVisualSceneInstance && !isPhysicsSceneInstance)
			{
				FUError::Error(FUError::WARNING_LEVEL, FUError::ERROR_INVALID_ELEMENT, child->line);
				continue;
			}

			FUUri instanceUri = ReadNodeUrl(child);
			if (instanceUri.GetFragment().empty())
			{
				FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_INVALID_URI, child->line);
			}
			else
			{
				FCDEntityReference* reference = (isVisualSceneInstance) ? theDocument->GetVisualSceneInstanceReference() : theDocument->AddPhysicsSceneInstanceReference();
				reference->SetUri(instanceUri);
				if (reference->IsLocal() && reference->GetEntity() == NULL)
				{
					FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_MISSING_URI_TARGET, child->line);
				}
			}
		}
	}

	// Link the effect surface parameters with the images
	for (size_t i = 0; i < theDocument->GetMaterialLibrary()->GetEntityCount(); ++i)
	{
		FArchiveXML::LinkMaterial(theDocument->GetMaterialLibrary()->GetEntity(i));
	}
	for (size_t i = 0; i < theDocument->GetEffectLibrary()->GetEntityCount(); ++i)
	{
		FArchiveXML::LinkEffect(theDocument->GetEffectLibrary()->GetEntity(i));
	}
	for (size_t i = 0; i < theDocument->GetControllerLibrary()->GetEntityCount(); ++i)
	{
		status |= FArchiveXML::LinkController(theDocument->GetControllerLibrary()->GetEntity(i));
	}

	for (size_t i = 0; i < theDocument->GetVisualSceneLibrary()->GetEntityCount(); i++)
	{
		FCDSceneNode* node = theDocument->GetVisualSceneLibrary()->GetEntity(i);
		status |= FArchiveXML::LinkSceneNode(node);
	}

	// Link the convex meshes with their point clouds (convex_hull_of)
	for (size_t i = 0; i < theDocument->GetGeometryLibrary()->GetEntityCount(); ++i)
	{
		FCDGeometryMesh* mesh = theDocument->GetGeometryLibrary()->GetEntity(i)->GetMesh();
		if (mesh) FArchiveXML::LinkGeometryMesh(mesh);
	}

	// Link the targeted entities, for 3dsMax cameras and lights
	size_t cameraCount = theDocument->GetCameraLibrary()->GetEntityCount();
	for (size_t i = 0; i < cameraCount; ++i)
	{
		FCDCamera* camera = theDocument->GetCameraLibrary()->GetEntity(i);
		FCDTargetedEntityDataMap::iterator it = FArchiveXML::documentLinkDataMap[theDocument].targetedEntityDataMap.find(camera);
		if (!it->second.targetId.empty())
		{
			status &= (FArchiveXML::LinkTargetedEntity(camera));
		}
	}
	size_t lightCount = theDocument->GetLightLibrary()->GetEntityCount();
	for (size_t i = 0; i < lightCount; ++i)
	{
		FCDLight* light = theDocument->GetLightLibrary()->GetEntity(i);
		FCDTargetedEntityDataMap::iterator it = FArchiveXML::documentLinkDataMap[theDocument].targetedEntityDataMap.find(light);
		if (!it->second.targetId.empty())
		{
			status &= (FArchiveXML::LinkTargetedEntity(light));
		}
	}

	// Check that all the animation curves that need them, have found drivers
	size_t animationCount = theDocument->GetAnimationLibrary()->GetEntityCount();
	for (size_t i = 0; i < animationCount; ++i)
	{
		FCDAnimation* animation = theDocument->GetAnimationLibrary()->GetEntity(i);
		status &= (FArchiveXML::LinkAnimation(animation));
	}

	if (!theDocument->GetFileUrl().empty())
	{
		// [staylor] Why is this done here?  Shouldn't it be in FCDExternalReferenceManager?
		// If it is, change it, either way delete the FUAssert (thanks)
		//FUAssert(false == true, ;);  
		FArchiveXML::RegisterLoadedDocument(theDocument);
		//FCDExternalReferenceManager::RegisterLoadedDocument(theDocument);
	}

	--FArchiveXML::loadedDocumentCount;
	return status;
}


bool FArchiveXML::ExportDocument(FCDocument* theDocument, xmlNode* colladaNode)
{
	bool status = true;

	if (FArchiveXML::loadedDocumentCount == 0)
		FArchiveXML::ClearIntermediateData();
	++FArchiveXML::loadedDocumentCount;

	if (colladaNode != NULL)
	{
		// Write the COLLADA document version and namespace: schema-required attributes
		AddAttribute(colladaNode, DAE_NAMESPACE_ATTRIBUTE, DAE_SCHEMA_LOCATION);
		AddAttribute(colladaNode, DAE_VERSION_ATTRIBUTE, DAE_SCHEMA_VERSION);

		// Write out the asset tag
		FArchiveXML::LetWriteObject(theDocument->GetAsset(), colladaNode);

		// Record the animation library. This library is built at the end, but should appear before the <scene> element.
		xmlNode* animationLibraryNode = NULL;
		if (!theDocument->GetAnimationLibrary()->IsEmpty())
		{
			animationLibraryNode = AddChild(colladaNode, DAE_LIBRARY_ANIMATION_ELEMENT);
		}

		// clean up the sub ids
#define CLEAN_LIBRARY_SUB_IDS(libraryName) if (!(libraryName)->IsEmpty()) { \
	size_t count = (libraryName)->GetEntityCount(); \
	for (size_t i = 0; i < count; i++) { \
	(libraryName)->GetEntity(i)->CleanSubId(); } }

		CLEAN_LIBRARY_SUB_IDS(theDocument->GetPhysicsSceneLibrary());
		CLEAN_LIBRARY_SUB_IDS(theDocument->GetPhysicsModelLibrary());
		CLEAN_LIBRARY_SUB_IDS(theDocument->GetVisualSceneLibrary());

#undef CLEAN_LIBRARY_SUB_IDS

		// Export the libraries
#define EXPORT_LIBRARY(memberName, daeElementName) if (!(memberName)->IsEmpty() || (memberName)->GetExtra()->HasContent()) { \
	xmlNode* libraryNode = AddChild(colladaNode, daeElementName); \
	FArchiveXML::WriteLibrary(memberName, libraryNode); }

		EXPORT_LIBRARY(theDocument->GetAnimationClipLibrary(), DAE_LIBRARY_ANIMATION_CLIP_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetPhysicsMaterialLibrary(), DAE_LIBRARY_PMATERIAL_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetForceFieldLibrary(), DAE_LIBRARY_FFIELDS_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetPhysicsModelLibrary(), DAE_LIBRARY_PMODEL_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetPhysicsSceneLibrary(), DAE_LIBRARY_PSCENE_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetCameraLibrary(), DAE_LIBRARY_CAMERA_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetLightLibrary(), DAE_LIBRARY_LIGHT_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetImageLibrary(), DAE_LIBRARY_IMAGE_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetMaterialLibrary(), DAE_LIBRARY_MATERIAL_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetEffectLibrary(), DAE_LIBRARY_EFFECT_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetGeometryLibrary(), DAE_LIBRARY_GEOMETRY_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetControllerLibrary(), DAE_LIBRARY_CONTROLLER_ELEMENT);
		EXPORT_LIBRARY(theDocument->GetVisualSceneLibrary(), DAE_LIBRARY_VSCENE_ELEMENT);

#undef EXPORT_LIBRARY

		xmlNode* sceneNode = NULL;
		if (theDocument->GetPhysicsSceneInstanceCount() > 0)
		{
			// Write out the <instance_physics_scene>
			if (sceneNode == NULL) sceneNode = AddChild(colladaNode, DAE_SCENE_ELEMENT);
			for (size_t i = 0; i < theDocument->GetPhysicsSceneInstanceCount(); ++i)
			{
				FCDEntityReference* reference = theDocument->GetPhysicsSceneInstanceReference(i);
				const FUUri& uri = reference->GetUri();
				fstring uriString = theDocument->GetFileManager()->CleanUri(uri);
				xmlNode* instanceVisualSceneNode = AddChild(sceneNode, DAE_INSTANCE_PHYSICS_SCENE_ELEMENT);
				AddAttribute(instanceVisualSceneNode, DAE_URL_ATTRIBUTE, uriString);
			}
		}
		if (theDocument->GetVisualSceneInstance() != NULL)
		{
			// Write out the <instance_visual_scene>
			if (sceneNode == NULL) sceneNode = AddChild(colladaNode, DAE_SCENE_ELEMENT);
			xmlNode* instanceVisualSceneNode = AddChild(sceneNode, DAE_INSTANCE_VSCENE_ELEMENT);
			const FUUri& uri = theDocument->GetVisualSceneInstanceReference()->GetUri();
			fstring uriString = theDocument->GetFileManager()->CleanUri(uri);
			AddAttribute(instanceVisualSceneNode, DAE_URL_ATTRIBUTE, uriString);
		}

		// Export the extra libraries
		if (theDocument->GetEmitterLibrary()->GetEntityCount() > 0)
		{
			xmlNode* typedExtraNode = AddChild(colladaNode, DAE_EXTRA_ELEMENT);
			AddAttribute(typedExtraNode, DAE_TYPE_ATTRIBUTE, DAEFC_LIBRARIES_TYPE);
			xmlNode* typedTechniqueNode = AddTechniqueChild(typedExtraNode, DAE_FCOLLADA_PROFILE);

			// Export the emitter library
			xmlNode* libraryNode = AddChild(typedTechniqueNode, DAE_LIBRARY_EMITTER_ELEMENT);

			if (!theDocument->GetEmitterLibrary()->GetTransientFlag()) 
				FArchiveXML::WriteLibrary(theDocument->GetEmitterLibrary(), libraryNode);
		}

		// Write out the animations
		if (animationLibraryNode != NULL)
		{
			if (!theDocument->GetAnimationLibrary()->GetTransientFlag()) 
				FArchiveXML::WriteLibrary(theDocument->GetAnimationLibrary(), animationLibraryNode);
		}

		// Write out the document extra.
		FArchiveXML::WriteExtra(theDocument->GetExtra(), colladaNode);
	}

	--FArchiveXML::loadedDocumentCount;

	return status;
}

bool FArchiveXML::LoadSwitch(FCDObject* object, const FUObjectType* objectType, xmlNode* node)
{
	XMLLoadFuncMap::iterator it = FArchiveXML::xmlLoadFuncs.find(objectType);
	if (it != FArchiveXML::xmlLoadFuncs.end())
	{
		return (*it->second)(object, node);
	}
	else
	{
		return false;
	}
}

template <class T>
bool FArchiveXML::LoadLibrary(FCDObject* object, xmlNode* node)
{
	FCDLibrary<T>* library = (FCDLibrary<T>*)object;

	bool status = true;
	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			if (IsEquivalent(child->name, DAE_ASSET_ELEMENT))
			{
				// Import the <asset> tag for this library.
				LoadAsset(library->GetAsset(true), child);
			}
			else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT))
			{
				// Import the <extra> tag for this library.
				LoadExtra(library->GetExtra(), child);
			}
			else
			{
				// Attempt to import this node as an entity of the library.
				T* entity = library->AddEntity();
				status &= (FArchiveXML::LoadSwitch(entity, &entity->GetObjectType(), child));
			}
		}

		if (FCollada::CancelLoading()) return false;
	}

	library->SetDirtyFlag();
	return status;
}

xmlNode* FArchiveXML::WriteSwitch(FCDObject* object, const FUObjectType* objectType, xmlNode* node)
{
	XMLWriteFuncMap::iterator it = FArchiveXML::xmlWriteFuncs.find(objectType);
	if (it != FArchiveXML::xmlWriteFuncs.end())
	{
		return (*it->second)(object, node);
	}
	else
	{
		return NULL;
	}
}

xmlNode* FArchiveXML::WriteParentSwitch(FCDObject* object, const FUObjectType* objectType, xmlNode* node)
{
	if (object->HasType(FCDObject::GetClassType()) && !object->IsType(FCDObject::GetClassType()))
	{
		return FArchiveXML::WriteSwitch(object, &objectType->GetParent(), node);
	}
	else 
	{
		FUBreak;
		return NULL;
	}
}

bool FArchiveXML::LoadAnimationLibrary(FCDObject* object, xmlNode* node)
{ 
	return FArchiveXML::LoadLibrary<FCDAnimation>(object, node);
}

bool FArchiveXML::LoadAnimationClipLibrary(FCDObject* object, xmlNode* node)
{
	return FArchiveXML::LoadLibrary<FCDAnimationClip>(object, node);
}

bool FArchiveXML::LoadCameraLibrary(FCDObject* object, xmlNode* node)
{ 
	return FArchiveXML::LoadLibrary<FCDCamera>(object, node);
}

bool FArchiveXML::LoadControllerLibrary(FCDObject* object, xmlNode* node)
{ 
	return FArchiveXML::LoadLibrary<FCDController>(object, node);
}

bool FArchiveXML::LoadEffectLibrary(FCDObject* object, xmlNode* node)
{ 
	return FArchiveXML::LoadLibrary<FCDEffect>(object, node);
}

bool FArchiveXML::LoadEmitterLibrary(FCDObject* object, xmlNode* node)
{
	return FArchiveXML::LoadLibrary<FCDEmitter>(object, node);
}

bool FArchiveXML::LoadForceFieldLibrary(FCDObject* object, xmlNode* node)
{ 
	return FArchiveXML::LoadLibrary<FCDForceField>(object, node);
}

bool FArchiveXML::LoadGeometryLibrary(FCDObject* object, xmlNode* node)
{
	return FArchiveXML::LoadLibrary<FCDGeometry>(object, node);
}

bool FArchiveXML::LoadImageLibrary(FCDObject* object, xmlNode* node)
{
	return FArchiveXML::LoadLibrary<FCDImage>(object, node);
}

bool FArchiveXML::LoadLightLibrary(FCDObject* object, xmlNode* node)
{
	return FArchiveXML::LoadLibrary<FCDLight>(object, node);
}

bool FArchiveXML::LoadMaterialLibrary(FCDObject* object, xmlNode* node)
{
	return FArchiveXML::LoadLibrary<FCDMaterial>(object, node);
}

bool FArchiveXML::LoadVisualSceneNodeLibrary(FCDObject* object, xmlNode* node)
{
	return FArchiveXML::LoadLibrary<FCDSceneNode>(object, node);
}

bool FArchiveXML::LoadPhysicsModelLibrary(FCDObject* object, xmlNode* node)
{
	return FArchiveXML::LoadLibrary<FCDPhysicsModel>(object, node);
}

bool FArchiveXML::LoadPhysicsMaterialLibrary(FCDObject* object, xmlNode* node)
{
	return FArchiveXML::LoadLibrary<FCDPhysicsMaterial>(object, node);
}

bool FArchiveXML::LoadPhysicsSceneLibrary(FCDObject* object, xmlNode* node)
{
	return FArchiveXML::LoadLibrary<FCDPhysicsScene>(object, node);
}

template <class T>
xmlNode* FArchiveXML::WriteLibrary(FCDLibrary<T>* library, xmlNode* node)
{
	// If present, write out the <asset>.
	FCDAsset* asset = library->GetAsset(false);
	if (asset != NULL) WriteAsset(asset, node);

	// Write out all the entities.
	for (size_t i = 0; i < library->GetEntityCount(); ++i)
	{
		T* entity = (T*)library->GetEntity(i);
		FArchiveXML::LetWriteObject(entity, node);
	}

	// Write out the extra tree.
	FArchiveXML::LetWriteObject(library->GetExtra(), node);
	return node;
}

#ifdef WIN32
#define PLUGIN_EXPORT __declspec(dllexport)
#else
#define PLUGIN_EXPORT
#endif

extern "C"
{
	PLUGIN_EXPORT uint32 GetPluginCount() { return 1; }
	PLUGIN_EXPORT const FUObjectType* GetPluginType(uint32) { return &FArchiveXML::GetClassType(); }
	PLUGIN_EXPORT FUPlugin* CreatePlugin(uint32) { return new FArchiveXML(); }
}
