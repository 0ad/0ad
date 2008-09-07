/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCPARCHIVECOLLADA_H_
#define _FCPARCHIVECOLLADA_H_

class FCDParameterAnimatable;

#ifndef _FAXSTRUCTURES_H_
#include "FAXStructures.h"
#endif // _FAXSTRUCTURES_H_
#ifndef _FCOLLADA_PLUGIN_H_
#include "FColladaPlugin.h"
#endif // _FCOLLADA_PLUGIN_H_
#ifndef _FC_DOCUMENT_H_
#include "FCDocument/FCDocument.h"
#endif // _FC_DOCUMENT_H_
#ifndef _FCD_MATERIAL_STANDARD_H_
#include "FCDocument/FCDEffectStandard.h"
#endif // _FCD_MATERIAL_STANDARD_H_
#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

typedef fm::pvector<FCDEffectParameter> FCDEffectParameterList;

#define FCP_ARCHIVECOLLADA_NAME "XML Archive Plug-in"

#define NUM_EXTENSIONS 2

class FArchiveXML : public FCPArchive
{
private:
	DeclareObjectType(FCPArchive);

	//
	// Importer variables
	//
	static XMLLoadFuncMap xmlLoadFuncs;

	//
	// Exporter variables
	// 
	static XMLWriteFuncMap xmlWriteFuncs;

	//
	// Link data used in 2nd passing of loading.
	//
	static DocumentLinkDataMap documentLinkDataMap;
	static int loadedDocumentCount;

	//
	// Extra extension registration
	// These are useful when the DAE files are encapsulated within some
	// other (perhaps compressed) format.
	//
	StringList extraExtensions;

public:
	FArchiveXML();
	virtual ~FArchiveXML();

	/**
		See FColladaPlugin.h
	*/
	virtual bool IsImportSupported(){ return true; }
	virtual bool IsExportSupported(){ return true; }
	virtual bool IsPartialExportSupported(){ return true; }

	virtual bool IsExtensionSupported(const char* ext);
	virtual int GetSupportedExtensionsCount(){ return NUM_EXTENSIONS + (int)extraExtensions.size(); }
	virtual const char* GetSupportedExtensionAt(int index);

	virtual bool AddExtraExtension(const char* ext);
	virtual bool RemoveExtraExtension(const char* ext);

	virtual bool ImportFile(const fchar* filePath, FCDocument* fcdocument);
	virtual bool ImportFileFromMemory(const fchar* filePath, FCDocument* fcdocument, const void* contents, size_t length);

	virtual bool ExportFile(FCDocument* fcdocument, const fchar* filePath);

	virtual bool StartExport(const fchar* absoluteFilePath);
	virtual bool ExportObject(FCDObject* object);
	virtual bool EndExport(fm::vector<uint8>& outData);
	virtual bool EndExport(const fchar* filePath);

	virtual bool ImportObject(FCDObject* object, const fm::vector<uint8>& data);
	/** 
		See FUPlugin.h
	*/
	virtual const char* GetPluginName() const { return FCP_ARCHIVECOLLADA_NAME; }
	virtual uint32 GetPluginVersion() const { return 1; }

public:

	/**
		Initializes the plug-in. Add function pointers into the map.
	*/
	static void Initialize();

	/**
		Clears intermediate data used in 2nd pass of the loading/writing process
	*/
	static void ClearIntermediateData();

	/** 
		Imports the parsed xml data into the FCDocument.
		@param theDocument the FCDocument to be filled with imported data.
		@param colladaNode the xmlNode containing the parsed data.
		@return 'true' if the operation is successful.
	*/
	bool Import(FCDocument* theDocument, xmlNode* colladaNode);

	/**
		Export the existing FCOLLADA document to the given xml node.
		@param theDocument. The FCOLLADA document to be exported.
		@param colladaNode. The root of the xml tree to be filled with the document content.
		@return true if the document is exported correctly.
	*/
	bool ExportDocument(FCDocument* theDocument, xmlNode* colladaNode);

	/**
		Takes care of calling the right function to load the FCDObject
		@param object The base FCDObject
		@param objectType The correponding loading function will be called.
		@param node the xmlNode
		@return 'true' if the content is loaded correctly and 'false' otherwise.
	*/
	static bool LoadSwitch(FCDObject* object, const FUObjectType* objectType, xmlNode* node);

	/**
		Takes care of calling the right function to export the FCDObject
		@param object The base FCDObject
		@param objectType The correponding exporting function will be called.
		@param parentNode the parent xmlNode
		@return 'true' if the content is exported correctly and 'false' otherwise.
	*/
	static xmlNode* WriteSwitch(FCDObject* object, const FUObjectType* objectType, xmlNode* parentNode);
	//static bool ExportParamSwitch(FCDObject* object, const FUObjectType* objectType, xmlNode* node);

	/**
		Takes care of calling the right function to export the parent class information
		@param object the base FCDObject whose parent information is to be exported.
		@param objectType The correponding exporting function will be called for the parent of 'objectType'.
		@param parentNode the parent xmlNode
		@return 'true' if the content is exported correctly and 'false' otherwise.
	*/
	static xmlNode* WriteParentSwitch(FCDObject* object, const FUObjectType* objectType, xmlNode* parentNode);

	/**
		Functions to load FCOLLADA objects
	*/

	//
	// General functions
	//
	static bool LoadObject(FCDObject* object, xmlNode* node);
		
	static bool LoadExtra(FCDObject* object, xmlNode* node);	
	static bool LoadExtraNode(FCDObject* object, xmlNode* node);				
	static bool LoadExtraTechnique(FCDObject* object, xmlNode* node);	
	static bool LoadExtraType(FCDObject* object, xmlNode* node);	
	static bool LoadAsset(FCDObject* object, xmlNode* node);				
	static bool LoadAssetContributor(FCDObject* object, xmlNode* node);	
	static bool LoadEntityReference(FCDObject* object, xmlNode* node);	
	static bool LoadExternalReferenceManager(FCDObject* object, xmlNode* node); 
	static bool LoadPlaceHolder(FCDObject* object, xmlNode* node);			

	static void FindAnimationChannelsArrayIndices(FCDocument* fcdocument, xmlNode* targetArray, Int32List& animatedIndices);
	static void RegisterLoadedDocument(FCDocument* document);
	
	//
	// Scene graph related functions
	//
	static bool LoadEntity(FCDObject* object, xmlNode* node);	
	static bool LoadTargetedEntity(FCDObject* object, xmlNode* node);	
	static bool LoadSceneNode(FCDObject* object, xmlNode* node);
	static bool LoadTransform(FCDObject* object, xmlNode* node);	
	static bool LoadTransformLookAt(FCDObject* object, xmlNode* node);				
	static bool LoadTransformMatrix(FCDObject* object, xmlNode* node);				
	static bool LoadTransformRotation(FCDObject* object, xmlNode* node);			
	static bool LoadTransformScale(FCDObject* object, xmlNode* node);				
	static bool LoadTransformSkew(FCDObject* object, xmlNode* node);				
	static bool LoadTransformTranslation(FCDObject* object, xmlNode* node);

	static bool LoadFromExtraSceneNode(FCDSceneNode* sceneNode);
	static uint32 GetTransformType(xmlNode* node);

	//
	// Controller related functions
	//
	static bool LoadController(FCDObject* object, xmlNode* node);			
	static bool LoadSkinController(FCDObject* object, xmlNode* node);		
	static bool LoadMorphController(FCDObject* object, xmlNode* node);		
	static FCDSkinController* FindSkinController(FCDControllerInstance* controllerInstance, FCDEntity* entity);

	//
	// Instance related functions
	//
	static bool LoadEntityInstance(FCDObject* object, xmlNode* node);		
	static bool LoadEmitterInstance(FCDObject* object, xmlNode* node);		
	static bool LoadGeometryInstance(FCDObject* object, xmlNode* node);	
	static bool LoadControllerInstance(FCDObject* object, xmlNode* node);	
	static bool LoadMaterialInstance(FCDObject* object, xmlNode* node);	
	static bool LoadPhysicsForceFieldInstance(FCDObject* object, xmlNode* node); 
	static bool LoadPhysicsModelInstance(FCDObject* object, xmlNode* node); 
	static bool LoadPhysicsRigidBodyInstance(FCDObject* object, xmlNode* node); 
	static bool LoadPhysicsRigidConstraintInstance(FCDObject* object, xmlNode* node); 

	static bool LinkControllerInstance(FCDControllerInstance* controllerInstance);
	static bool LinkEmitterInstance(FCDEmitterInstance* emitterInstance);

	static bool ImportEmittedInstanceList(FCDEmitterInstance* emitterInstance, xmlNode* node);
	static uint32 GetEntityInstanceType(xmlNode* node);

	//
	// Material and effect related functions
	//
	static bool LoadMaterial(FCDObject* object, xmlNode* node);
	static bool LoadEffectCode(FCDObject* object, xmlNode* node);			
	static bool LoadEffectParameter(FCDObject* object, xmlNode* node);
	static bool LoadEffectParameterBool(FCDObject* object, xmlNode* node); 
	static bool LoadEffectParameterFloat(FCDObject* object, xmlNode* node); 
	static bool LoadEffectParameterFloat2(FCDObject* object, xmlNode* node); 
	static bool LoadEffectParameterFloat3(FCDObject* object, xmlNode* node); 
	static bool LoadEffectParameterInt(FCDObject* object, xmlNode* node); 
	static bool LoadEffectParameterMatrix(FCDObject* object, xmlNode* node); 
	static bool LoadEffectParameterSampler(FCDObject* object, xmlNode* node); 
	static bool LoadEffectParameterString(FCDObject* object, xmlNode* node); 
	static bool LoadEffectParameterSurface(FCDObject* object, xmlNode* node); 
	static bool LoadEffectParameterVector(FCDObject* object, xmlNode* node); 
	static bool LoadEffectPass(FCDObject* object, xmlNode* node);			
	static bool LoadEffectPassShader(FCDObject* object, xmlNode* node);	
	static bool LoadEffectPassState(FCDObject* object, xmlNode* node);		
	static bool LoadEffectProfile(FCDObject* object, xmlNode* node);		
	static bool LoadEffectProfileFX(FCDObject* object, xmlNode* node);		
	static bool LoadEffectStandard(FCDObject* object, xmlNode* node);		
	static bool LoadEffectTechnique(FCDObject* object, xmlNode* node);	
	static bool LoadEffect(FCDObject* object, xmlNode* node);				
	static bool LoadTexture(FCDObject* object, xmlNode* node);				
	static bool LoadImage(FCDObject* object, xmlNode* node);				

	static bool ParseColorTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parameterNode, FCDEffectParameterColor4* value, uint32 bucketIndex);
	static bool ParseFloatTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parameterNode, FCDEffectParameterFloat* value, uint32 bucketIndex);
	static bool ParseSimpleTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parameterNode, uint32 bucketIndex);
	static uint32 GetEffectParameterType(xmlNode* parameterNode);

	//
	// Animation related functions
	//
	static bool LoadAnimated(FCDObject* object, xmlNode* node);
	static bool LoadAnimationChannel(FCDObject* object, xmlNode* node);	
	static bool LoadAnimationCurve(FCDObject* object, xmlNode* node);		
	static bool LoadAnimationMultiCurve(FCDObject* object, xmlNode* node);	
	static bool LoadAnimation(FCDObject* object, xmlNode* node);			
	static bool LoadAnimationClip(FCDObject* object, xmlNode* node);	


	//
	// Camera related functions
	//
	static bool LoadCamera(FCDObject* object, xmlNode* node);	

	//
	// Light related functions
	//
	static bool LoadLight(FCDObject* object, xmlNode* node);

	//
	// Geometry related functions
	//
	static bool LoadGeometrySource(FCDObject* object, xmlNode* node);		
	static bool LoadGeometryMesh(FCDObject* object, xmlNode* node);		
	static bool LoadGeometryNURBSSurface(FCDObject* object, xmlNode* node); 
	static bool LoadGeometry(FCDObject* object, xmlNode* node);			
	static bool LoadGeometryPolygons(FCDObject* object, xmlNode* node);
	static bool LoadGeometrySpline(FCDObject* object, xmlNode* node);		
	static bool LoadSpline(FCDObject* object, xmlNode* node);				
	static bool LoadBezierSpline(FCDObject* object, xmlNode* node);		
	static bool LoadLinearSpline(FCDObject* object, xmlNode* node);		
	static bool LoadNURBSSpline(FCDObject* object, xmlNode* node);	

	static void SetTypeFCDGeometrySource(FCDGeometrySource* geometrySource, FUDaeGeometryInput::Semantic type);

	//
	// Physics related functions
	//
	static bool LoadPhysicsRigidBodyParameters(FCDPhysicsRigidBodyParameters* parameters, xmlNode* techniqueNode, FCDPhysicsRigidBodyParameters* defaultParameters = NULL);
	static bool AttachModelInstancesFCDPhysicsModel(FCDPhysicsModel* physicsModel);

	static bool LoadPhysicsShape(FCDObject* object, xmlNode* node);		
	static bool LoadPhysicsAnalyticalGeometry(FCDObject* object, xmlNode* node); 
	static bool LoadPASBox(FCDObject* object, xmlNode* node);				
	static bool LoadPASCapsule(FCDObject* object, xmlNode* node);			
	static bool LoadPASTaperedCapsule(FCDObject* object, xmlNode* node);	
	static bool LoadPASCylinder(FCDObject* object, xmlNode* node);			
	static bool LoadPASTaperedCylinder(FCDObject* object, xmlNode* node);	
	static bool LoadPASPlane(FCDObject* object, xmlNode* node);			
	static bool LoadPASSphere(FCDObject* object, xmlNode* node);			
	static bool LoadPhysicsMaterial(FCDObject* object, xmlNode* node);		
	static bool LoadPhysicsModel(FCDObject* object, xmlNode* node);		
	static bool LoadPhysicsRigidBody(FCDObject* object, xmlNode* node);	
	static bool LoadPhysicsRigidConstraint(FCDObject* object, xmlNode* node); 
	static bool LoadPhysicsScene(FCDObject* object, xmlNode* node);		

	//
	// Emitter related functions
	//
	static bool LoadEmitter(FCDObject* object, xmlNode* node);				

	//
	// Force related functions
	//
	static bool LoadForceField(FCDObject* object, xmlNode* node);			
			
	//
	// Library related functions
	//
	template <class T> static bool LoadLibrary(FCDObject* object, xmlNode* node);
	static bool LoadAnimationLibrary(FCDObject* object, xmlNode* node);
	static bool LoadAnimationClipLibrary(FCDObject* object, xmlNode* node);
	static bool LoadCameraLibrary(FCDObject* object, xmlNode* node);
	static bool LoadControllerLibrary(FCDObject* object, xmlNode* node);
	static bool LoadEffectLibrary(FCDObject* object, xmlNode* node);
	static bool LoadEmitterLibrary(FCDObject* object, xmlNode* node);
	static bool LoadForceFieldLibrary(FCDObject* object, xmlNode* node);
	static bool LoadGeometryLibrary(FCDObject* object, xmlNode* node);
	static bool LoadImageLibrary(FCDObject* object, xmlNode* node);
	static bool LoadLightLibrary(FCDObject* object, xmlNode* node);
	static bool LoadMaterialLibrary(FCDObject* object, xmlNode* node);
	static bool LoadVisualSceneNodeLibrary(FCDObject* object, xmlNode* node);
	static bool LoadPhysicsModelLibrary(FCDObject* object, xmlNode* node);
	static bool LoadPhysicsMaterialLibrary(FCDObject* object, xmlNode* node);
	static bool LoadPhysicsSceneLibrary(FCDObject* object, xmlNode* node);

	//
	// General helper functions
	//
	static bool LoadExtraNodeChildren(FCDENode* fcdenode, xmlNode* customNode);

	//
	// Animation helper functions
	//
	static bool ProcessChannels(FCDAnimated* animated, FCDAnimationChannelList& channels);
	static void LoadAnimatable(FCDParameterAnimatable* animatable, xmlNode* node);
	static void LoadAnimatable(FCDocument* document, FCDParameterListAnimatable* animatable, xmlNode* node);

	static xmlNode* FindChildByIdFCDAnimation(FCDAnimation* animation, const fm::string& _id);

	static void FindAnimationChannels(FCDocument* fcdocument, const fm::string& pointer, FCDAnimationChannelList& channels);
	static void FindAnimationChannels(FCDAnimation* animation, const fm::string& pointer, FCDAnimationChannelList& targetChannels);

	static FCDAnimatedCustom* CreateFCDAnimatedCustom(FCDObject* document, xmlNode* node);


	//
	// Linking functions used in the 2nd pass.
	//
	static bool LinkDriver(FCDocument* fcdoument, FCDAnimated* animated, const fm::string& animatedTargetPointer);
	static bool LinkDriver(FCDAnimation* animation, FCDAnimated* animated, const fm::string& animatedTargetPointer);
	static bool LinkDriver(FCDAnimationChannel* animationChannel, FCDAnimated* animated, const fm::string& animatedTargetPointer);
	static bool LinkAnimated(FCDAnimated* animated, xmlNode* node);
	static bool LinkAnimatedCustom(FCDAnimatedCustom* animatedCustom, xmlNode* node);
	static bool LinkAnimation(FCDAnimation* animation);

	static bool LinkTargetedEntity(FCDTargetedEntity* targetedEntity);
	static bool LinkSceneNode(FCDSceneNode* sceneNode);

	static void LinkMaterial(FCDMaterial* material);
	static void LinkEffect(FCDEffect* effect);
	static void LinkEffectParameterSurface(FCDEffectParameterSurface* effectParameterSurface);
	static void LinkEffectParameterSampler(FCDEffectParameterSampler* effectParameterSampler, FCDEffectParameterList& parameters);
	static void LinkEffectProfile(FCDEffectProfile* effectProfile);
	static void LinkEffectProfileFX(FCDEffectProfileFX* effectProfileFX);
	static void LinkEffectStandard(FCDEffectStandard* effectStandard);
	static void LinkEffectTechnique(FCDEffectTechnique* effectTechnique);
	static void LinkTexture(FCDTexture* texture, FCDEffectParameterList& effectParameters);

	static bool LinkController(FCDController* controller);
	static bool LinkSkinController(FCDSkinController* skinController);
	static bool LinkMorphController(FCDMorphController* morphController);

	static bool LinkGeometryMesh(FCDGeometryMesh* geometryMesh);


	/** 
		Functions to export FCOLLADA objects
	*/

	//
	// General related functions
	//
	static xmlNode* WriteObject(FCDObject* object, xmlNode* parentNode);
	static xmlNode* WriteExtraNode(FCDObject* object, xmlNode* parentNode);
	static xmlNode* WriteExtra(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteExtraTechnique(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteExtraType(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteAsset(FCDObject* object, xmlNode* parentNode);				
	static xmlNode* WriteAssetContributor(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteEntityReference(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteExternalReferenceManager(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WritePlaceHolder(FCDObject* object, xmlNode* parentNode);			

	static void WriteChildrenFCDENode(FCDENode* eNode, xmlNode* customNode);
	static void WriteTechniquesFCDEType(FCDEType* eType, xmlNode* parentNode);
	static void WriteTechniquesFCDExtra(FCDExtra* extra, xmlNode* parentNode);

	static xmlNode* LetWriteObject(FCDObject* object, void* entityNode) 
	{ 
		if (!object->GetTransientFlag()) 
			return FArchiveXML::WriteSwitch(object, &object->GetObjectType(), (xmlNode*) entityNode); 
		return NULL;  
	}

	//
	// Scene graph related functions
	//
	static xmlNode* WriteEntity(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteTargetedEntity(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteSceneNode(FCDObject* object, xmlNode* parentNode);
	static xmlNode* WriteTransform(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteTransformLookAt(FCDObject* object, xmlNode* parentNode);				
	static xmlNode* WriteTransformMatrix(FCDObject* object, xmlNode* parentNode);				
	static xmlNode* WriteTransformRotation(FCDObject* object, xmlNode* parentNode);			
	static xmlNode* WriteTransformScale(FCDObject* object, xmlNode* parentNode);				
	static xmlNode* WriteTransformSkew(FCDObject* object, xmlNode* parentNode);				
	static xmlNode* WriteTransformTranslation(FCDObject* object, xmlNode* parentNode);

	static xmlNode* WriteToEntityXMLFCDEntity(FCDEntity* entity, xmlNode* parentNode, const char* nodeName, bool writeId = true);
	static void WriteEntityExtra(FCDEntity* entity, xmlNode* entityNode);
	static void WriteEntityInstanceExtra(FCDEntityInstance* entityInstance, xmlNode* instanceNode);
	static void WriteTargetedEntityExtra(FCDTargetedEntity* targetedEntity, xmlNode* entityNode);
	static void WriteVisualScene(FCDSceneNode* sceneNode, xmlNode* parentNode);
	static void WriteTransformBase(FCDTransform* transform, xmlNode* transformNode, const char* wantedSid);

	//
	// Controller related functions
	//
	static xmlNode* WriteController(FCDObject* object, xmlNode* parentNode);			
	static xmlNode* WriteSkinController(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WriteMorphController(FCDObject* object, xmlNode* parentNode);		


	//
	// Instance related functions
	//
	static xmlNode* WriteEntityInstance(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WriteEmitterInstance(FCDObject* object, xmlNode* parentNode);
	static xmlNode* WriteSpriteInstance(FCDEntityInstance* object, xmlNode* parentNode);
	static xmlNode* WriteGeometryInstance(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteControllerInstance(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteMaterialInstance(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WritePhysicsForceFieldInstance(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WritePhysicsModelInstance(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WritePhysicsRigidBodyInstance(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WritePhysicsRigidConstraintInstance(FCDObject* object, xmlNode* parentNode); 

	//
	// Material and effect related functions
	//
	static xmlNode* WriteMaterial(FCDObject* object, xmlNode* parentNode);
	static xmlNode* WriteEffectCode(FCDObject* object, xmlNode* parentNode);			
	static xmlNode* WriteEffectParameter(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WriteEffectParameterBool(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WriteEffectParameterFloat(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WriteEffectParameterFloat2(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WriteEffectParameterFloat3(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WriteEffectParameterInt(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WriteEffectParameterMatrix(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WriteEffectParameterSampler(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WriteEffectParameterString(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WriteEffectParameterSurface(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WriteEffectParameterVector(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WriteEffectPass(FCDObject* object, xmlNode* parentNode);			
	static xmlNode* WriteEffectPassShader(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteEffectPassState(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WriteEffectProfile(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WriteEffectProfileFX(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WriteEffectStandard(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WriteEffectTechnique(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteEffect(FCDObject* object, xmlNode* parentNode);				
	static xmlNode* WriteTexture(FCDObject* object, xmlNode* parentNode);				
	static xmlNode* WriteImage(FCDObject* object, xmlNode* parentNode);				

	static xmlNode* WriteColorTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parentNode, const char* parameterNodeName, const FCDEffectParameterColor4* value, uint32 bucketIndex);
	static xmlNode* WriteFloatTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parentNode, const char* parameterNodeName, const FCDEffectParameterFloat* value, uint32 bucketIndex);
	static xmlNode* WriteTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parentNode, uint32 bucketIndex);

	//
	// Animation related functions
	//
	static xmlNode* WriteAnimated(FCDObject* object, xmlNode* parentNode);
	static xmlNode* WriteAnimationChannel(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteAnimationCurve(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WriteAnimationMultiCurve(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WriteAnimation(FCDObject* object, xmlNode* parentNode);			
	static xmlNode* WriteAnimationClip(FCDObject* object, xmlNode* parentNode);	

	static bool WriteAnimatedValue(const FCDParameterAnimatable* value, xmlNode* valueNode, const char* wantedSid, int32 arrayElement = -1);
	static void WriteAnimatedValue(const FCDAnimated* _animated, xmlNode* valueNode, const char* wantedSid);

	static void WriteSourceFCDAnimationCurve(FCDAnimationCurve* animationCurve, xmlNode* parentNode, const fm::string& baseId);
	static xmlNode* WriteSamplerFCDAnimationCurve(FCDAnimationCurve* animationCurve, xmlNode* parentNode, const fm::string& baseId);
	static xmlNode* WriteChannelFCDAnimationCurve(FCDAnimationCurve* animationCurve, xmlNode* parentNode, const fm::string& baseId, const char* targetPointer);
	static void WriteSourceFCDAnimationMultiCurve(FCDAnimationMultiCurve* animationMultiCurve, xmlNode* parentNode, const char** qualifiers, const fm::string& baseId);
	static xmlNode* WriteSamplerFCDAnimationMultiCurve(FCDAnimationMultiCurve* animationMultiCurve, xmlNode* parentNode, const fm::string& baseId);
	static xmlNode* WriteChannelFCDAnimationMultiCurve(FCDAnimationMultiCurve* animationMultiCurve, xmlNode* parentNode, const fm::string& baseId, const fm::string& pointer);

	//
	// Camera related functions
	//
	static xmlNode* WriteCamera(FCDObject* object, xmlNode* parentNode);	

	//
	// Light related functions
	//
	static xmlNode* WriteLight(FCDObject* object, xmlNode* parentNode);

	//
	// Geometry related functions
	//
	static xmlNode* WriteGeometry(FCDObject* object, xmlNode* parentNode);			
	static xmlNode* WriteGeometrySource(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WriteGeometryMesh(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WriteGeometryPolygons(FCDObject* object, xmlNode* parentNode);
	static xmlNode* WriteGeometrySpline(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WriteNURBSSpline(FCDNURBSSpline* nURBSSpline, xmlNode* parentNode, const fm::string& parentId, const fm::string& splineId);
	static xmlNode* WriteSpline(FCDSpline* spline, xmlNode* parentNode, const fm::string& parentId, const fm::string& splineId);


	//
	// Physics related functions
	//
	static xmlNode* WritePhysicsShape(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WritePhysicsAnalyticalGeometry(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WritePASBox(FCDObject* object, xmlNode* parentNode);				
	static xmlNode* WritePASCapsule(FCDObject* object, xmlNode* parentNode);			
	static xmlNode* WritePASTaperedCapsule(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WritePASCylinder(FCDObject* object, xmlNode* parentNode);			
	static xmlNode* WritePASTaperedCylinder(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WritePASPlane(FCDObject* object, xmlNode* parentNode);			
	static xmlNode* WritePASSphere(FCDObject* object, xmlNode* parentNode);			
	static xmlNode* WritePhysicsMaterial(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WritePhysicsModel(FCDObject* object, xmlNode* parentNode);		
	static xmlNode* WritePhysicsRigidBody(FCDObject* object, xmlNode* parentNode);	
	static xmlNode* WritePhysicsRigidConstraint(FCDObject* object, xmlNode* parentNode); 
	static xmlNode* WritePhysicsScene(FCDObject* object, xmlNode* parentNode);		

	static void WritePhysicsRigidBodyParameters(FCDPhysicsRigidBodyParameters* physicsRigidBodyParameters, xmlNode* techniqueNode);
	template <class TYPE, int QUAL>
	static xmlNode* AddPhysicsParameter(xmlNode* parentNode, const char* name, FCDParameterAnimatableT<TYPE,QUAL>& value);


	//
	// Emitter related functions
	//
	static xmlNode* WriteEmitter(FCDObject* object, xmlNode* parentNode);				

	//
	// Force related functions
	//
	static xmlNode* WriteForceField(FCDObject* object, xmlNode* parentNode);	

	//
	// Library related functions
	//
	template <class T>
	static xmlNode* WriteLibrary(FCDLibrary<T>* library, xmlNode* node);
};

#endif //_FCPARCHIVECOLLADA_H_
