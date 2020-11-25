/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometrySource.h"

//
// FCDGeometryPolygonsInput
//

ImplementObjectType(FCDGeometryPolygonsInput);
ImplementParameterObject(FCDGeometryPolygonsInput, FCDGeometrySource, source, new FCDGeometrySource(parent->GetDocument()));

FCDGeometryPolygonsInput::FCDGeometryPolygonsInput(FCDocument* document, FCDGeometryPolygons* _parent)
:	FCDObject(document), parent(_parent)
,	InitializeParameterNoArg(source)
,	InitializeParameter(set, -1)
,	InitializeParameter(offset, 0)
,	InitializeParameterNoArg(indices)
{
}

FCDGeometryPolygonsInput::~FCDGeometryPolygonsInput()
{
	if (source != NULL)
	{
		UntrackObject(source);
		source = NULL;
	}
}

FUDaeGeometryInput::Semantic FCDGeometryPolygonsInput::GetSemantic() const
{
	FUAssert(source != NULL, return FUDaeGeometryInput::UNKNOWN);
	return source->GetType();
}

// Sets the referenced source
void FCDGeometryPolygonsInput::SetSource(FCDGeometrySource* _source)
{
	// Untrack the old source and track the new source
	if (source != NULL) UntrackObject(source);
	source = _source;
	if (source != NULL) TrackObject(source);
}

// Callback when the tracked source is released.
void FCDGeometryPolygonsInput::OnObjectReleased(FUTrackable* object)
{
	if (source == object)
	{
		source = NULL;
		
		// Verify whether we own/share the index list.
		if (!indices.empty())
		{
			size_t inputCount = parent->GetInputCount();
			for (size_t i = 0; i < inputCount; ++i)
			{
				FCDGeometryPolygonsInput* other = parent->GetInput(i);
				if (other->offset == offset)
				{
					// Move the shared list of indices to the other input.
					other->indices = indices;
					indices.clear();
					break;
				}
			}
		}
	}
}

void FCDGeometryPolygonsInput::SetIndices(const uint32* _indices, size_t count)
{
	FUParameterUInt32List& indices = FindIndices();
	if (count > 0)
	{
		indices.resize(count);
		memcpy(&indices.front(), _indices, count * sizeof(uint32));
	}
	else indices.clear();
}

void FCDGeometryPolygonsInput::SetIndexCount(size_t count)
{
	FUParameterUInt32List& indices = FindIndices();
	indices.resize(count);
}

const uint32* FCDGeometryPolygonsInput::GetIndices() const
{
	return FindIndices().begin();
}

size_t FCDGeometryPolygonsInput::GetIndexCount() const
{
	return FindIndices().size();
}

void FCDGeometryPolygonsInput::ReserveIndexCount(size_t count)
{
	FUParameterUInt32List& indices = FindIndices();
	if (count > indices.size()) indices.reserve(count);
}

void FCDGeometryPolygonsInput::AddIndex(uint32 index)
{
	FindIndices().push_back(index);
}

void FCDGeometryPolygonsInput::AddIndices(const UInt32List& _indices)
{
	FUParameterUInt32List& indices = FindIndices();
	indices.insert(indices.size(), _indices.begin(), _indices.size());
}

const FUParameterUInt32List& FCDGeometryPolygonsInput::FindIndices() const
{
	if (OwnsIndices()) return indices; // Early exit for local owner.

	size_t inputCount = parent->GetInputCount();
	for (size_t i = 0; i < inputCount; ++i)
	{
		FCDGeometryPolygonsInput* input = parent->GetInput(i);
		if (input->offset == offset && input->OwnsIndices()) return input->indices;
	}

	// No indices allocated yet.
	return indices;
}
