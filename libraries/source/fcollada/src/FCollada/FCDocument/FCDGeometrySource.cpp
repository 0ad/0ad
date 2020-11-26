/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FUtils/FUDaeEnum.h"

//
// FCDGeometrySource
//

ImplementObjectType(FCDGeometrySource);
ImplementParameterObject(FCDGeometrySource, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));

FCDGeometrySource::FCDGeometrySource(FCDocument* document)
:	FCDObjectWithId(document, "GeometrySource")
,	InitializeParameterNoArg(name)
,	InitializeParameterAnimatableNoArg(sourceData)
,	InitializeParameter(stride, 0)
,	InitializeParameter(sourceType, (uint32) FUDaeGeometryInput::UNKNOWN)
,	InitializeParameterNoArg(extra)
{
}

FCDGeometrySource::~FCDGeometrySource()
{
}

void FCDGeometrySource::SetDataCount(size_t count)
{
	sourceData.resize(count);
	SetDirtyFlag();
}

FCDGeometrySource* FCDGeometrySource::Clone(FCDGeometrySource* clone) const
{
	if (clone == NULL) clone = new FCDGeometrySource(const_cast<FCDocument*>(GetDocument()));
	FCDObjectWithId::Clone(clone);
	clone->name = name;
	clone->sourceType = sourceType;

	// Clone the data of this source.
	clone->stride = stride;
	clone->sourceData.GetDataList() = sourceData.GetDataList(); // this should be replaced by an operator= where even the FCDAnimated* list is copied.
	clone->sourceType = sourceType;

	// Clone the extra information.
	if (extra != NULL)
	{
		extra->Clone(clone->GetExtra());
	}

	return clone;	
}

void FCDGeometrySource::SetData(const FloatList& _sourceData, uint32 _sourceStride, size_t offset, size_t count)
{
	// Remove all the data currently held by the source.
	sourceData.clear();
	stride = _sourceStride;

	// Check the given bounds
	size_t beg = min(offset, _sourceData.size()), end;
	if (count == 0) end = _sourceData.size();
	else end = min(count + offset, _sourceData.size());
	sourceData.insert(0, _sourceData.begin() + beg, end - beg);

	SetDirtyFlag();
}

FCDExtra* FCDGeometrySource::GetExtra()
{
	return (extra != NULL) ? extra : extra = new FCDExtra(GetDocument(), this);
}
