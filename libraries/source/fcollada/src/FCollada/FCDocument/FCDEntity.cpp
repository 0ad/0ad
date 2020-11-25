/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDExtra.h"

//
// Constants
//

static const size_t MAX_NAME_LENGTH = 512;

//
// FCDEntity
//

ImplementObjectType(FCDEntity);
ImplementParameterObject(FCDEntity, FCDAsset, asset, new FCDAsset(parent->GetDocument()));
ImplementParameterObject(FCDEntity, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));

FCDEntity::FCDEntity(FCDocument* document, const char* baseId)
:	FCDObjectWithId(document, baseId)
,	InitializeParameterNoArg(name)
,	InitializeParameterNoArg(extra)
,	InitializeParameterNoArg(asset)
,	InitializeParameterNoArg(note)
{
	extra = new FCDExtra(document, this);
}

FCDEntity::~FCDEntity()
{
}

// Structure cloning
FCDEntity* FCDEntity::Clone(FCDEntity* clone, bool UNUSED(cloneChildren)) const
{
	if (clone == NULL)
	{
		clone = new FCDEntity(const_cast<FCDocument*>(GetDocument()));
	}

	FCDObjectWithId::Clone(clone);
	clone->name = name;
	clone->note = note;
	if (extra != NULL)
	{
		extra->Clone(clone->extra);
	}
	return clone;
}

fstring FCDEntity::CleanName(const fchar* c)
{
	size_t len = 0;
	for (; len < MAX_NAME_LENGTH; len++) { if (c[len] == 0) break; }
	fstring cleanName(len, *c);
	fchar* id = cleanName.begin();
	if (*c != 0)
	{
	
		// First character: alphabetic or '_'.
		if ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || *c == '_') *id = *c;
		else *id = '_';

		// Other characters: alphabetic, numeric, '_'.
		// NOTE: ':' and '.' are NOT acceptable characters.
		for (size_t i = 1; i < len; ++i)
		{
			++id; ++c;
			if ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '_' || *c == '-') *id = *c;
			else *id = '_';
		}
		*(++id) = 0;
	}
	return cleanName;
}

void FCDEntity::SetName(const fstring& _name) 
{
	name = CleanName(_name.c_str());
	SetDirtyFlag();
}

FCDAsset* FCDEntity::GetAsset()
{
	return (asset != NULL) ? asset : (asset = new FCDAsset(GetDocument()));
}

void FCDEntity::GetHierarchicalAssets(FCDAssetConstList& assets) const
{
	if (asset != NULL) assets.push_back(asset);
	else assets.push_back(GetDocument()->GetAsset());
}

// Look for a children with the given COLLADA Id.
const FCDEntity* FCDEntity::FindDaeId(const fm::string& _daeId) const
{
	if (GetDaeId() == _daeId) return this;
	return NULL;
}

bool FCDEntity::HasNote() const
{
	return !note->empty();
}

const fstring& FCDEntity::GetNote() const
{
	return *note;
}

void FCDEntity::SetNote(const fstring& _note)
{
	note = _note;
	SetDirtyFlag();
}
