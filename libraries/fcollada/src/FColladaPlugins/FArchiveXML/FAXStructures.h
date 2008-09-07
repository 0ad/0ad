/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FAXSTRUCTURES_H_
#define _FAXSTRUCTURES_H_

class FCDObject;
class FCDENode;
class FCDEType;
class FCDEntity;
class FCDTargetedEntity;
class FCDTransform;
class FCDSceneNode;
class FCDEntityInstance;
class FCDEmitterInstance;
class FCDControllerInstance;
class FCDGeometryInstance;
class FCDSkinController;
class FCDAnimated;
class FCDAnimatedCustom;
class FCDAnimation;
class FCDAnimationChannel;
class FCDAnimationCurve;
class FCDAnimationMultiCurve;
class FCDPhysicsRigidBodyParameters;
class FCDPhysicsModel;
class FCDEffectParameter;
class FCDEffectParameterSurface;
class FCDEffectParameterSampler;
class FCDEffectStandard;
class FCDEffectProfileFX;
class FCDEffectTechnique;
class FCDTexture;
class FCDMaterial;
class FCDController;
class FCDSkinController;
class FCDMorphController;
class FCDGeometrySource;
class FCDGeometryPolygons;
class FCDGeometryMesh;
class FCDNURBSSpline;
class FCDSpline;
class FCDEmitterObject;
class FCDMeshPosition;
class FCDEmitterParticle;
class FCDParticleModifier;
class FCDPositionObject;
class FCDOrientationObject;
class FCDBasicRotationModifier;
class FCDColourTintModifier;
class FCDParticleAgeSizeModifier;
class FCDRandomAccelModifier;
class FCDRandomOffsetModifier;
class FCDRotationModifier;
class FCDTranslationModifier;
class FCDVelocityAlignModifier;
class FCDWorldAgeSizeModifier;
class FCDForceDrag;
class FCDForceDragDamping;
class FCDEmitter;
class FCDExternalReferenceManager;

typedef bool(* XMLLoadFunc)(FCDObject*, xmlNode* node);
typedef fm::map<const FUObjectType*, XMLLoadFunc> XMLLoadFuncMap;

typedef xmlNode* (* XMLWriteFunc)(FCDObject*, xmlNode* node);
typedef fm::map<const FUObjectType*, XMLWriteFunc> XMLWriteFuncMap;

//
// Define data structures to store intermediate data.
//

//
// For FCDTargetedEntity
//
struct FCDTargetedEntityData
{
	fm::string targetId;
};
typedef fm::map<FCDTargetedEntity*, FCDTargetedEntityData> FCDTargetedEntityDataMap;

//
// For FCDEmitterInstance
//
struct FCDEmitterInstanceData
{
	StringList forceInstUris;
};
typedef fm::map<FCDEmitterInstance*, FCDEmitterInstanceData> FCDEmitterInstanceDataMap;

//
// For FCDAnimated
//
struct FCDAnimatedData
{
	fm::string pointer;

	//
	//[sli 5-15-2007] The following variable could be still be in FCDAnimated.
	// To be figured out later.
	//
	//StringList qualifiers;
};
typedef fm::map<FCDAnimated*, FCDAnimatedData> FCDAnimatedDataMap;

//
// For FCDAnimationChannel
//
struct FAXAnimationChannelDefaultValue
{
	FCDAnimationCurve* curve; /**< An animation curve contained by this channel. */
	float defaultValue; /**< The default value for an animation value pointer that is not animated but may be merged. */
	
	/** Default constructor. */
	FAXAnimationChannelDefaultValue() : curve(NULL), defaultValue(0.0f) {}
	/** Simple constructor. @param c A curve. @param f The default value. @param q The default value's qualifier. */
	FAXAnimationChannelDefaultValue(FCDAnimationCurve* c, float f) { curve = c; defaultValue = f; }
};
typedef fm::vector<FAXAnimationChannelDefaultValue> FAXAnimationChannelDefaultValueList;

struct FCDAnimationChannelData
{
	// Channel target
	fm::string targetPointer;
	fm::string targetQualifier;

	// Maya-specific: the driver for this/these curves
	fm::string driverPointer;
	int32 driverQualifier;

	// Export parameters
	FAXAnimationChannelDefaultValueList defaultValues;
	FCDAnimated* animatedValue;

	FCDAnimationChannelData()
	{
		driverQualifier = -1;
	}
};
typedef fm::map<FCDAnimationChannel*, FCDAnimationChannelData> FCDAnimationChannelDataMap;

//
// For FCDAnimationCurve
//
struct FCDAnimationCurveData
{
	int32 targetElement;
	fm::string targetQualifier;

	FCDAnimationCurveData()
	{
		targetElement = -1;
	}
};
typedef fm::map<FCDAnimationCurve*, FCDAnimationCurveData> FCDAnimationCurveDataMap;

//
// For FCDAnimation
//
struct FCDAnimationData
{
	FAXNodeIdPairList childNodes; // import-only.
};
typedef fm::map<FCDAnimation*, FCDAnimationData> FCDAnimationDataMap;

//
// For FCDPhysicsModel
//
typedef fm::map<xmlNode*, FUUri> ModelInstanceNameNodeMap;
struct FCDPhysicsModelData
{
	ModelInstanceNameNodeMap modelInstancesMap;
};
typedef fm::map<FCDPhysicsModel*, FCDPhysicsModelData> FCDPhysicsModelDataMap;

//
// For FCDEffectParameterSampler
//
struct FCDEffectParameterSamplerData
{
	fm::string surfaceSid;
};
typedef fm::map<FCDEffectParameterSampler*, FCDEffectParameterSamplerData> FCDEffectParameterSamplerDataMap;

//
// For FCDTexture
//
struct FCDTextureData
{
	fm::string samplerSid;
};
typedef fm::map<FCDTexture*, FCDTextureData> FCDTextureDataMap;

//
// For FCDSkinController
//
struct FCDSkinControllerData
{
	bool jointAreSids;
};
typedef fm::map<FCDSkinController*, FCDSkinControllerData> FCDSkinControllerDataMap;

//
// For FCDMorphController
//
struct FCDMorphControllerData
{
	fm::string targetId;
};
typedef fm::map<FCDMorphController*, FCDMorphControllerData> FCDMorphControllerDataMap;

//
// For FCDGeometrySource
//
struct FCDGeometrySourceData
{
	xmlNode* sourceNode;
};
typedef fm::map<FCDGeometrySource*, FCDGeometrySourceData> FCDGeometrySourceDataMap;


typedef fm::pvector<FCDAnimationChannel> FCDAnimationChannelList;

#endif //_FAXSTRUCTURES_H_

//
// Per-document link data using in 2nd passing of loading.
//
struct FCDocumentLinkData
{
	FCDEmitterInstanceDataMap emitterInstanceDataMap;
	FCDTargetedEntityDataMap targetedEntityDataMap;
	FCDAnimationChannelDataMap animationChannelData;
	FCDAnimatedDataMap animatedData;
	FCDAnimationCurveDataMap animationCurveData;
	FCDAnimationDataMap animationData;
	FCDPhysicsModelDataMap physicsModelDataMap;
	FCDEffectParameterSamplerDataMap effectParameterSamplerDataMap;
	FCDTextureDataMap textureDataMap;
	FCDSkinControllerDataMap skinControllerDataMap;
	FCDMorphControllerDataMap morphControllerDataMap;
	FCDGeometrySourceDataMap geometrySourceDataMap;
};

typedef fm::map<const FCDocument*, FCDocumentLinkData> DocumentLinkDataMap;
