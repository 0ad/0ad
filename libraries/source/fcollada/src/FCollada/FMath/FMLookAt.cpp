/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMLookAt.h"

//
// FMLookAt
//

FMLookAt::FMLookAt()
{
}

FMLookAt::FMLookAt(const FMVector3& _position, const FMVector3& _target, const FMVector3& _up)
:	position(_position), target(_target), up(_up)
{
}

bool operator==(const FMLookAt& first, const FMLookAt& other)
{
	return IsEquivalent(first.up, other.up) && IsEquivalent(first.position, other.position) && IsEquivalent(first.target, other.target);
}

