#ifndef __EXPMESH_H
#define __EXPMESH_H

#include <vector>
#include "MaxInc.h"
#include "lib/types.h"
#include "Vector3D.h"

#include "ExpVertex.h"

class VNormal;
class ExpSkeleton;

////////////////////////////////////////////////////////////////////////
// ExpFace: face declaration used in building mesh geometry
struct ExpFace {
	// vertex indices
	u32 m_V[3];		
	// index of this face in max's face list (necessary, since extra
	// faces may be created in fixing t-junctions, but it's still
	// necessary to map back to MAX for eg. getting material on face)
	u32 m_MAXindex;	
	// smoothing group
	u32 m_Smooth;
	// face normal
	CVector3D m_Normal;
	// face area
	float m_Area;
};

// handy typedefs
typedef std::vector<u32> IndexList;
typedef std::vector<CVector3D> PointList;
typedef std::vector<ExpFace> FaceList;
typedef std::vector<ExpVertex> VertexList;
typedef std::vector<SVertexBlend> VBlendList;

////////////////////////////////////////////////////////////////////////
// ExpMesh: mesh type used in building mesh geometry
class ExpMesh 
{
public:
	// list of faces in mesh
	FaceList m_Faces;
	// list of vertices used by mesh
	VertexList m_Vertices;
	// list of vertex blends used by mesh
	VBlendList m_Blends;
};

////////////////////////////////////////////////////////////////////////
// PMDExpMesh: class used for building output meshes
class PMDExpMesh
{
public:
	PMDExpMesh(INode* node,const ExpSkeleton* skeleton);

	static bool IsMesh(Object* obj);
	ExpMesh* Build();


private:
	// the node we're constructing the mesh from
	INode* m_Node;
	// the skeleton attached to the mesh, if any
	const ExpSkeleton* m_Skeleton;
	
	// construct list of vertices and faces used by mesh
	void BuildVerticesAndFaces(Mesh& mesh,VertexList& vertices,CVector3D* vpoints,VBlendList& vblends,VNormal* vnormals,FaceList& faces);
	// build and return vertex normals, accounting for smoothing groups
	VNormal* BuildVertexNormals(Mesh& mesh,VBlendList& vblends);	
	// build and return vertex blends (determined by Physique), if any 
	void BuildVertexBlends(Mesh& mesh,VBlendList& blends);
	// insert the given bone/weight blend into the given blend set
	void InsertBlend(unsigned char bone,float weight,SVertexBlend& blend);

	CVector3D* BuildVertexPoints(Mesh& mesh,VBlendList& vblends);

};

#endif