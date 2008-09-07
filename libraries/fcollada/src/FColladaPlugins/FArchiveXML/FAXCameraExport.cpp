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

xmlNode* FArchiveXML::WriteCamera(FCDObject* object, xmlNode* parentNode)
{
	FCDCamera* camera = (FCDCamera*)object;

	// Create the base camera node
	xmlNode* cameraNode = FArchiveXML::WriteToEntityXMLFCDEntity(camera, parentNode, DAE_CAMERA_ELEMENT);
	xmlNode* opticsNode = AddChild(cameraNode, DAE_OPTICS_ELEMENT);
	xmlNode* baseNode = AddChild(opticsNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	const char* baseNodeName,* horizontalViewName,* verticalViewName;
	switch (camera->GetProjectionType())
	{
	case FCDCamera::PERSPECTIVE:
		baseNodeName = DAE_CAMERA_PERSP_ELEMENT;
		horizontalViewName = DAE_XFOV_CAMERA_PARAMETER;
		verticalViewName = DAE_YFOV_CAMERA_PARAMETER;
		break;

	case FCDCamera::ORTHOGRAPHIC:
		baseNodeName = DAE_CAMERA_ORTHO_ELEMENT;
		horizontalViewName = DAE_XMAG_CAMERA_PARAMETER;
		verticalViewName = DAE_YMAG_CAMERA_PARAMETER;
		break;

	default: baseNodeName = horizontalViewName = verticalViewName = DAEERR_UNKNOWN_ELEMENT; break;
	}
	baseNode = AddChild(baseNode, baseNodeName);

	// Write out the basic camera parameters
	if (camera->HasHorizontalFov())
	{
		xmlNode* viewNode = AddChild(baseNode, horizontalViewName, camera->GetFovX());
		FArchiveXML::WriteAnimatedValue(&camera->GetFovX(), viewNode, horizontalViewName);
	}
	if (!camera->HasHorizontalFov() || camera->HasVerticalFov())
	{
		xmlNode* viewNode = AddChild(baseNode, verticalViewName, camera->GetFovY());
		FArchiveXML::WriteAnimatedValue(&camera->GetFovY(), viewNode, verticalViewName);
	}

	// Aspect ratio: can only be exported if one of the vertical or horizontal view ratios is missing.
	if (camera->HasAspectRatio())
	{
		xmlNode* aspectNode = AddChild(baseNode, DAE_ASPECT_CAMERA_PARAMETER, camera->GetAspectRatio());
		FArchiveXML::WriteAnimatedValue(&camera->GetAspectRatio(), aspectNode, "aspect_ratio");
	}

	// Near/far clip plane distance
	xmlNode* clipNode = AddChild(baseNode, DAE_ZNEAR_CAMERA_PARAMETER, camera->GetNearZ());
	FArchiveXML::WriteAnimatedValue(&camera->GetNearZ(), clipNode, "near_clip");
	clipNode = AddChild(baseNode, DAE_ZFAR_CAMERA_PARAMETER, camera->GetFarZ());
	FArchiveXML::WriteAnimatedValue(&camera->GetFarZ(), clipNode, "far_clip");

	// Add the application-specific technique/parameters
	FCDENodeList extraParameterNodes;
	FUTrackedPtr<FCDETechnique> techniqueNode = NULL;

	// Export the <extra> elements and release the temporarily-added parameters/technique
	FArchiveXML::WriteTargetedEntityExtra(camera, cameraNode);
	CLEAR_POINTER_VECTOR(extraParameterNodes);
	if (techniqueNode != NULL && techniqueNode->GetChildNodeCount() == 0) SAFE_RELEASE(techniqueNode);
	return cameraNode;
}
