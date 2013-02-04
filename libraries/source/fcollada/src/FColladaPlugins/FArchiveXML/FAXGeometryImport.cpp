/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryNURBSSurface.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometrySpline.h"

bool FArchiveXML::LoadGeometrySource(FCDObject* object, xmlNode* sourceNode)
{
	FCDGeometrySource* geometrySource = (FCDGeometrySource*) object;
	FCDGeometrySourceData data;
	data.sourceNode = sourceNode;
	FArchiveXML::documentLinkDataMap[geometrySource->GetDocument()].geometrySourceDataMap.insert(geometrySource, data);

	bool status = true;

	// Read in the name and id of the source
	geometrySource->SetName(TO_FSTRING(ReadNodeName(sourceNode)));
	fm::string id = ReadNodeId(sourceNode);
	if (id.empty())
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_GEOMETRY_SOURCE_ID, sourceNode->line);
	}
	geometrySource->SetDaeId(id);
	if (!id.empty() && geometrySource->GetDaeId() != id)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_DUPLICATE_ID, sourceNode->line);
	}

	// Read in the source data
	geometrySource->SetStride(ReadSource(sourceNode, geometrySource->GetSourceData().GetDataList()));
	if (geometrySource->GetStride() == 0)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_SOURCE, sourceNode->line);
	}

	// If the <source> element has non-common techniques: we need to parse the extra information from them.
	FCDExtra* extra = geometrySource->GetExtra();
	FArchiveXML::LoadExtra(extra, sourceNode);
	if (extra->GetDefaultType()->GetTechniqueCount() == 0) SAFE_RELEASE(extra)

	return status;
}

bool FArchiveXML::LoadGeometryMesh(FCDObject* object, xmlNode* meshNode)
{
	FCDGeometryMesh* geometryMesh = (FCDGeometryMesh*)object;

	bool status = true;

	if (geometryMesh->IsConvex()) // <convex_mesh> element
		geometryMesh->SetConvexHullOf(ReadNodeProperty(meshNode, DAE_CONVEX_HULL_OF_ATTRIBUTE));

	if (geometryMesh->IsConvex() && !geometryMesh->GetConvexHullOf().empty())
	{
		return status;
	}

	// Read in the data sources
	xmlNodeList sourceDataNodes;
	FindChildrenByType(meshNode, DAE_SOURCE_ELEMENT, sourceDataNodes);
	for (xmlNodeList::iterator it = sourceDataNodes.begin(); it != sourceDataNodes.end(); ++it)
	{
		if (FCollada::CancelLoading()) return false;

		FCDGeometrySource* source = geometryMesh->AddSource();
		status &= (FArchiveXML::LoadGeometrySource(source, *it));
	}

	// Retrieve the <vertices> node
	xmlNode* verticesNode = FindChildByType(meshNode, DAE_VERTICES_ELEMENT);
	if (verticesNode == NULL)
	{
		status &= !FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_MESH_VERTICES_MISSING, meshNode->line);
	}

	// Read in the per-vertex inputs
	bool hasPositions = false;

	xmlNodeList vertexInputNodes;
	FindChildrenByType(verticesNode, DAE_INPUT_ELEMENT, vertexInputNodes);
	for (xmlNodeList::iterator it = vertexInputNodes.begin(); it < vertexInputNodes.end(); ++it)
	{
		xmlNode* vertexInputNode = *it;
		fm::string inputSemantic = ReadNodeSemantic(vertexInputNode);
		FUDaeGeometryInput::Semantic semantic = FUDaeGeometryInput::FromString(inputSemantic);
		if (semantic != FUDaeGeometryInput::VERTEX)
		{
			fm::string sourceId = ReadNodeSource(vertexInputNode);
			FCDGeometrySource* source = geometryMesh->FindSourceById(sourceId);
			if (source == NULL)
			{
				return FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_UNKNOWN_MESH_ID, vertexInputNode->line);
			}
			FArchiveXML::SetTypeFCDGeometrySource(source, semantic);
			if (semantic == FUDaeGeometryInput::POSITION) hasPositions = true;
			geometryMesh->AddVertexSource(source);
		}
	}
	if (!hasPositions)
	{
		status &= !FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_VP_INPUT_NODE_MISSING, verticesNode->line);
	}
	if (geometryMesh->GetVertexSourceCount() == 0)
	{
		status &= !FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_GEOMETRY_VERTICES_MISSING, verticesNode->line);
	}

	// Create our rendering object and read in the tessellation
	xmlNodeList polygonsNodes;
	for (xmlNode* childNode = meshNode->children; childNode != NULL; childNode = childNode->next)
	{
		if (FCollada::CancelLoading()) return false;

		FCDGeometryPolygons::PrimitiveType primType = (FCDGeometryPolygons::PrimitiveType) -1;

		if (childNode->type != XML_ELEMENT_NODE) continue;
		else if (IsEquivalent(childNode->name, DAE_SOURCE_ELEMENT)) continue;
		else if (IsEquivalent(childNode->name, DAE_VERTICES_ELEMENT)) continue;
		else if (IsEquivalent(childNode->name, DAE_POLYGONS_ELEMENT)
			|| IsEquivalent(childNode->name, DAE_TRIANGLES_ELEMENT)
			|| IsEquivalent(childNode->name, DAE_POLYLIST_ELEMENT)) primType = FCDGeometryPolygons::POLYGONS;
		else if (IsEquivalent(childNode->name, DAE_LINES_ELEMENT)) primType = FCDGeometryPolygons::LINES;
		else if (IsEquivalent(childNode->name, DAE_LINESTRIPS_ELEMENT)) primType = FCDGeometryPolygons::LINE_STRIPS;
		else if (IsEquivalent(childNode->name, DAE_TRIFANS_ELEMENT)) primType = FCDGeometryPolygons::TRIANGLE_FANS;
		else if (IsEquivalent(childNode->name, DAE_TRISTRIPS_ELEMENT)) primType = FCDGeometryPolygons::TRIANGLE_STRIPS;
		else if (IsEquivalent(childNode->name, DAE_POINTS_ELEMENT)) primType = FCDGeometryPolygons::POINTS;
		else continue;

		FUAssert(primType != (FCDGeometryPolygons::PrimitiveType) -1, continue);
		FCDGeometryPolygons* polygon = geometryMesh->AddPolygons();
		polygon->SetPrimitiveType(primType);
		status &= FArchiveXML::LoadGeometryPolygons(polygon, childNode);
	}

	size_t polygonsCount = geometryMesh->GetPolygonsCount();
	if (polygonsCount == 0)
	{
		status &= !FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_MESH_TESSELLATION_MISSING, meshNode->line);
	}

	// Process any per-vertex input sets
	xmlNode* verticesExtraNode = FindChildByType(verticesNode, DAE_EXTRA_ELEMENT);
	verticesExtraNode = FindTechnique(verticesExtraNode, DAE_FCOLLADA_PROFILE);
	if (verticesExtraNode != NULL)
	{
		xmlNodeList extraInputNodes;
		FindChildrenByType(verticesExtraNode, DAE_INPUT_ELEMENT, extraInputNodes);
		size_t extraInputNodeCount = extraInputNodes.size();
		for (size_t i = 0; i < extraInputNodeCount; ++i)
		{
			// Retrieve the source id and the set attributes.
			fm::string daeId = ReadNodeSource(extraInputNodes[i]);
			if (daeId.empty()) continue;
			if (daeId[0] == '#') daeId.erase(0, 1);
			fm::string _set = ReadNodeProperty(extraInputNodes[i], DAE_SET_ATTRIBUTE);
			if (_set.empty()) continue;
			int32 set = FUStringConversion::ToInt32(_set);

			// Find the matching per-vertex source and their polygon sets inputs.
			FCDGeometrySource* source = geometryMesh->FindSourceById(daeId);
			if (source == NULL || !geometryMesh->IsVertexSource(source)) continue;
			for (size_t j = 0; j < polygonsCount; ++j)
			{
				FCDGeometryPolygons* polys = geometryMesh->GetPolygons(j);
				FCDGeometryPolygonsInput* input = polys->FindInput(source);
				if (input == NULL) continue;
				input->SetSet(set);
			}
		}
	}

	// Calculate the important statistics/offsets/counts
	geometryMesh->Recalculate();
	return status;
}


bool FArchiveXML::LoadGeometry(FCDObject* object, xmlNode* geometryNode)
{
	FCDGeometry* geometry = (FCDGeometry*)object;

	geometry->SetMesh(NULL);
	geometry->SetSpline(NULL);

	bool status = FArchiveXML::LoadEntity(object, geometryNode);
	if (!status) return status;
	if (!IsEquivalent(geometryNode->name, DAE_GEOMETRY_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_GL_ELEMENT, geometryNode->line);
		return status;
	}

	// Read in the first valid child element found
	for (xmlNode* child = geometryNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_MESH_ELEMENT))
		{
			// Create a new mesh
			FCDGeometryMesh* m = geometry->CreateMesh();
			m->SetConvex(false);
			status &= (FArchiveXML::LoadGeometryMesh(m, child));
			break;
		}
		else if (IsEquivalent(child->name, DAE_CONVEX_MESH_ELEMENT))
		{
			// Create a new convex mesh
			FCDGeometryMesh* m = geometry->CreateMesh();
			m->SetConvex(true);
			status &= (FArchiveXML::LoadGeometryMesh(m, child));
			break;
		}
		else if (IsEquivalent(child->name, DAE_SPLINE_ELEMENT))
		{
			// Create a new spline
			FCDGeometrySpline* s = geometry->CreateSpline();
			status &= (FArchiveXML::LoadGeometrySpline(s, child));
			break;
		}
	}

	if (geometry->GetMesh() == NULL && geometry->GetSpline() == NULL && !geometry->IsPSurface())
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_GEOMETRY, geometryNode->line);
	}

	return status;
}

bool FArchiveXML::LoadGeometryPolygons(FCDObject* object, xmlNode* baseNode)
{
	FCDGeometryPolygons* geometryPolygons = (FCDGeometryPolygons*)object;

	bool status = true;

	// Retrieve the expected face count from the base node's 'count' attribute
	size_t expectedFaceCount = ReadNodeCount(baseNode);

	// Check the node's name to know whether to expect a <vcount> element
	size_t expectedVertexCount; bool isPolygons = false, isTriangles = false, isPolylist = false;
	if (geometryPolygons->GetPrimitiveType() == FCDGeometryPolygons::POLYGONS)
	{
		if (IsEquivalent(baseNode->name, DAE_POLYGONS_ELEMENT)) { expectedVertexCount = 4; isPolygons = true; }
		else if (IsEquivalent(baseNode->name, DAE_TRIANGLES_ELEMENT)) { expectedVertexCount = 3 * expectedFaceCount; isTriangles = true; }
		else { FUAssert(IsEquivalent(baseNode->name, DAE_POLYLIST_ELEMENT), return false); expectedVertexCount = 0; isPolylist = true; }
	}
	else
	{
		expectedVertexCount = 0;
		isPolygons = true; // read in the <p> elements. there really shouldn't be any <h> or <ph> elements anyway.
	}

	// Retrieve the material symbol used by these polygons
	geometryPolygons->SetMaterialSemantic(TO_FSTRING(ReadNodeProperty(baseNode, DAE_MATERIAL_ATTRIBUTE)));
	if (geometryPolygons->GetMaterialSemantic().empty())
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_POLYGON_MAT_SYMBOL, baseNode->line);
	}

	// Read in the per-face, per-vertex inputs
	xmlNode* itNode = NULL;
	bool hasVertexInput = false;
	FCDGeometryPolygonsInputList idxOwners;
	for (itNode = baseNode->children; itNode != NULL; itNode = itNode->next)
	{
		if (FCollada::CancelLoading()) return false;

		if (itNode->type != XML_ELEMENT_NODE) continue;
		if (IsEquivalent(itNode->name, DAE_INPUT_ELEMENT))
		{
			fm::string sourceId = ReadNodeSource(itNode);
			if (sourceId[0] == '#') sourceId.erase(0, 1);

			// Parse input idx/offset
			fm::string idx = ReadNodeProperty(itNode, DAE_OFFSET_ATTRIBUTE);
			uint32 offset = (!idx.empty()) ? FUStringConversion::ToUInt32(idx) : (uint32) (idxOwners.size() + 1);
			if (offset >= idxOwners.size()) idxOwners.resize(offset + 1);

			// Parse input set
			fm::string setString = ReadNodeProperty(itNode, DAE_SET_ATTRIBUTE);
			uint32 set = setString.empty() ? -1 : FUStringConversion::ToInt32(setString);

			// Parse input semantic
			FUDaeGeometryInput::Semantic semantic = FUDaeGeometryInput::FromString(ReadNodeSemantic(itNode));
			if (semantic == FUDaeGeometryInput::UNKNOWN) continue; // Unknown input type
			else if (semantic == FUDaeGeometryInput::VERTEX)
			{
				// There should never be more than one 'VERTEX' input.
				if (hasVertexInput)
				{
					FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EXTRA_VERTEX_INPUT, itNode->line);
					continue;
				}
				hasVertexInput = true;

				// Add an input for all the vertex sources in the parent.
				size_t vertexSourceCount = geometryPolygons->GetParent()->GetVertexSourceCount();
				for (uint32 i = 0; i < vertexSourceCount; ++i)
				{
					FCDGeometrySource* vertexSource = geometryPolygons->GetParent()->GetVertexSource(i);
					FCDGeometryPolygonsInput* vertexInput = geometryPolygons->FindInput(vertexSource);
					if (vertexInput == NULL) vertexInput = geometryPolygons->AddInput(vertexSource, offset);
					if (idxOwners[offset] == NULL) idxOwners[offset] = vertexInput;
					vertexInput->SetSet(set);
				}
			}
			else
			{
				// Retrieve the source for this input
				FCDGeometrySource* source = geometryPolygons->GetParent()->FindSourceById(sourceId);
				if (source != NULL)
				{
					FArchiveXML::SetTypeFCDGeometrySource(source, semantic); 
					FCDGeometryPolygonsInput* input = geometryPolygons->AddInput(source, offset);
					if (idxOwners[offset] == NULL) idxOwners[offset] = input;
					input->SetSet(set);
				}
				else
				{
					FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_POLYGONS_INPUT, itNode->line);
				}
			}
		}
		else if (IsEquivalent(itNode->name, DAE_EXTRA_ELEMENT))
		{
			status &= (FArchiveXML::LoadExtra(geometryPolygons->GetExtra(), itNode));
		}
		else if (IsEquivalent(itNode->name, DAE_POLYGON_ELEMENT)
			|| IsEquivalent(itNode->name, DAE_VERTEXCOUNT_ELEMENT)
			|| IsEquivalent(itNode->name, DAE_POLYGONHOLED_ELEMENT))
		{
			break;
		}
		else
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_POLYGON_CHILD, itNode->line);
		}
	}

	// Verify the information retrieved so far.
	bool noTessellation = false;
	if (expectedFaceCount == 0)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_POLYGONS, baseNode->line);
		noTessellation = true;
	}
	if (itNode == NULL)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::WARNING_NO_POLYGON, baseNode->line);
		return status;
	}
	if (!hasVertexInput)
	{
		// Verify that we did find a VERTEX polygon set input.
		//return status.Fail(FS("Cannot find 'VERTEX' polygons' input within geometry: ") + TO_FSTRING(parent->GetDaeId()), baseNode->line);
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_NO_VERTEX_INPUT, baseNode->line);
		return status;
	}

	if (!noTessellation)
	{
		// Look for the <vcount> element and parse it in
		xmlNode* vCountNode = FindChildByType(baseNode, DAE_VERTEXCOUNT_ELEMENT);
		bool hasVertexCounts = vCountNode != NULL;
		if (isPolylist && !hasVertexCounts)
		{
			FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_NO_VCOUNT, baseNode->line);
			return status;
		}
		else if (!isPolylist && hasVertexCounts)
		{
 			FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISPLACED_VCOUNT, baseNode->line);
			return status;
		}
		else if (isPolylist)
		{
			// Process the vertex counts.
			const char* vCountDataString = ReadNodeContentDirect(vCountNode);
			UInt32List vCountData;
			if (vCountDataString != NULL) FUStringConversion::ToUInt32List(vCountDataString, vCountData);
			size_t vCountCount = vCountData.size();
			geometryPolygons->SetFaceVertexCountCount(vCountCount);
			memcpy((void*) geometryPolygons->GetFaceVertexCounts(), vCountData.begin(), sizeof(uint32) * vCountCount);

			// Count the total number of face-vertices expected, to pre-buffer the index lists
			// The absolute maximum possible is the number of vertices (That is, a face 
			// that includes every vertex)
			// We can assume all these ptrs are valid, otherwise we wouldnt get here.
			size_t nVertices = geometryPolygons->GetParent()->GetPositionSource()->GetValueCount();
			expectedVertexCount = 0;
			for (size_t i = 0; i < vCountCount; ++i) 
			{
				if (vCountData[i] > nVertices)
				{
					FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_INVALID_FACE_COUNT, vCountNode->line);
					return false;
				}
				expectedVertexCount += vCountData[i];
			}
		}
	}

	// Pre-allocate the buffers with enough memory
	fm::pvector<UInt32List> allIndices;
	UInt32List* masterIndices = NULL;
	size_t indexStride = idxOwners.size();
	allIndices.resize(indexStride);
	for (size_t i = 0; i < indexStride; ++i)
	{
		FCDGeometryPolygonsInput* input = idxOwners[i];
		if (input == NULL) allIndices[i] = NULL;
		else
		{
			allIndices[i] = new UInt32List();
			allIndices[i]->reserve(expectedVertexCount);
			if (masterIndices == NULL) masterIndices = allIndices[i];
			input->ReserveIndexCount(expectedVertexCount);
		}
	}

	// Process the tessellation
	for (; itNode != NULL; itNode = itNode->next)
	{
		if (FCollada::CancelLoading())
		{
			CLEAR_POINTER_VECTOR(allIndices);
			return false;
		}

		if (itNode->type != XML_ELEMENT_NODE) continue;
		if (IsEquivalent(itNode->name, DAE_POLYGON_ELEMENT) || IsEquivalent(itNode->name, DAE_POLYGONHOLED_ELEMENT))
		{
			// Retrieve the indices
			xmlNode* holeNode = NULL;
			const char* content = NULL;
			if (!IsEquivalent(itNode->name, DAE_POLYGONHOLED_ELEMENT)) 
			{
				content = ReadNodeContentDirect(itNode);
			} 
			else 
			{
				// Holed face found
				for (xmlNode* child = itNode->children; child != NULL; child = child->next)
				{
					if (child->type != XML_ELEMENT_NODE) continue;
					if (IsEquivalent(child->name, DAE_POLYGON_ELEMENT)) 
					{
						content = ReadNodeContentDirect(child);
					}
					else if (IsEquivalent(child->name, DAE_HOLE_ELEMENT)) 
					{ 
						holeNode = child; break; 
					}
					else 
					{
						FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_UNKNOWN_PH_ELEMENT, itNode->line);
						continue;
					}
				}
			}

			// Parse the indices
			FUStringConversion::ToInterleavedUInt32List(content, allIndices);
			uint32 localFaceVertexCount = (uint32) masterIndices->size();

			if (isTriangles) for (uint32 i = 0; i < localFaceVertexCount / 3; ++i) geometryPolygons->AddFaceVertexCount(3);
			else if (isPolygons) geometryPolygons->AddFaceVertexCount(localFaceVertexCount);

			// Push the indices to the index buffers
			for (size_t k = 0; k < indexStride; ++k)
			{
				FCDGeometryPolygonsInput* input = idxOwners[k];
				if (input != NULL) input->AddIndices(*allIndices[k]);
			}

			// Append any hole indices found
			for (; holeNode != NULL; holeNode = holeNode->next)
			{
				if (holeNode->type != XML_ELEMENT_NODE) continue;

				// Read in the hole indices and push them on top of the other indices
				content = ReadNodeContentDirect(holeNode);
				FUStringConversion::ToInterleavedUInt32List(content, allIndices);
				for (size_t k = 0; k < indexStride; ++k)
				{
					FCDGeometryPolygonsInput* input = idxOwners[k];
					if (input != NULL) input->AddIndices(*allIndices[k]);
				}

				// Create the hole face and record its index
				size_t holeVertexCount = masterIndices->size();
				geometryPolygons->AddHole((uint32) geometryPolygons->GetFaceVertexCountCount());
				geometryPolygons->AddFaceVertexCount((uint32) holeVertexCount);
			}
		}

		else if (IsEquivalent(itNode->name, DAE_EXTRA_ELEMENT))
		{
			status &= (FArchiveXML::LoadExtra(geometryPolygons->GetExtra(), itNode));
		}
		else if (IsEquivalent(itNode->name, DAE_VERTEXCOUNT_ELEMENT)) {} // Don't whine at this one.
		else
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_POLYGON_CHILD, itNode->line);
		}

		if (FCollada::CancelLoading())
		{
			CLEAR_POINTER_VECTOR(allIndices);
			return false;
		}
	}

	// Check the actual face count
	size_t primitiveCount;
	if (geometryPolygons->GetPrimitiveType() != FCDGeometryPolygons::LINES)
	{
		primitiveCount = geometryPolygons->GetFaceVertexCountCount() - geometryPolygons->GetHoleFaceCount();
	}
	else
	{
		// LINES are special: the 'count' attribute implies total inner lines primitives.
		primitiveCount = 0;
		size_t pCount = geometryPolygons->GetFaceVertexCountCount();
		for (size_t i = 0; i < pCount; ++i)
		{
			// There is one primitive for two vertices in the <p> elements.
			primitiveCount += geometryPolygons->GetFaceVertexCount(i) / 2;
 		}
	}
	if (expectedFaceCount != primitiveCount)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_INVALID_FACE_COUNT, baseNode->line);
	}

	CLEAR_POINTER_VECTOR(allIndices);
	geometryPolygons->SetDirtyFlag();
	return status;
}


bool FArchiveXML::LoadGeometrySpline(FCDObject* object, xmlNode* splineNode)
{
	FCDGeometrySpline* geometrySpline = (FCDGeometrySpline*)object;

	bool status = true;

	// for each spline
	for (; splineNode != NULL; splineNode = splineNode->next)
	{
		// is it a spline?
		if (!IsEquivalent(splineNode->name, DAE_SPLINE_ELEMENT)) continue;

		// needed extra node
		// TODO. those will be moved to attributes
		xmlNode* extraNode = FindChildByType(splineNode, DAE_EXTRA_ELEMENT);
		if (extraNode == NULL) continue;
		xmlNode* fcolladaNode = FindTechnique(extraNode, DAE_FCOLLADA_PROFILE);
		if (fcolladaNode == NULL) continue;
		xmlNode* typeNode = FindChildByType(fcolladaNode, DAE_TYPE_ATTRIBUTE);
		if (typeNode == NULL) continue;

		// get the spline type
		FUDaeSplineType::Type splineType = FUDaeSplineType::FromString(ReadNodeContentFull(typeNode));

		// The types must be compatible
		if (!geometrySpline->SetType(splineType))
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_VARYING_SPLINE_TYPE, splineNode->line);
			return status;
		}

		// Read in the typed spline.
		FCDSpline* spline = geometrySpline->AddSpline();
		bool s = FArchiveXML::LoadSwitch(spline, &spline->GetObjectType(), splineNode);
		if (!s)
		{
			SAFE_RELEASE(spline);
			status = false;
		}
	}
		
	geometrySpline->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadSpline(FCDObject* object, xmlNode* splineNode)
{
	FCDSpline* spline = (FCDSpline*)object;

	// Read the curve closed attribute
	spline->SetClosed(FUStringConversion::ToBoolean(ReadNodeProperty(splineNode, DAE_CLOSED_ATTRIBUTE)));

	// Read in the <control_vertices> element, which define the base type for this curve
	xmlNode* controlVerticesNode = FindChildByType(splineNode, DAE_CONTROL_VERTICES_ELEMENT);
	if (controlVerticesNode == NULL)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_CONTROL_VERTICES_MISSING, splineNode->line);
		return false;
	}

	// Read in the <control_vertices> inputs.
	xmlNodeList inputElements;
	FindChildrenByType(controlVerticesNode, DAE_INPUT_ELEMENT, inputElements);
	for (size_t i = 0; i < inputElements.size(); i++)
	{
		xmlNode* inputNode = inputElements[i];
		fm::string sourceId = ReadNodeProperty(inputNode, DAE_SOURCE_ATTRIBUTE);
		if (sourceId.empty()) { FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_DEFAULT_ERROR); return false; }
		xmlNode* sourceNode = FindChildById(splineNode, sourceId);
		if (sourceNode == NULL) { FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_DEFAULT_ERROR); return false; }

		if (IsEquivalent(ReadNodeProperty(inputNode, DAE_SEMANTIC_ATTRIBUTE), DAE_CVS_SPLINE_INPUT))
		{
			// Read in the spline control points.
			ReadSource(sourceNode, spline->GetCVs());
		}
	}

	return true;
}

bool FArchiveXML::LoadBezierSpline(FCDObject* object, xmlNode* splineNode)
{
	if (!FArchiveXML::LoadSpline(object, splineNode)) return false;

	FCDBezierSpline* bezierSpline = (FCDBezierSpline*)object;
	return bezierSpline->IsValid();
}

bool FArchiveXML::LoadLinearSpline(FCDObject* object, xmlNode* splineNode)
{
	if (!FArchiveXML::LoadSpline(object, splineNode)) return false;

	FCDLinearSpline* linearSpline = (FCDLinearSpline*)object;
	return linearSpline->IsValid();
}

bool FArchiveXML::LoadNURBSSpline(FCDObject* object, xmlNode* splineNode)
{
	if (!FArchiveXML::LoadSpline(object, splineNode)) return false;

	bool status = true;
	FCDNURBSSpline* nurbsSpline = (FCDNURBSSpline*)object;

	xmlNode* extraNode = FindChildByType(splineNode, DAE_EXTRA_ELEMENT);
	if (extraNode == NULL) { FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_DEFAULT_ERROR); return status; }
	xmlNode* fcolladaNode = FindTechnique(extraNode, DAE_FCOLLADA_PROFILE);
	if (fcolladaNode == NULL) { FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_DEFAULT_ERROR); return status; }

	// Read in the NURBS degree
	xmlNode* degreeNode = FindChildByType(fcolladaNode, DAE_DEGREE_ATTRIBUTE);
	nurbsSpline->SetDegree((degreeNode != NULL) ? FUStringConversion::ToUInt32(ReadNodeContentDirect(degreeNode)) : 3);

	// Read in the <control_vertices> element, which define the base type for this curve
	xmlNode* controlVerticesNode = FindChildByType(splineNode, DAE_CONTROL_VERTICES_ELEMENT);
	if (controlVerticesNode == NULL)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_CONTROL_VERTICES_MISSING, splineNode->line);
		return status;
	}

	// read the sources
	xmlNodeList inputElements;
	FindChildrenByType(controlVerticesNode, DAE_INPUT_ELEMENT, inputElements);

	for (size_t i = 0; i < inputElements.size(); i++)
	{
		xmlNode* inputNode = inputElements[i];
		fm::string sourceId = ReadNodeProperty(inputNode, DAE_SOURCE_ATTRIBUTE).substr(1);
		if (sourceId.empty()) { FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_DEFAULT_ERROR); return status; }
		xmlNode* sourceNode = FindChildById(splineNode, sourceId);
		if (sourceNode == NULL) { FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_DEFAULT_ERROR); return status; }

		else if (IsEquivalent(ReadNodeProperty(inputNode, DAE_SEMANTIC_ATTRIBUTE), DAE_KNOT_SPLINE_INPUT))
		{
			ReadSource(sourceNode, nurbsSpline->GetKnots());
		}
		else if (IsEquivalent(ReadNodeProperty(inputNode, DAE_SEMANTIC_ATTRIBUTE), DAE_WEIGHT_SPLINE_INPUT))
		{
			ReadSource(sourceNode, nurbsSpline->GetWeights());
		}
	}

	status &= nurbsSpline->IsValid();

	return status;
}

void FArchiveXML::SetTypeFCDGeometrySource(FCDGeometrySource* geometrySource, FUDaeGeometryInput::Semantic type)
{
	FCDGeometrySourceDataMap::iterator it = FArchiveXML::documentLinkDataMap[geometrySource->GetDocument()].geometrySourceDataMap.find(geometrySource);
	FUAssert(it != FArchiveXML::documentLinkDataMap[geometrySource->GetDocument()].geometrySourceDataMap.end(),);
	FCDGeometrySourceData& data = it->second;

	geometrySource->SetSourceType(type);
	geometrySource->GetAnimatedValues().clear();

	// Most types should remain un-animated
	if (geometrySource->GetType() != FUDaeGeometryInput::POSITION && geometrySource->GetType() != FUDaeGeometryInput::COLOR) return;

	FArchiveXML::LoadAnimatable(geometrySource->GetDocument(), &geometrySource->GetSourceData(), data.sourceNode);
	if (geometrySource->GetSourceData().IsAnimated() && geometrySource->GetType() == FUDaeGeometryInput::POSITION)
	{
		// Set the relative flags.
		FUObjectContainer<FCDAnimated>& animateds = geometrySource->GetSourceData().GetAnimatedValues();
		for (FCDAnimated** animated = animateds.begin(); animated != animateds.end(); ++animated)
		{
			(*animated)->SetRelativeAnimationFlag();
		}
	}
	geometrySource->SetDirtyFlag();
}
