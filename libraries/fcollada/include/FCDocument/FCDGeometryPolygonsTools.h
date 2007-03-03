/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDGeometryPolygonsSpecial.h
	This file defines the FCDGeometryPolygonsTools namespace.
*/

#ifndef _FCD_GEOMETRY_POLYGONS_TOOLS_H_
#define _FCD_GEOMETRY_POLYGONS_TOOLS_H_

class FCDGeometryMesh;
class FCDGeometryPolygons;

/** A translation map between old vertex position indices and the new indices.
	It is generated in the FCDGeometryPolygonsTools::GenerateUniqueIndices function. */
typedef fm::map<uint32, UInt32List> FCDGeometryIndexTranslationMap;

/** Holds commonly-used transformation functions for meshes and polygons sets. */
namespace FCDGeometryPolygonsTools
{
	/** Triangulates a mesh.
		@param mesh The mesh to triangulate. */
	FCOLLADA_EXPORT void Triangulate(FCDGeometryMesh* mesh);

	/** Triangulates a polygons set.
		@param polygons The polygons set to triangulate.
		@param recalculate Once the triangulation is done,
			should the statistics of the mesh be recalculated?*/
	FCOLLADA_EXPORT void Triangulate(FCDGeometryPolygons* polygons, bool recalculate = true);

	/** Generates the texture tangents and binormals for a given source of texture coordinates.
		A source of normals and a source of vertex positions will be expected.
		@param mesh The mesh that contains the texture coordinate source.
		@param source The source of texture coordinates that needs its tangents and binormals generated.
		@param generateBinormals Whether the binormals should also be generated.
			Do note that the binormal is always the cross-product of the tangent and the normal at a vertex-face pair. */
	FCOLLADA_EXPORT void GenerateTextureTangentBasis(FCDGeometryMesh* mesh, FCDGeometrySource* texcoordSource, bool generateBinormals = true);

	/** Prepares the mesh for using its geometry sources in vertex buffers with a unique index buffer.
		This is useful for older engines and the applications that only support one index
		per face-vertex pair.
		@param mesh The mesh to process.
		@param polygons The polygons set to isolate and process. If this pointer is NULL, the whole mesh is processed
			for one vertex buffer.
		@param translationMap Optional map that returns how to translate old vertex position indices into new indices.
			This map is necessary to support skins and morphers. */
	FCOLLADA_EXPORT void GenerateUniqueIndices(FCDGeometryMesh* mesh, FCDGeometryPolygons* polygons = NULL, FCDGeometryIndexTranslationMap* translationMap = NULL);
}

#endif // _FCD_GEOMETRY_POLYGONS_TOOLS_H_
