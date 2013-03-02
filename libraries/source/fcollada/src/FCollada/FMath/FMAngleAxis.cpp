/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMAngleAxis.h"

//
// FMAngleAxis
//

FMAngleAxis::FMAngleAxis()
{
}

FMAngleAxis::FMAngleAxis(const FMVector3& _axis, float _angle)
:	axis(_axis), angle(_angle)
{
}

bool operator==(const FMAngleAxis& first, const FMAngleAxis& other)
{
	if (IsEquivalent(first.angle, other.angle))
	{
		return IsEquivalent(first.axis, other.axis);
	}
	else
	{
		return IsEquivalent(first.angle, -other.angle) && IsEquivalent(first.axis, -other.axis);
	}
}

