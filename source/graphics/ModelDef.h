///////////////////////////////////////////////////////////////////////////////
//
// Name:		ModelDef.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MODELDEF_H
#define _MODELDEF_H

#include "CStr.h"
#include "Vector3D.h"
#include "MeshManager.h"
#include "SkeletonAnimDef.h"
#include <map>
#include <vector>

class CMeshManager;
class CModelDef;

///////////////////////////////////////////////////////////////////////////////
// SPropPoint: structure describing a prop point
struct SPropPoint
{
	// name of the prop point
	CStr m_Name;
	// position of the point
	CVector3D m_Position;
	// rotation of the point
	CQuaternion m_Rotation;
	// object to parent space transformation 
	CMatrix3D m_Transform;
	// index of parent bone; 0xff if unboned
	u8 m_BoneIndex;
};

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
// CModelDefRPrivate
class CModelDefRPrivate
{
public:
	CModelDefRPrivate() { }
	virtual ~CModelDefRPrivate() { }
};


////////////////////////////////////////////////////////////////////////////////////////
// CModelDef: a raw 3D model; describes the vertices, faces, skinning and skeletal 
// information of a model
class CModelDef
{
	friend class CMeshManager;
public:
	// current file version given to saved animations
	enum { FILE_VERSION = 2 };
	// supported file read version - files with a version less than this will be rejected
	enum { FILE_READ_VERSION = 1 };


public:
	// constructor
	CModelDef();
	// destructor
	virtual ~CModelDef();

	// model I/O functions
	static void Save(const char* filename,const CModelDef* mdef);
	
public:
	// accessor: get vertex data
	size_t GetNumVertices() const { return (size_t)m_NumVertices; }
	SModelVertex* GetVertices() const { return m_pVertices; }

	// accessor: get face data
	size_t GetNumFaces() const { return (size_t)m_NumFaces; }
	SModelFace* GetFaces() const { return m_pFaces; }

	// accessor: get bone data
	size_t GetNumBones() const { return (size_t)m_NumBones; }
	CBoneState* GetBones() const { return m_Bones; }


	// accessor: get prop data
	int GetNumPropPoints() const { return m_NumPropPoints; }
	SPropPoint* GetPropPoints() const { return m_PropPoints; }

	// find and return pointer to prop point matching given name; return
	// null if no match (case insensitive search)
	SPropPoint* FindPropPoint(const char* name) const;

	/**
	 * SetRenderData: Register renderer private data. Use the key to
	 * distinguish between private data used by different render paths.
	 * The private data will be managed by this CModelDef object:
	 * It will be deleted when CModelDef is destructed or when private
	 * data is registered using the same key.
	 *
	 * @param key The opaque key that is used to identify the caller.
	 * The given private data can be retrieved by passing key to GetRenderData.
	 * @param data The private data.
	 *
	 * postconditions : data is bound to the lifetime of this CModelDef
	 * object.
	 */
	void SetRenderData(const void* key, CModelDefRPrivate* data);
	
	// accessor: render data
	CModelDefRPrivate* GetRenderData(const void* key) const;

	// accessor: get model name (for debugging)
	CStr GetName() const { return m_Name; }

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
	// prop point data
	u32 m_NumPropPoints;
	SPropPoint* m_PropPoints;

private:
	// filename
	CStr m_Name;

	// renderdata shared by models of the same modeldef,
	// by render path
	typedef std::map<const void*, CModelDefRPrivate*> RenderDataMap;
	RenderDataMap m_RenderData;

protected:
	static CModelDef* Load(const char* filename);
};

#endif
