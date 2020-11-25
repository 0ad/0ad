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
#include "FCDocument.h"
#include "FCDLight.h"

//
// FCDLight
//

ImplementObjectType(FCDLight);

FCDLight::FCDLight(FCDocument* document)
:	FCDTargetedEntity(document, "Light")
,	InitializeParameterAnimatable(color, FMVector3::One)
,	InitializeParameterAnimatable(intensity, 1.0f)
,	InitializeParameter(lightType, FCDLight::POINT)
,	InitializeParameterAnimatable(constantAttenuationFactor, 1.0f)
,	InitializeParameterAnimatable(linearAttenuationFactor, 0.0f)
,	InitializeParameterAnimatable(quadracticAttenuationFactor, 0.0f)
,	InitializeParameterAnimatable(fallOffExponent, 1.0f)
,	InitializeParameterAnimatable(fallOffAngle, 5.0f)
,	InitializeParameterAnimatable(outerAngle, 5.0f)
,	InitializeParameterAnimatable(penumbraAngle, 0.0f) // not used by default
,	InitializeParameterAnimatable(dropoff, 0.0f) // not used by default
{
}

FCDLight::~FCDLight()
{
}
