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
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometrySource.h"

//
// FCDGeometryPolygons
//

ImplementObjectType(FCDGeometryPolygons);
ImplementParameterObject(FCDGeometryPolygons, FCDGeometryPolygonsInput, inputs, new FCDGeometryPolygonsInput(parent->GetDocument(), parent));
ImplementParameterObject(FCDGeometryPolygons, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));

FCDGeometryPolygons::FCDGeometryPolygons(FCDocument* document, FCDGeometryMesh* _parent)
:	FCDObject(document)
,	parent(_parent)
,	InitializeParameterNoArg(inputs)
,	InitializeParameterNoArg(faceVertexCounts)
,	InitializeParameterNoArg(holeFaces)
,	InitializeParameter(primitiveType, POLYGONS)
,	faceVertexCount(0), faceOffset(0), faceVertexOffset(0), holeOffset(0)
,	InitializeParameterNoArg(materialSemantic)
,	InitializeParameterNoArg(extra)
{
	// Pre-buffer the face-vertex counts so that AddFaceVertexCount won't be extremely costly.
	faceVertexCounts.reserve(32);
}

FCDGeometryPolygons::~FCDGeometryPolygons()
{
	holeFaces.clear();
	parent = NULL;
}

FCDExtra* FCDGeometryPolygons::GetExtra()
{
	return (extra != NULL) ? extra : (extra = new FCDExtra(GetDocument(), this));
}

// Creates a new face.
void FCDGeometryPolygons::AddFace(uint32 degree)
{
	bool newPolygonSet = faceVertexCounts.empty();
	faceVertexCounts.push_back(degree);

	// Inserts empty indices
	size_t inputCount = inputs.size();
	for (size_t i = 0; i < inputCount; ++i)
	{
		FCDGeometryPolygonsInput* input = inputs[i];
		if (!newPolygonSet && input->OwnsIndices()) input->SetIndexCount(input->GetIndexCount() + degree);
		else if (newPolygonSet && input->GetIndexCount() == 0)
		{
			// Declare this input as the owner!
			input->SetIndexCount(degree);
		}
	}

	parent->Recalculate();
	SetDirtyFlag();
}

// Removes a face
void FCDGeometryPolygons::RemoveFace(size_t index)
{
	FUAssert(index < GetFaceCount(), return);

	// Remove the associated indices, if they exist.
	size_t offset = GetFaceVertexOffset(index);
	size_t indexCount = GetFaceVertexCount(index);
	size_t inputCount = inputs.size();
	for (size_t i = 0; i < inputCount; ++i)
	{
		FCDGeometryPolygonsInput* input = inputs[i];
		if (!input->OwnsIndices()) continue;

		size_t inputIndexCount = input->GetIndexCount();
		if (offset < inputIndexCount)
		{
			// Move the indices backwards.
			uint32* indices = input->GetIndices();
			for (size_t o = offset; o < inputIndexCount - indexCount; ++o)
			{
				indices[o] = indices[o + indexCount];
			}
			input->SetIndexCount(max(offset, inputIndexCount - indexCount));
		}
	}

	// Remove the face and its holes
	size_t holeBefore = GetHoleCountBefore(index);
	size_t holeCount = GetHoleCount(index);
	faceVertexCounts.erase(index + holeBefore, holeCount + 1); // +1 in order to remove the polygon as well as the holes.

	parent->Recalculate();
	SetDirtyFlag();
}

// Calculates the offset of face-vertex pairs before the given face index within the polygon set.
size_t FCDGeometryPolygons::GetFaceVertexOffset(size_t index) const
{
	size_t offset = 0;

	// We'll need to skip over the holes
	size_t holeCount = GetHoleCountBefore(index);
	if (index + holeCount < faceVertexCounts.size())
	{
		// Sum up the wanted offset
		UInt32List::const_iterator end = faceVertexCounts.begin() + index + holeCount;
		for (UInt32List::const_iterator it = faceVertexCounts.begin(); it != end; ++it)
		{
			offset += (*it);
		}
	}
	return offset;
}

// Calculates the number of holes within the polygon set that appear before the given face index.
size_t FCDGeometryPolygons::GetHoleCountBefore(size_t index) const
{
	size_t holeCount = 0;
	for (UInt32List::const_iterator it = holeFaces.begin(); it != holeFaces.end(); ++it)
	{
		if ((*it) <= index) { ++holeCount; ++index; }
	}
	return holeCount;
}

// Retrieves the number of holes within a given face.
size_t FCDGeometryPolygons::GetHoleCount(size_t index) const
{
	size_t holeCount = 0;
	for (size_t i = index + GetHoleCountBefore(index) + 1; i < faceVertexCounts.size(); ++i)
	{
		bool isHoled = holeFaces.find((uint32) i) != holeFaces.end();
		if (!isHoled) break;
		else ++holeCount;
	}
	return holeCount;
}

// The number of face-vertex pairs for a given face.
size_t FCDGeometryPolygons::GetFaceVertexCount(size_t index) const
{
	size_t count = 0;
	if (index < GetFaceCount())
	{
		size_t holeCount = GetHoleCount(index);
		UInt32List::const_iterator it = faceVertexCounts.begin() + index + GetHoleCountBefore(index);
		UInt32List::const_iterator end = it + holeCount + 1; // +1 in order to sum the face-vertex pairs of the polygon as its holes.
		for (; it != end; ++it) count += (*it);
	}
	return count;
}

FCDGeometryPolygonsInput* FCDGeometryPolygons::AddInput(FCDGeometrySource* source, uint32 offset)
{
	FCDGeometryPolygonsInput* input = new FCDGeometryPolygonsInput(GetDocument(), this);
	inputs.push_back(input);
	input->SetOffset(offset);
	input->SetSource(source);
	SetNewChildFlag();
	return input;
}

void FCDGeometryPolygons::SetHoleFaceCount(size_t count)
{
	holeFaces.resize(count);
	SetDirtyFlag();
}

bool FCDGeometryPolygons::IsHoleFaceHole(size_t index)
{
	return holeFaces.find((uint32) index) != holeFaces.end();
}

void FCDGeometryPolygons::AddHole(uint32 index)
{
	FUAssert(!IsHoleFaceHole(index), return);

	// Ordered insert
	const uint32* it = holeFaces.begin();
	for (; it != holeFaces.end(); ++it)
	{
		if (index < (*it)) break;
	}
	holeFaces.insert(it - holeFaces.begin(), index);
}

void FCDGeometryPolygons::AddFaceVertexCount(uint32 count)
{
	faceVertexCounts.push_back(count);
}

void FCDGeometryPolygons::SetFaceVertexCountCount(size_t count)
{
	faceVertexCounts.resize(count);
}

const FCDGeometryPolygonsInput* FCDGeometryPolygons::FindInput(FUDaeGeometryInput::Semantic semantic) const
{
	for (const FCDGeometryPolygonsInput** it = inputs.begin(); it != inputs.end(); ++it)
	{
		if ((*it)->GetSemantic() == semantic) return (*it);
	}
	return NULL;
}

const FCDGeometryPolygonsInput* FCDGeometryPolygons::FindInput(const FCDGeometrySource* source) const
{
	for (const FCDGeometryPolygonsInput** it = inputs.begin(); it != inputs.end(); ++it)
	{
		if ((*it)->GetSource() == source) return (*it);
	}
	return NULL;
}

FCDGeometryPolygonsInput* FCDGeometryPolygons::FindInput(const fm::string& sourceId)
{
	const char* s = sourceId.c_str();
	if (*s == '#') ++s;
	size_t inputCount = inputs.size();
	for (size_t i = 0; i < inputCount; ++i)
	{
		FCDGeometryPolygonsInput* input = inputs[i];
		if (input->GetSource()->GetDaeId() == s) return input;
	}
	return NULL;
}

void FCDGeometryPolygons::FindInputs(FUDaeGeometryInput::Semantic semantic, FCDGeometryPolygonsInputConstList& _inputs) const
{
	for (const FCDGeometryPolygonsInput** it = inputs.begin(); it != inputs.end(); ++it)
	{
		if ((*it)->GetSemantic() == semantic) _inputs.push_back(*it);
	}
}

// Recalculates the face-vertex count within the polygons
void FCDGeometryPolygons::Recalculate()
{
	faceVertexCount = 0;
	for (const uint32* itC = faceVertexCounts.begin(); itC != faceVertexCounts.end(); ++itC)
	{
		faceVertexCount += (*itC);
	}
	SetDirtyFlag();
}

// [DEPRECATED]
bool FCDGeometryPolygons::IsTriangles() const
{
	UInt32List::const_iterator itC;
	for (itC = faceVertexCounts.begin(); itC != faceVertexCounts.end() && (*itC) == 3; ++itC) {}
	return (itC == faceVertexCounts.end());
}

int32 FCDGeometryPolygons::TestPolyType() const
{
	UInt32List::const_iterator itC = faceVertexCounts.begin();
	if (!faceVertexCounts.empty())
	{
		uint32 fCount = *itC;
		for (itC ; itC != faceVertexCounts.end() && *itC == fCount; ++itC) {}
		if (itC == faceVertexCounts.end()) return fCount;
	}
	return -1;
}

// Clone this list of polygons
FCDGeometryPolygons* FCDGeometryPolygons::Clone(FCDGeometryPolygons* clone, const FCDGeometrySourceCloneMap& cloneMap) const
{
	if (clone == NULL) return NULL;

	// Clone the miscellaneous information.
	clone->materialSemantic = materialSemantic;
	clone->faceVertexCounts = faceVertexCounts;
	clone->faceOffset = faceOffset;
	clone->faceVertexCount = faceVertexCount;
	clone->faceVertexOffset = faceVertexOffset;
	clone->holeOffset = holeOffset;
	clone->holeFaces = holeFaces;
	
	// Clone the geometry inputs
	// Note that the vertex source inputs are usually created by default.
	size_t inputCount = inputs.size();
	clone->inputs.reserve(inputCount);
	for (size_t i = 0; i < inputCount; ++i)
	{
		// Find the cloned source that correspond to the original input.
		FCDGeometrySource* cloneSource = NULL;
		FCDGeometrySourceCloneMap::const_iterator it = cloneMap.find(inputs[i]->GetSource());
		if (it == cloneMap.end())
		{
			// Attempt to match by ID instead.
			const fm::string& id = inputs[i]->GetSource()->GetDaeId();
			cloneSource = clone->GetParent()->FindSourceById(id);
		}
		else
		{
			cloneSource = (*it).second;
		}

		// Retrieve or create the input to clone.
		FCDGeometryPolygonsInput* input = clone->FindInput(cloneSource);
		if (input == NULL)
		{
			input = clone->AddInput(cloneSource, inputs[i]->GetOffset());
		}

		// Clone the input information.
		if (inputs[i]->OwnsIndices())
		{
			input->SetIndices(inputs[i]->GetIndices(), inputs[i]->GetIndexCount());
		}
		input->SetSet(inputs[i]->GetSet());
	}

	return clone;
}
