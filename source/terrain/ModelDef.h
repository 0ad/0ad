///////////////////////////////////////////////////////////////////////////////
//
// Name:		ModelDef.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MODELDEF_H
#define _MODELDEF_H

///////////////////////////////////////////////////////////////////////////////
// TODO,RC 11/03/04: get rid of all the m_Name[MAX_NAME_LENGTH] - use CStr
//		- problem: conflicts with CStr in MAX's SDK; can't compile PMDExp if 
// ps\CStr.h included here
///////////////////////////////////////////////////////////////////////////////

#include "res/res.h"
#include "Vector3D.h"
#include "SkeletonAnim.h"

#ifndef MAX_NAME_LENGTH	
#define MAX_NAME_LENGTH	(128)
#endif

///////////////////////////////////////////////////////////////////////////////
// SVertexBlend: structure containing the necessary data for blending vertices 
// with multiple bones 
struct SVertexBlend
{
	enum { SIZE = 4 };
	// index of the influencing bone, or 0xff if none
	u8 m_Bone[SIZE];
	// weight of the influence; all weights sum to 1
	float m_Weight[SIZE];
};

///////////////////////////////////////////////////////////////////////////////
// SModelVertex: structure containing per-vertex data
struct SModelVertex
{
	// vertex position
	CVector3D m_Coords;
	// vertex normal
	CVector3D m_Norm;
	// vertex UVs
	float m_U, m_V;
	// vertex blend data
	SVertexBlend m_Blend;
};


///////////////////////////////////////////////////////////////////////////////
// SModelFace: structure containing per-face data
struct SModelFace
{
	// indices of the 3 vertices on this face
	u16 m_Verts[3];
};


////////////////////////////////////////////////////////////////////////////////////////
// CModelDef: a raw 3D model; describes the vertices, faces, skinning and skeletal 
// information of a model
class CModelDef
{
public:
	// current file version given to saved animations
	enum { FILE_VERSION = 1 };
	// supported file read version - files with a version less than this will be rejected
	enum { FILE_READ_VERSION = 1 };


public:
	// constructor
	CModelDef();
	// destructor
	virtual ~CModelDef();

	// model I/O functions
	static CModelDef* Load(const char* filename);
	static void Save(const char* filename,const CModelDef* mdef);
	
public:
	// accessor: get vertex data
	int GetNumVertices() const { return m_NumVertices; }
	SModelVertex *GetVertices() const { return m_pVertices; }

	// accessor: get face data
	int GetNumFaces() const { return m_NumFaces; }
	SModelFace *GetFaces() const { return m_pFaces; }

	// accessor: get bone data
	int GetNumBones() const { return m_NumBones; }
	CBoneState *GetBones() const { return m_Bones; }

public:
	// vertex data
	u32 m_NumVertices;
	SModelVertex* m_pVertices;
	// face data
	u32	m_NumFaces;
	SModelFace* m_pFaces;
	// bone data - default model pose
	u32 m_NumBones;
	CBoneState* m_Bones;
};

#endif
