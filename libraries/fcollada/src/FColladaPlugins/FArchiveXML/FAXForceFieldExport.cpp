/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDForceTyped.h"
#include "FCDocument/FCDForceDeflector.h"
#include "FCDocument/FCDForceDrag.h"
#include "FCDocument/FCDForceGravity.h"
#include "FCDocument/FCDForcePBomb.h"
#include "FCDocument/FCDForceWind.h"
#include "FCDocument/FCDForceField.h"


xmlNode* FArchiveXML::WriteForceField(FCDObject* object, xmlNode* parentNode)
{
	FCDForceField* forceField = (FCDForceField*)object;

	xmlNode* forceFieldNode = FArchiveXML::WriteToEntityXMLFCDEntity(forceField, parentNode, DAE_FORCE_FIELD_ELEMENT);


	if (forceField->GetInformation() != NULL)
	{
		FArchiveXML::WriteTechniquesFCDExtra(forceField->GetInformation(), forceFieldNode);
	}

	FArchiveXML::WriteEntityExtra(forceField, forceFieldNode);
	return forceFieldNode;
}
