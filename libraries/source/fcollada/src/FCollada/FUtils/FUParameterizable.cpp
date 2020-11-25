/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUParameter.h"
#include "FUParameterizable.h"

//
// FUParameterizable
//

ImplementObjectType(FUParameterizable)

FUParameterizable::FUParameterizable()
:	FUTrackable(), flags(0)
{
}

FUParameterizable::~FUParameterizable()
{
}

