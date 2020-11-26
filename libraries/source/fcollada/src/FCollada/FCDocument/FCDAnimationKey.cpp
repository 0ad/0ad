/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDAnimationKey.h"

//
// FCDAnimationMKey
//

FCDAnimationMKey::FCDAnimationMKey(uint32 _dimension)
:	dimension(_dimension)
{
	output = new float[dimension];
}

FCDAnimationMKey::~FCDAnimationMKey()
{
	SAFE_DELETE_ARRAY(output);
}

//
// FCDAnimationMKeyBezier
//

FCDAnimationMKeyBezier::FCDAnimationMKeyBezier(uint32 dimension)
:	FCDAnimationMKey(dimension)
{
	inTangent = new FMVector2[dimension];
	outTangent = new FMVector2[dimension];
}

FCDAnimationMKeyBezier::~FCDAnimationMKeyBezier()
{
	SAFE_DELETE_ARRAY(inTangent);
	SAFE_DELETE_ARRAY(outTangent);
}

//
// FCDAnimationMKeyTCB
//

FCDAnimationMKeyTCB::FCDAnimationMKeyTCB(uint32 dimension)
:	FCDAnimationMKey(dimension)
{
	tension = new float[dimension];
	continuity = new float[dimension];
	bias = new float[dimension];
	easeIn = new float[dimension];
	easeOut = new float[dimension];
}

FCDAnimationMKeyTCB::~FCDAnimationMKeyTCB()
{
	SAFE_DELETE_ARRAY(tension);
	SAFE_DELETE_ARRAY(continuity);
	SAFE_DELETE_ARRAY(bias);
	SAFE_DELETE_ARRAY(easeIn);
	SAFE_DELETE_ARRAY(easeOut);
}
