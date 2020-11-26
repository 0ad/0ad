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
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDLibrary.h"
#include "FUtils/FUStringConversion.h"

// 
// FCDGeometryMesh
//

ImplementObjectType(FCDGeometryMesh);
ImplementParameterObject(FCDGeometryMesh, FCDGeometrySource, sources, new FCDGeometrySource(parent->GetDocument()));
ImplementParameterObject(FCDGeometryMesh, FCDGeometryPolygons, polygons, new FCDGeometryPolygons(parent->GetDocument(), parent));
ImplementParameterObjectNoCtr(FCDGeometryMesh, FCDGeometrySource, vertexSources);

FCDGeometryMesh::FCDGeometryMesh(FCDocument* document, FCDGeometry* _parent)
:	FCDObject(document), parent(_parent)
,	InitializeParameterNoArg(sources)
,	InitializeParameterNoArg(polygons)
,	InitializeParameterNoArg(vertexSources)
,	faceCount(0), holeCount(0), faceVertexCount(0)
,	isConvex(true), convexify(false)
,	InitializeParameterNoArg(convexHullOf)
{
}

FCDGeometryMesh::~FCDGeometryMesh()
{
	polygons.clear();
	sources.clear();
	faceVertexCount = faceCount = holeCount = 0;
	parent = NULL;
}

// Retrieve the parent's id
const fm::string& FCDGeometryMesh::GetDaeId() const
{
	return parent->GetDaeId();
}

void FCDGeometryMesh::SetConvexHullOf(FCDGeometry* _geom)
{
	convexHullOf = _geom->GetDaeId();
	SetDirtyFlag();
}

const FCDGeometryMesh* FCDGeometryMesh::FindConvexHullOfMesh() const
{
	const FCDGeometryMesh* mesh = this;
	while ((mesh != NULL) && !mesh->GetConvexHullOf().empty())
	{
		const FCDocument* document = mesh->GetDocument();
		const FCDGeometry* geometry = document->GetGeometryLibrary()->
				FindDaeId(mesh->GetConvexHullOf());
		if (geometry == NULL) return NULL;
		mesh = geometry->GetMesh();
	}
	return mesh;
}

// Search for a data source in the geometry node
const FCDGeometrySource* FCDGeometryMesh::FindSourceById(const fm::string& id) const
{
	const char* localId = id.c_str();
	if (localId[0] == '#') ++localId;
	for (const FCDGeometrySource** it = sources.begin(); it != sources.end(); ++it)
	{
		if ((*it)->GetDaeId() == localId) return (*it);
	}
	return NULL;
}

// Retrieve the source for the given type
const FCDGeometrySource* FCDGeometryMesh::FindSourceByType(FUDaeGeometryInput::Semantic type) const 
{
	for (const FCDGeometrySource** itS = sources.begin(); itS != sources.end(); ++itS)
	{
		if ((*itS)->GetType() == type) return (*itS);
	}
	return NULL;
}

void FCDGeometryMesh::FindSourcesByType(FUDaeGeometryInput::Semantic type, FCDGeometrySourceConstList& _sources) const
{
	for (const FCDGeometrySource** itS = sources.begin(); itS != sources.end(); ++itS)
	{
		if ((*itS)->GetType() == type) _sources.push_back(*itS);
	}
}

const FCDGeometrySource* FCDGeometryMesh::FindSourceByName(const fstring& name) const 
{
	for (const FCDGeometrySource** itS = sources.begin(); itS != sources.end(); ++itS)
	{
		if ((*itS)->GetName() == name) return (*itS);
	}
	return NULL;
}

// Creates a new polygon group.
FCDGeometryPolygons* FCDGeometryMesh::AddPolygons()
{
	FCDGeometryPolygons* polys = new FCDGeometryPolygons(GetDocument(), this);
	polygons.push_back(polys);

	// Add to this new polygons all the per-vertex sources.
	size_t vertexSourceCount = vertexSources.size();
	for (size_t v = 0; v < vertexSourceCount; ++v)
	{
		polys->AddInput(vertexSources[v], 0);
	}

	SetNewChildFlag();
	if (parent != NULL) parent->SetNewChildFlag();
	return polys;
}

bool FCDGeometryMesh::IsTriangles() const
{
	bool isTriangles = true;
	for (size_t i = 0; i < polygons.size() && isTriangles; ++i)
	{
		isTriangles = (polygons[i]->TestPolyType() == 3);
	}
	return isTriangles;
}

// Retrieves the polygon sets that use a given material semantic
void FCDGeometryMesh::FindPolygonsByMaterial(const fstring& semantic, FCDGeometryPolygonsList& sets)
{
	size_t polygonCount = polygons.size();
	for (size_t p = 0; p < polygonCount; ++p)
	{
		if (polygons[p]->GetMaterialSemantic() == semantic) sets.push_back(polygons[p]);
	}
}

// Creates a new per-vertex data source
FCDGeometrySource* FCDGeometryMesh::AddVertexSource(FUDaeGeometryInput::Semantic type)
{
	FCDGeometrySource* vertexSource = AddSource(type);
	vertexSources.push_back(vertexSource);

	// Add this new per-vertex data source to all the existing polygon groups, at offset 0.
	size_t polygonsCount = polygons.size();
	for (size_t p = 0; p < polygonsCount; ++p)
	{
		polygons[p]->AddInput(vertexSource, 0);
	}

	SetNewChildFlag();
	return vertexSource;
}

// Sets a source as per-vertex data.
void FCDGeometryMesh::AddVertexSource(FCDGeometrySource* source)
{
	FUAssert(source != NULL, return);
	FUAssert(!vertexSources.contains(source), return);

	// Add the source to the list of per-vertex sources.
	vertexSources.push_back(source);

	// Remove any polygon set input that uses the source.
	size_t polygonsCount = polygons.size();
	for (size_t p = 0; p < polygonsCount; ++p)
	{
		FCDGeometryPolygonsInput* input = polygons[p]->FindInput(source);
		int32 set = (input != NULL) ? input->GetSet() : -1;
		SAFE_RELEASE(input);
		input = polygons[p]->AddInput(source, 0);
		if (set > -1) input->SetSet(set);
	}

	SetNewChildFlag();
}

void FCDGeometryMesh::RemoveVertexSource(FCDGeometrySource* source)
{
	FUAssert(source != NULL, return);
	if (!vertexSources.contains(source)) return;

	// Add the source to the list of per-vertex sources.
	vertexSources.erase(source);
	SetDirtyFlag();
}

// Creates a new data source
FCDGeometrySource* FCDGeometryMesh::AddSource(FUDaeGeometryInput::Semantic type)
{
	FCDGeometrySource* source = new FCDGeometrySource(GetDocument());
	source->SetType(type);
	sources.push_back(source);
	SetNewChildFlag();
	return source;
}

// Recalculates all the hole/vertex/face-vertex counts and offsets within the mesh and its polygons
void FCDGeometryMesh::Recalculate()
{
	faceCount = holeCount = faceVertexCount = 0;
	size_t polygonsCount = polygons.size();
	for (size_t p = 0; p < polygonsCount; ++p)
	{
		FCDGeometryPolygons* polys = polygons[p];
		polys->Recalculate();

		polys->SetFaceOffset(faceCount);
		polys->SetHoleOffset(holeCount);
		polys->SetFaceVertexOffset(faceVertexCount);
		faceCount += polys->GetFaceCount();
		holeCount += polys->GetHoleCount();
		faceVertexCount += polys->GetFaceVertexCount();
	}
	SetDirtyFlag();
}

FCDGeometryMesh* FCDGeometryMesh::Clone(FCDGeometryMesh* clone) const
{
	if (clone == NULL) clone = new FCDGeometryMesh(const_cast<FCDocument*>(GetDocument()), NULL);

	// Copy the miscellaneous information
	clone->convexHullOf = convexHullOf;
	clone->isConvex = isConvex;
	clone->convexify = convexify;
	clone->faceCount = faceCount;
	clone->holeCount = holeCount;
	clone->faceVertexCount = faceVertexCount;

	// Clone the sources
	FCDGeometrySourceCloneMap cloneMap;
	for (const FCDGeometrySource** itS = sources.begin(); itS != sources.end(); ++itS)
	{
		FCDGeometrySource* clonedSource = (IsVertexSource(*itS)) ? clone->AddVertexSource() : clone->AddSource();
		(*itS)->Clone(clonedSource);
		cloneMap.insert(*itS, clonedSource);
	}

	// Clone the polygon sets.
	for (const FCDGeometryPolygons** itP = polygons.begin(); itP != polygons.end(); ++itP)
	{
		FCDGeometryPolygons* clonedPolys = clone->AddPolygons();
		(*itP)->Clone(clonedPolys, cloneMap);
	}

	return clone;
}
