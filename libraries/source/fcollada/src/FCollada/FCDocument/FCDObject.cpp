/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDObject.h"

// 
// FCDObject
//

ImplementObjectType(FCDObject);

FCDObject::FCDObject(FCDocument* _document)
:	FUParameterizable(), document(_document)
,	userHandle(NULL)
{
	SetDirtyFlag();
}

FCDObject::~FCDObject()
{
}


