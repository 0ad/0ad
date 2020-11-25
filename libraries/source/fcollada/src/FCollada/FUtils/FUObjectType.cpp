/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUObjectType.h"

FUObjectType::FUObjectType(const char* _typeName)
: parent(NULL)
{
	typeName = _typeName;
}

FUObjectType::FUObjectType(const FUObjectType& _parent, const char* _typeName)
: parent(&_parent)
{
	typeName = _typeName;
}

bool FUObjectType::Includes(const FUObjectType& otherType) const
{
	if (otherType == *this) return true;
	else if (parent != NULL) return parent->Includes(otherType);
	else return false;
}
