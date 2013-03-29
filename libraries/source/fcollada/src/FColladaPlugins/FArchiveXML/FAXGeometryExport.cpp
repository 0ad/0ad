/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryNURBSSurface.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FUtils/FUDaeEnum.h"
#include "FUtils/FUDaeEnumSyntax.h"

xmlNode* FArchiveXML::WriteGeometrySource(FCDObject* object, xmlNode* parentNode)
{
	FCDGeometrySource* geometrySource = (FCDGeometrySource*)object;

	xmlNode* sourceNode = NULL;

	// Export the source directly, using the correct parameters and the length factor
	FloatList& sourceData = geometrySource->GetSourceData().GetDataList();
	uint32 stride = geometrySource->GetStride();
	switch (geometrySource->GetType())
	{
	case FUDaeGeometryInput::POSITION: sourceNode = AddSourceFloat(parentNode, geometrySource->GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::NORMAL: sourceNode = AddSourceFloat(parentNode, geometrySource->GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::GEOTANGENT: sourceNode = AddSourceFloat(parentNode, geometrySource->GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::GEOBINORMAL: sourceNode = AddSourceFloat(parentNode, geometrySource->GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::TEXCOORD: sourceNode = AddSourceFloat(parentNode, geometrySource->GetDaeId(), sourceData, stride, FUDaeAccessor::STPQ); break;
	case FUDaeGeometryInput::TEXTANGENT: sourceNode = AddSourceFloat(parentNode, geometrySource->GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::TEXBINORMAL: sourceNode = AddSourceFloat(parentNode, geometrySource->GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::UV: sourceNode = AddSourceFloat(parentNode, geometrySource->GetDaeId(), sourceData, stride, FUDaeAccessor::XYZW); break;
	case FUDaeGeometryInput::COLOR: sourceNode = AddSourceFloat(parentNode, geometrySource->GetDaeId(), sourceData, stride, FUDaeAccessor::RGBA); break;
	case FUDaeGeometryInput::EXTRA: sourceNode = AddSourceFloat(parentNode, geometrySource->GetDaeId(), sourceData, stride, NULL); break;
	case FUDaeGeometryInput::UNKNOWN: sourceNode = AddSourceFloat(parentNode, geometrySource->GetDaeId(), sourceData, stride, NULL); break;

	case FUDaeGeometryInput::VERTEX: // Refuse to export these sources
	default: break;
	}

	if (!geometrySource->GetName().empty())
	{
		AddAttribute(sourceNode, DAE_NAME_ATTRIBUTE, geometrySource->GetName());
	}

	if (geometrySource->GetExtra() != NULL)
	{
		FArchiveXML::WriteTechniquesFCDExtra(geometrySource->GetExtra(), sourceNode);
	}

	for (size_t i = 0; i < geometrySource->GetAnimatedValues().size(); ++i)
	{
		FArchiveXML::WriteAnimatedValue(geometrySource->GetAnimatedValues()[i], sourceNode, "");
	}

	return sourceNode;
}

xmlNode* FArchiveXML::WriteGeometryMesh(FCDObject* object, xmlNode* parentNode)
{
	FCDGeometryMesh* geometryMesh = (FCDGeometryMesh*)object;

	xmlNode* meshNode = NULL;

	if (geometryMesh->IsConvex() && !geometryMesh->GetConvexHullOf().empty())
	{
		meshNode = AddChild(parentNode, DAE_CONVEX_MESH_ELEMENT);
		FUSStringBuilder convexHullOfName(geometryMesh->GetConvexHullOf());
		AddAttribute(meshNode, DAE_CONVEX_HULL_OF_ATTRIBUTE, convexHullOfName);
	}
	else
	{
		meshNode = AddChild(parentNode, DAE_MESH_ELEMENT);

		// Write out the sources
		for (size_t i = 0; i < geometryMesh->GetSourceCount(); ++i)
		{
			FArchiveXML::LetWriteObject(geometryMesh->GetSource(i), meshNode);
		}

		// Write out the <vertices> element
		xmlNode* verticesNode = AddChild(meshNode, DAE_VERTICES_ELEMENT);
		xmlNode* verticesInputExtraNode = NULL,* verticesInputExtraTechniqueNode = NULL;
		for (size_t i = 0; i < geometryMesh->GetVertexSourceCount(); ++i)
		{
			FCDGeometrySource* source = geometryMesh->GetVertexSource(i);
			const char* semantic = FUDaeGeometryInput::ToString(source->GetType());
			AddInput(verticesNode, source->GetDaeId(), semantic);
			if (geometryMesh->GetPolygonsCount() > 0)
			{
				FCDGeometryPolygons* firstPolys = geometryMesh->GetPolygons(0);
				FCDGeometryPolygonsInput* input = firstPolys->FindInput(source);
				FUAssert(input != NULL, continue);
				if (input->GetSet() != -1)
				{
					// We are interested in the set information, so if it is available, export it as an extra.
					if (verticesInputExtraNode == NULL)
					{
						verticesInputExtraNode = FUXmlWriter::CreateNode(DAE_EXTRA_ELEMENT);
						verticesInputExtraTechniqueNode = FUXmlWriter::AddChild(verticesInputExtraNode, DAE_TECHNIQUE_ELEMENT);
						FUXmlWriter::AddAttribute(verticesInputExtraTechniqueNode, DAE_PROFILE_ATTRIBUTE, DAE_FCOLLADA_PROFILE);
					}
					AddInput(verticesInputExtraTechniqueNode, source->GetDaeId(), semantic, -1, input->GetSet());
				}
			}
		}
		if (verticesInputExtraNode != NULL) AddChild(verticesNode, verticesInputExtraNode);

		FUSStringBuilder verticesNodeId(geometryMesh->GetDaeId()); verticesNodeId.append("-vertices");
		AddAttribute(verticesNode, DAE_ID_ATTRIBUTE, verticesNodeId);

		// Write out the polygons
		for (size_t i = 0; i < geometryMesh->GetPolygonsCount(); ++i)
		{
			FArchiveXML::LetWriteObject(geometryMesh->GetPolygons(i), meshNode);
		}
	}
	return meshNode;
}


xmlNode* FArchiveXML::WriteGeometry(FCDObject* object, xmlNode* parentNode)
{
	FCDGeometry* geometry = (FCDGeometry*)object;

	xmlNode* geometryNode = FArchiveXML::WriteToEntityXMLFCDEntity(geometry, parentNode, DAE_GEOMETRY_ELEMENT);

	if (geometry->GetMesh() != NULL) FArchiveXML::LetWriteObject(geometry->GetMesh(), geometryNode);
	else if (geometry->GetSpline() != NULL) FArchiveXML::LetWriteObject(geometry->GetSpline(), geometryNode);

	FArchiveXML::WriteEntityExtra(geometry, geometryNode);
	return geometryNode;
}

xmlNode* FArchiveXML::WriteGeometryPolygons(FCDObject* object, xmlNode* parentNode)
{
	FCDGeometryPolygons* geometryPolygons = (FCDGeometryPolygons*)object;

	// Are there holes? Then, export a <polygons> element.
	// Are there only non-triangles within the list? Then, export a <polylist> element.
	// Otherwise, you only have triangles: export a <triangles> element.
	// That's all nice for polygon lists, otherwise we export the correct primitive type.
	bool hasHoles = false, hasNPolys = true;

	// Create the base node for these polygons
	const char* polygonNodeType;
	switch (geometryPolygons->GetPrimitiveType())
	{
	case FCDGeometryPolygons::POLYGONS:
		// Check for polygon with holes and triangle-only conditions.
		hasHoles = geometryPolygons->GetHoleFaceCount() > 0;
		if (!hasHoles) hasNPolys = (geometryPolygons->TestPolyType() != 3);

		if (hasHoles) polygonNodeType = DAE_POLYGONS_ELEMENT;
		else if (hasNPolys) polygonNodeType = DAE_POLYLIST_ELEMENT;
		else polygonNodeType = DAE_TRIANGLES_ELEMENT;
		break;

	case FCDGeometryPolygons::LINES: hasHoles = true; polygonNodeType = DAE_LINES_ELEMENT; break;
	case FCDGeometryPolygons::LINE_STRIPS: hasHoles = true; polygonNodeType = DAE_LINESTRIPS_ELEMENT; break;
	case FCDGeometryPolygons::TRIANGLE_FANS: hasHoles = true; polygonNodeType = DAE_TRIFANS_ELEMENT; break;
	case FCDGeometryPolygons::TRIANGLE_STRIPS: hasHoles = true; polygonNodeType = DAE_TRISTRIPS_ELEMENT; break;
	case FCDGeometryPolygons::POINTS: polygonNodeType = DAE_POINTS_ELEMENT; break;
	default: polygonNodeType = emptyCharString; FUFail(break); break;
	}
	xmlNode* polygonsNode = AddChild(parentNode, polygonNodeType);

	// Add the inputs
	// Find which input owner belongs to the <vertices> element. Replace the semantic and the source id accordingly.
	// Make sure to add that 'vertex' input only once.
	FUSStringBuilder verticesNodeId(geometryPolygons->GetParent()->GetDaeId()); verticesNodeId.append("-vertices");
	bool isVertexInputFound = false;
	fm::pvector<const FCDGeometryPolygonsInput> idxOwners; // Record a list of input data owners.
	for (size_t i = 0; i < geometryPolygons->GetInputCount(); ++i)
	{
		const FCDGeometryPolygonsInput* input = geometryPolygons->GetInput(i);
		const FCDGeometrySource* source = input->GetSource();
		if (source != NULL)
		{
			if (!geometryPolygons->GetParent()->IsVertexSource(source))
			{
				const char* semantic = FUDaeGeometryInput::ToString(input->GetSemantic());
				FUDaeWriter::AddInput(polygonsNode, source->GetDaeId(), semantic, input->GetOffset(), input->GetSet());
			}
			else if (!isVertexInputFound)
			{
				FUDaeWriter::AddInput(polygonsNode, verticesNodeId.ToCharPtr(), DAE_VERTEX_INPUT, input->GetOffset());
				isVertexInputFound = true;
			}
		}

		if (input->OwnsIndices())
		{
			if (input->GetOffset() >= idxOwners.size()) idxOwners.resize(input->GetOffset() + 1);
			idxOwners[input->GetOffset()] = input;
		}
	}

	FUSStringBuilder builder;
	builder.reserve(1024);

	// For the poly-list case, export the list of vertex counts
	if (!hasHoles && hasNPolys)
	{
		FUStringConversion::ToString(builder, geometryPolygons->GetFaceVertexCounts(), geometryPolygons->GetFaceVertexCountCount());
		xmlNode* vcountNode = AddChild(polygonsNode, DAE_VERTEXCOUNT_ELEMENT);
		AddContentUnprocessed(vcountNode, builder.ToCharPtr());
		builder.clear();
	}

	// For the non-holes cases, open only one <p> element for all the data indices
	xmlNode* pNode = NULL,* phNode = NULL;
	if (!hasHoles) pNode = AddChild(polygonsNode, DAE_POLYGON_ELEMENT);

	// Export the data indices (tessellation information)
	size_t faceCount = geometryPolygons->GetFaceCount();
	uint32 faceVertexOffset = 0;
	size_t holeOffset = 0;
	for (size_t faceIndex = 0; faceIndex < faceCount; ++faceIndex)
	{
		// For the holes cases, verify whether this face or the next one(s) are holes. We may need to open a new <ph>/<p> element
		size_t holeCount = 0;
		if (hasHoles)
		{
			holeCount = geometryPolygons->GetHoleCount(faceIndex);

			if (holeCount == 0)
			{
				// Just open a <p> element: this is the most common case
				pNode = AddChild(polygonsNode, DAE_POLYGON_ELEMENT);
			}
			else
			{
				// Open up a new <ph> element and its <p> element
				phNode = AddChild(polygonsNode, DAE_POLYGONHOLED_ELEMENT);
				pNode = AddChild(phNode, DAE_POLYGON_ELEMENT);
			}
		}

		for (size_t holeIndex = 0; holeIndex < holeCount + 1; ++holeIndex)
		{
			// Write out the tessellation information for all the vertices of this face
			uint32 faceVertexCount = geometryPolygons->GetFaceVertexCounts()[faceIndex + holeOffset + holeIndex];
			for (uint32 faceVertexIndex = faceVertexOffset; faceVertexIndex < faceVertexOffset + faceVertexCount; ++faceVertexIndex)
			{
				for (fm::pvector<const FCDGeometryPolygonsInput>::iterator itI = idxOwners.begin(); itI != idxOwners.end(); ++itI)
				{
					if ((*itI) != NULL)
					{
						builder.append((*itI)->GetIndices()[faceVertexIndex]);
						builder.append(' ');
					}
					else builder.append("0 ");
				}
			}

			// For the holes cases: write out the indices for every polygon element
			if (hasHoles)
			{
				if (!builder.empty()) builder.pop_back(); // take out the last space
				AddContentUnprocessed(pNode, builder.ToCharPtr());
				builder.clear();

				if (holeIndex < holeCount)
				{
					// Open up a <h> element
					pNode = AddChild(phNode, DAE_HOLE_ELEMENT);
				}
			}

			faceVertexOffset += faceVertexCount;
		}
		holeOffset += holeCount;
	}

	// For the non-holes cases: write out the indices at the very end, for the single <p> element
	if (!hasHoles)
	{
		if (!builder.empty()) builder.pop_back(); // take out the last space
		AddContentUnprocessed(pNode, builder.ToCharPtr());
	}

	// Write out the material semantic and the number of polygons
	if (!geometryPolygons->GetMaterialSemantic().empty())
	{
		AddAttribute(polygonsNode, DAE_MATERIAL_ATTRIBUTE, geometryPolygons->GetMaterialSemantic());
	}

	// Calculate the primitive count, taking into consideration the LINES special case.
	size_t primitiveCount = geometryPolygons->GetFaceCount();
	if (geometryPolygons->GetPrimitiveType() == FCDGeometryPolygons::LINES && primitiveCount > 0)
	{
		primitiveCount = geometryPolygons->GetFaceVertexCount(0) / 2;
	}
	AddAttribute(polygonsNode, DAE_COUNT_ATTRIBUTE, primitiveCount);

	// Write out the extra information tree.
	FArchiveXML::LetWriteObject(geometryPolygons->GetExtra(), polygonsNode);

	return polygonsNode;
}

xmlNode* FArchiveXML::WriteGeometrySpline(FCDObject* object, xmlNode* parentNode)
{
	FCDGeometrySpline* geometrySpline = (FCDGeometrySpline*)object;

	// create as many <spline> node as there are splines in the array
	for (size_t i = 0; i < geometrySpline->GetSplineCount(); ++i)
	{
		FCDSpline* colladaSpline = geometrySpline->GetSpline(i);
		if (colladaSpline == NULL) continue;

		fm::string parentId = geometrySpline->GetParent()->GetDaeId();
		fm::string splineId = FUStringConversion::ToString(i);

		if (colladaSpline->IsType(FCDNURBSSpline::GetClassType()))
		{
			FArchiveXML::WriteNURBSSpline((FCDNURBSSpline*)colladaSpline, parentNode, parentId, splineId);
		}
		else
		{
			FArchiveXML::WriteSpline(colladaSpline, parentNode, parentId, splineId);
		}
	}

	return NULL;
}

xmlNode* FArchiveXML::WriteNURBSSpline(FCDNURBSSpline* nURBSSpline, xmlNode* parentNode, const fm::string& parentId, const fm::string& splineId)
{
	// Create the <spline> XML tree node and set its 'closed' attribute.
	xmlNode* splineNode = AddChild(parentNode, DAE_SPLINE_ELEMENT);
	AddAttribute(splineNode, DAE_CLOSED_ATTRIBUTE, nURBSSpline->IsClosed());

	// Write out the control point, weight and knot sources
	FUSStringBuilder controlPointSourceId(parentId); controlPointSourceId += "-cvs-" + splineId;
	AddSourcePosition(splineNode, controlPointSourceId.ToCharPtr(), nURBSSpline->GetCVs());
	FUSStringBuilder weightSourceId(parentId); weightSourceId += "-weights-" + splineId;
	AddSourceFloat(splineNode, weightSourceId.ToCharPtr(), nURBSSpline->GetWeights(), "WEIGHT");
	FUSStringBuilder knotSourceId(parentId); knotSourceId += "-knots-" + splineId;
	AddSourceFloat(splineNode, knotSourceId.ToCharPtr(), nURBSSpline->GetKnots(), "KNOT");

	// Write out the <control_vertices> element and its inputs
	xmlNode* verticesNode = AddChild(splineNode, DAE_CONTROL_VERTICES_ELEMENT);
	AddInput(verticesNode, controlPointSourceId.ToCharPtr(), DAE_CVS_SPLINE_INPUT);
	AddInput(verticesNode, weightSourceId.ToCharPtr(), DAE_WEIGHT_SPLINE_INPUT);
	AddInput(verticesNode, knotSourceId.ToCharPtr(), DAE_KNOT_SPLINE_INPUT);

	// Write out the <extra> information: the spline type and degree.
	xmlNode* extraNode = AddExtraTechniqueChild(splineNode,DAE_FCOLLADA_PROFILE);
	AddChild(extraNode, DAE_TYPE_ATTRIBUTE, FUDaeSplineType::ToString(nURBSSpline->GetSplineType()));
	AddChild(extraNode, DAE_DEGREE_ATTRIBUTE, FUStringConversion::ToString(nURBSSpline->GetDegree()));
	return splineNode;
}

xmlNode* FArchiveXML::WriteSpline(FCDSpline* spline, xmlNode* parentNode, const fm::string& parentId, const fm::string& splineId)
{
	// Create the spline node with its 'closed' attribute
    xmlNode* splineNode = AddChild(parentNode, DAE_SPLINE_ELEMENT);
	AddAttribute(splineNode, DAE_CLOSED_ATTRIBUTE, spline->IsClosed());

	// Write out the control point source
	FUSStringBuilder controlPointSourceId(parentId); controlPointSourceId += "-cvs-" + splineId;
	AddSourcePosition(splineNode, controlPointSourceId.ToCharPtr(), spline->GetCVs());

	// Write out the <control_vertices> element and its inputs
	xmlNode* verticesNode = AddChild(splineNode, DAE_CONTROL_VERTICES_ELEMENT);
	AddInput(verticesNode, controlPointSourceId.ToCharPtr(), DAE_CVS_SPLINE_INPUT);

	// Write out the spline type
	xmlNode* extraNode = AddExtraTechniqueChild(splineNode, DAE_FCOLLADA_PROFILE);
	AddChild(extraNode, DAE_TYPE_ATTRIBUTE, FUDaeSplineType::ToString(spline->GetSplineType()));
	return splineNode;
}

