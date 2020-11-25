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
#include "FCDocument/FCDCamera.h"

//
// FCDCamera
//

ImplementObjectType(FCDCamera);

FCDCamera::FCDCamera(FCDocument* document)
:	FCDTargetedEntity(document, "Camera")
,	InitializeParameter(projection, PERSPECTIVE)
,	InitializeParameterAnimatable(viewY, 60.0f)
,	InitializeParameterAnimatable(viewX, 60.0f)
,	InitializeParameterAnimatable(aspectRatio, 1.0f)
,	InitializeParameterAnimatable(nearZ, 1.0f)
,	InitializeParameterAnimatable(farZ, 1000.0f)
{
	ResetHasHorizontalViewFlag();
	ResetHasVerticalViewFlag();
}

FCDCamera::~FCDCamera()
{
}

void FCDCamera::SetFovX(float _viewX)
{
	viewX = _viewX;
	if (GetHasVerticalViewFlag() && !IsEquivalent(viewX, 0.0f)) aspectRatio = viewX / viewY;
	SetHasHorizontalViewFlag();
	SetDirtyFlag(); 
}

void FCDCamera::SetFovY(float _viewY)
{
	viewY = _viewY;
	if (GetHasHorizontalViewFlag() && !IsEquivalent(viewX, 0.0f)) aspectRatio = viewX / viewY;
	SetHasVerticalViewFlag();
	SetDirtyFlag(); 
}

void FCDCamera::SetAspectRatio(float _aspectRatio)
{
	aspectRatio = _aspectRatio;
	SetDirtyFlag(); 
}
