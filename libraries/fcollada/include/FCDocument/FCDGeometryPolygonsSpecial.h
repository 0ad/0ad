/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDGeometryPolygonsSpecial.h
	This file defines the FCDGeometryPolygonsSpecial class.
*/

#ifndef _FCD_GEOMETRY_POLYGONS_SPECIAL_H_
#define _FCD_GEOMETRY_POLYGONS_SPECIAL_H_

#ifndef _FCD_GEOMETRY_POLYGONS_H_
#include "FCDocument/FCDGeometryPolygons.h"
#endif // _FCD_GEOMETRY_POLYGONS_H_
#ifndef _FU_DAE_ENUM_H_
#include "FUtils/FUDaeEnum.h"
#endif // _FU_DAE_ENUM_H_


/**
	A mesh set of either triangle fans, triangle strips, lines or line strips.
	Each set contains a list of inputs and indices of the input.

	@ingroup FCDGeometry
*/
class FCOLLADA_EXPORT FCDGeometryPolygonsSpecial : public FCDGeometryPolygons
{
private:
	DeclareObjectType(FCDObject);

	/** The type of primitives described in this geometry */
	PrimitiveType type;

	/** A list containing the number of indices to use for each primitive. This is necessary to know how to
		separate the indices when multiple <p> elements are specified in a mesh. */
	UInt32List numberOfVerticesPerList;

public:
	/** Constructor: do not use directly. Instead, use the FCDGeometryMesh::AddPolygons function
		to create new polygon sets.
		@param document The COLLADA document which owns this polygon set.
		@param parent The geometric mesh which contains this polygon set.*/
	FCDGeometryPolygonsSpecial(FCDocument* document, FCDGeometryMesh* parent, PrimitiveType _type);

	/** Destructor. */
	virtual ~FCDGeometryPolygonsSpecial();

	/** Retrieves the type of primitives described in this geometry (lines, triangle strips, etc).
		@return The type of primitives. */
	inline virtual PrimitiveType GetType() const { return type; }

	/** Retrieves the number of indices to use for each primitive. This is useful if the Collada file
		specifies multiple <p> elements, which get all bundled together in the index list.
		@return The list of number of indices to use for each primitive. */
	inline const UInt32List& GetNumberOfVerticesPerList() const {return numberOfVerticesPerList;}
	inline UInt32List& GetNumberOfVerticesPerList() {return numberOfVerticesPerList;}

	/** Retrieves the number of faces within the set.
		@return The number of faces within the set. */
	inline size_t GetFaceCount() const;

	/** Retrieves the number of face-vertex pairs which appear
		before a given face within the primitive set.
		This value is useful when doing per-vertex mesh operations within the primitive set.
		@param index The index of the face.
		@return The number of face-vertex pairs before the given face, within the primitive set. */
	virtual size_t GetFaceVertexOffset(size_t index) const;

	/** Retrieves the number of face-vertex pairs for a given face.
		This value includes face-vertex pairs that create the primitive and its holes.
		@param index A face index.
		@return The number of face-vertex pairs for a given face. */
	virtual size_t GetFaceVertexCount(size_t index) const;

	/** Creates a new face.
		Enough indices to fill the face will be added to the primitive set inputs: you will
		want to overwrite those, as they will all be set to zero.
		@param degree Unused as the primitive type will determine the number of vertices. */
	virtual void AddFace(uint32 degree);

	/** Removes a face
		@param index The index of the face to remove. All the indices associated
			with this face and all the following ones will also be removed. */
	virtual void RemoveFace(size_t index);

	/** Triangulates the primitive set.
		Not useful for the kinds of primives processed in this class */
	virtual void Triangulate() {};

	/** [INTERNAL] Recalculates the buffered offset and count values for this polygon set. */
	virtual void Recalculate() {};

	/** [INTERNAL] Reads in the polygon set element from a given COLLADA XML tree node.
		COLLADA has multiple polygon set elements. The most common ones are \<triangles\> and \<polylist\>.
		@param polygonNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the polygon set.*/
	virtual bool LoadFromXML(xmlNode* polygonNode);

	/** [INTERNAL] Writes out the correct polygon set element to the given COLLADA XML tree node.
		COLLADA has multiple polygon set elements. The most common ones are \<triangles\> and \<polylist\>.
		@param parentNode The COLLADA XML parent node in which to insert the geometric mesh.
		@return The created XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** [INTERNAL] Creates a copy of this mesh.
		You should use the FCDGeometry::Clone function instead of this function.
		@param clone The clone polygon set.
		@param cloneMap A match-map of the original geometry sources to the clone geometry sources for the mesh.
		@return The clone polygon set. */
//	virtual FCDGeometryPolygons* Clone(FCDGeometryPolygons* clone, const FCDGeometrySourceCloneMap& cloneMap) const;

private:
	// Performs operations needed before tessellation
	virtual bool InitTessellation(xmlNode* itNode, 
		uint32* localFaceVertexCount, UInt32List& allIndices, 
		const char* content, xmlNode*& holeNode, uint32 idxCount, 
		bool* failed);
};


#endif // _FCD_GEOMETRY_POLYGONS_SPECIAL_H_
