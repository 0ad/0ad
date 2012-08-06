#ifndef INCLUDED_MIKKWRAP
#define INCLUDED_MIKKWRAP


#include <graphics/mikktspace.h>

class MikkTSpace
{

public:
	
	MikkTSpace(const CModelDefPtr& m, std::vector<float>& v);

	void generate();
	
private:
	
	SMikkTSpaceInterface interface;
	SMikkTSpaceContext context;

	const CModelDefPtr& model;

	std::vector<float>& newVertices;
	

	// Returns the number of faces (triangles/quads) on the mesh to be processed.
	static int getNumFaces(const SMikkTSpaceContext *pContext);


	// Returns the number of vertices on face number iFace
	// iFace is a number in the range {0, 1, ..., getNumFaces()-1}
	static int getNumVerticesOfFace(const SMikkTSpaceContext *pContext, const int iFace);


	// returns the position/normal/texcoord of the referenced face of vertex number iVert.
	// iVert is in the range {0,1,2} for triangles and {0,1,2,3} for quads.
	static void getPosition(const SMikkTSpaceContext *pContext, 
			float fvPosOut[], const int iFace, const int iVert);

	static void getNormal(const SMikkTSpaceContext *pContext, 
			float fvNormOut[], const int iFace, const int iVert);

	static void getTexCoord(const SMikkTSpaceContext *pContext, 
			float fvTexcOut[], const int iFace, const int iVert);


	// either (or both) of the two setTSpace callbacks can be set.
	// The call-back m_setTSpaceBasic() is sufficient for basic normal mapping.

	// This function is used to return the tangent and fSign to the application.
	// fvTangent is a unit length vector.
	// For normal maps it is sufficient to use the following simplified version of the bitangent which is generated at pixel/vertex level.
	// bitangent = fSign * cross(vN, tangent);
	// Note that the results are returned unindexed. It is possible to generate a new index list
	// But averaging/overwriting tangent spaces by using an already existing index list WILL produce INCRORRECT results.
	// DO NOT! use an already existing index list.
	//void setTSpaceBasic(const MikkTSpace *parent, const SMikkTSpaceContext *pContext, 
	//		const float fvTangent[], const float fSign, const int iFace, const int iVert);


	// This function is used to return tangent space results to the application.
	// fvTangent and fvBiTangent are unit length vectors and fMagS and fMagT are their
	// true magnitudes which can be used for relief mapping effects.
	// fvBiTangent is the "real" bitangent and thus may not be perpendicular to fvTangent.
	// However, both are perpendicular to the vertex normal.
	// For normal maps it is sufficient to use the following simplified version of the bitangent which is generated at pixel/vertex level.
	// fSign = bIsOrientationPreserving ? 1.0f : (-1.0f);
	// bitangent = fSign * cross(vN, tangent);
	// Note that the results are returned unindexed. It is possible to generate a new index list
	// But averaging/overwriting tangent spaces by using an already existing index list WILL produce INCRORRECT results.
	// DO NOT! use an already existing index list.
	static void setTSpace(const SMikkTSpaceContext * pContext, const float fvTangent[], 
			const float fvBiTangent[], const float fMagS, const float fMagT, 
			const tbool bIsOrientationPreserving, const int iFace, const int iVert);


};


#endif // INCLUDED_MIKKWRAP