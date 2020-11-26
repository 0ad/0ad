/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEmitter.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDEmitterParticle.h"

//
// FCDEmitter
//

ImplementObjectType(FCDEmitter);

FCDEmitter::FCDEmitter(FCDocument* document)
:	FCDEntity(document, "Emitter")
{
}

FCDEmitter::~FCDEmitter()
{
}

FCDEntity* FCDEmitter::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDEmitter* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDEmitter(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDEmitter::GetClassType())) clone = (FCDEmitter*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
	}
	return _clone;
}

