/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMSkew.h"

//
// FMSkew
//

FMSkew::FMSkew()
{
}

FMSkew::FMSkew(const FMVector3& _rotateAxis, const FMVector3& _aroundAxis, float _angle)
:	rotateAxis(_rotateAxis), aroundAxis(_aroundAxis), angle(_angle)
{
}

bool operator==(const FMSkew& first, const FMSkew& other)
{
	return IsEquivalent(first.rotateAxis, other.rotateAxis) && IsEquivalent(first.aroundAxis, other.aroundAxis) && IsEquivalent(first.angle, other.angle);
}

