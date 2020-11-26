/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDForceDeflector.h"
#include "FCDocument/FCDForceDrag.h"
#include "FCDocument/FCDForceGravity.h"
#include "FCDocument/FCDForcePBomb.h"
#include "FCDocument/FCDForceWind.h"

//
// FCDForceField
//

ImplementObjectType(FCDForceField);
ImplementParameterObject(FCDForceField, FCDExtra, information, new FCDExtra(parent->GetDocument(), parent))

FCDForceField::FCDForceField(FCDocument* document)
:	FCDEntity(document, "ForceField")
,	InitializeParameterNoArg(information)
{
	information = new FCDExtra(GetDocument(), this);
}

FCDForceField::~FCDForceField()
{
}

const FCDExtra* FCDForceField::GetInformation() const
{
	if (information == NULL)
	{
		const_cast<FCDForceField*>(this)->information = new FCDExtra(const_cast<FCDocument*>(GetDocument()), const_cast<FCDForceField*>(this));
	}
	return information;
}

FCDEntity* FCDForceField::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDForceField* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDForceField(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDForceField::GetClassType())) clone = (FCDForceField*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{

		// Clone the extra information.
		information->Clone(clone->information);
	}
	return _clone;
}
