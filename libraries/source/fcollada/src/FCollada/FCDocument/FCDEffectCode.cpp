/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDObjectWithId.h"
#include "FCDocument/FCDEffectCode.h"
#include "FUtils/FUFileManager.h"

ImplementObjectType(FCDEffectCode);

FCDEffectCode::FCDEffectCode(FCDocument* document) : FCDObject(document)
{
	type = INCLUDE;
}

FCDEffectCode::~FCDEffectCode()
{
}

// Do not inline this function.  No memory-creating functions should be inline
void FCDEffectCode::SetSubId(const fm::string& _sid)
{ 
	sid = FCDObjectWithId::CleanSubId(_sid); 
	SetDirtyFlag(); 
}

void FCDEffectCode::SetFilename(const fstring& _filename)
{
	filename = GetDocument()->GetFileManager()->GetCurrentUri().MakeAbsolute(_filename);
	type = INCLUDE;
	SetDirtyFlag();
}

// Clone
FCDEffectCode* FCDEffectCode::Clone(FCDEffectCode* clone) const
{
	if (clone == NULL) clone = new FCDEffectCode(const_cast<FCDocument*>(GetDocument()));
	clone->type = type;
	clone->sid = sid;
	clone->filename = filename;
	clone->code = code;
	return clone;
}
