/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDForceTyped.h"
#include "FCDocument/FCDForceDeflector.h"
#include "FCDocument/FCDForceDrag.h"
#include "FCDocument/FCDForceGravity.h"
#include "FCDocument/FCDForcePBomb.h"
#include "FCDocument/FCDForceWind.h"
#include "FCDocument/FCDForceField.h"

				
bool FArchiveXML::LoadForceField(FCDObject* object, xmlNode* node)					
{
	FCDForceField* forceField = (FCDForceField*)object;

	bool status = FArchiveXML::LoadEntity(object, node);
	if (!status) return status;
	if (!IsEquivalent(node->name, DAE_FORCE_FIELD_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_FORCE_FIELD_ELEMENT, node->line);
		return status;
	}


	status &= (FArchiveXML::LoadExtra(forceField->GetInformation(), node));
	
	forceField->SetDirtyFlag();
	return status;
}
