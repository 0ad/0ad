/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument.h"
#include "FCDObjectWithId.h"
#include "FUtils/FUUniqueStringMap.h"

static const size_t MAX_ID_LENGTH = 512;

//
// FCDObjectWithId
//

ImplementObjectType(FCDObjectWithId);

FCDObjectWithId::FCDObjectWithId(FCDocument* document, const char* baseId)
:	FCDObject(document)
,	InitializeParameter(daeId, baseId)
{
	ResetUniqueIdFlag();
}

FCDObjectWithId::~FCDObjectWithId()
{
	RemoveDaeId();
}

void FCDObjectWithId::Clone(FCDObjectWithId* clone) const
{
	clone->daeId = daeId;
	const_cast<FCDObjectWithId*>(this)->RemoveDaeId();
}

const fm::string& FCDObjectWithId::GetDaeId() const
{
	if (!GetUniqueIdFlag())
	{
		// Generate a new id
		FCDObjectWithId* e = const_cast<FCDObjectWithId*>(this);
		FUSUniqueStringMap* names = e->GetDocument()->GetUniqueNameMap();
		FUAssert(!e->daeId->empty(), e->daeId = "unknown_object");
		names->insert(e->daeId);
		e->SetUniqueIdFlag();
	}
	return daeId;
}

void FCDObjectWithId::SetDaeId(const fm::string& id)
{
	RemoveDaeId();

	// Use this id to enforce a unique id.
	FUSUniqueStringMap* names = GetDocument()->GetUniqueNameMap();
	daeId = CleanId(id);
	names->insert(daeId);
	SetUniqueIdFlag();
	SetDirtyFlag();
}

void FCDObjectWithId::SetDaeId(fm::string& id)
{
	SetDaeId(*(const fm::string*)&id);
	id = daeId; // We return back the new value.
}

void FCDObjectWithId::RemoveDaeId()
{
	if (GetUniqueIdFlag())
	{
		FUSUniqueStringMap* names = GetDocument()->GetUniqueNameMap();
		names->erase(daeId);
		ResetUniqueIdFlag();
		SetDirtyFlag();
	}
}

// Clean-up the id and names to match the schema definitions of 'IDref' and 'Name'.
fm::string FCDObjectWithId::CleanId(const char* c)
{
	size_t len = 0;
	for (; len < MAX_ID_LENGTH; len++) { if (c[len] == 0) break; }
	fm::string out(len, *c);
	char* id = out.begin();
	if (*c != 0)
	{
		// First character: alphabetic or '_'.
		if ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || *c == '_') *id = *c;
		else *id = '_';

		// Other characters: alphabetic, numeric, '_', '-' or '.'.
		// Otherwise, use HTML extended character write-up: &#<num>;
		// NOTE: ':' is not an acceptable characters.
		//for (++c, ++id; *c != 0; ++c, ++id)
		for (size_t i = 1; i < len; i++)
		{
			++c; ++id;
			if ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '_' || *c == '-' || *c == '.') *id = *c;
			else *id = '_';
		}
		// Dont forget to close.
		*(++id) = 0;
	}
	return out;
}

fm::string FCDObjectWithId::CleanSubId(const char* c)
{
	size_t len = 0;
	for (; len < MAX_ID_LENGTH; len++) { if (c[len] == 0) break; }
	fm::string out(len, *c);
	char* sid = out.begin();
	if (*c != 0)
	{
		// First character: alphabetic or '_'.
		if ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || *c == '_') *sid = *c;
		else *sid = '_';

		// Other characters: alphabetic, numeric, '_', '-' or '.'.
		// Otherwise, use HTML extended character write-up: &#<num>;
		// NOTE: ':' is not an acceptable characters.
		//for (++c, ++id; *c != 0; ++c, ++id)
		for (size_t i = 1; i < len; i++)
		{
			++c; ++sid;
			if ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '_' || *c == '-') *sid = *c;
			else *sid = '_';
		}
		// Dont forget to close.
		*(++sid) = 0;
	}
	return out;
}

