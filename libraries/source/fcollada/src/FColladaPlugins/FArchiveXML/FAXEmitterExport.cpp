/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDEmitter.h"
#include "FCDocument/FCDEmitterObject.h"
#include "FCDocument/FCDEmitterParticle.h"
#include "FCDocument/FCDParticleModifier.h"


//
// Emitter export
//

xmlNode* FArchiveXML::WriteEmitter(FCDObject* object, xmlNode* parentNode)
{
	FCDEmitter* emitter = (FCDEmitter*) object;
	xmlNode* emitterNode = FArchiveXML::WriteToEntityXMLFCDEntity(emitter, parentNode, DAE_EMITTER_ELEMENT);

	FArchiveXML::WriteEntityExtra(emitter, emitterNode);
	return emitterNode;
}

