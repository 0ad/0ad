/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDCamera.h"

bool FArchiveXML::LoadCamera(FCDObject* object, xmlNode* cameraNode)
{ 
	if (!FArchiveXML::LoadTargetedEntity(object, cameraNode)) return false;

	bool status = true;
	FCDCamera* camera = (FCDCamera*)object;
	if (!IsEquivalent(cameraNode->name, DAE_CAMERA_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_CAM_ELEMENT, cameraNode->line);
		return status;
	}

	FCDExtra* extra = camera->GetExtra();

	// COLLADA 1.4: Grab the <optics> element's techniques
	xmlNode* opticsNode = FindChildByType(cameraNode, DAE_OPTICS_ELEMENT);
	xmlNode* commonTechniqueNode = FindChildByType(opticsNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	if (opticsNode != NULL) FArchiveXML::LoadExtra(extra, opticsNode); // backward compatibility-only, the extra information has been moved into the <camera><extra> element.

	// Retrieve the <perspective> or <orthographic> element
	xmlNode* cameraContainerNode = NULL;
	xmlNode* cameraOthNode = FindChildByType(commonTechniqueNode, DAE_CAMERA_ORTHO_ELEMENT);
	xmlNode* cameraPersNode = FindChildByType(commonTechniqueNode, DAE_CAMERA_PERSP_ELEMENT);

	if (cameraOthNode != NULL) camera->SetProjectionType(FCDCamera::ORTHOGRAPHIC);
	if (cameraPersNode != NULL) camera->SetProjectionType(FCDCamera::PERSPECTIVE);

	cameraContainerNode = (cameraOthNode == NULL) ? cameraPersNode : cameraOthNode;

	// Check the necessary camera structures
	if ((cameraOthNode != NULL) && (cameraPersNode != NULL))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_CAM_PROG_TYPE, cameraContainerNode->line);
		return status;
	}
	
	if (cameraContainerNode == NULL)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_PARAM_ROOT_MISSING, cameraNode->line);
		return status;
	}
	

	// Setup the camera according to the type and its parameters
	// Retrieve all the camera parameters
	StringList parameterNames;
	xmlNodeList parameterNodes;
	FindParameters(cameraContainerNode, parameterNames, parameterNodes);

	size_t parameterCount = parameterNodes.size();
	for (size_t i = 0; i < parameterCount; ++i)
	{
		xmlNode* parameterNode = parameterNodes[i];
		const fm::string& parameterName = parameterNames[i];
		const char* parameterValue = ReadNodeContentDirect(parameterNode);

#define COMMON_CAM_PARAMETER(colladaParam, setFn, getFn) \
		if (parameterName == colladaParam) { \
			camera->setFn(FUStringConversion::ToFloat(parameterValue)); \
			FArchiveXML::LoadAnimatable(&camera->getFn(), parameterNode); } else

		// Process the camera parameters
		COMMON_CAM_PARAMETER(DAE_ZNEAR_CAMERA_PARAMETER, SetNearZ, GetNearZ)
		COMMON_CAM_PARAMETER(DAE_ZFAR_CAMERA_PARAMETER, SetFarZ, GetFarZ)
		COMMON_CAM_PARAMETER(DAE_XFOV_CAMERA_PARAMETER, SetFovX, GetFovX)
		COMMON_CAM_PARAMETER(DAE_YFOV_CAMERA_PARAMETER, SetFovY, GetFovY)
		COMMON_CAM_PARAMETER(DAE_XMAG_CAMERA_PARAMETER, SetMagX, GetMagX)
		COMMON_CAM_PARAMETER(DAE_YMAG_CAMERA_PARAMETER, SetMagY, GetMagY)
		COMMON_CAM_PARAMETER(DAE_ASPECT_CAMERA_PARAMETER, SetAspectRatio, GetAspectRatio)
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_CAM_PARAM, parameterNode->line);
		}

#undef COMMON_CAM_PARAMETER
	}

	camera->SetDirtyFlag(); 
	return status;
}



