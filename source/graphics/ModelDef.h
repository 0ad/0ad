/* Copyright (C) 2011 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Defines a raw 3d model.
 */

#ifndef INCLUDED_MODELDEF
#define INCLUDED_MODELDEF

#include "ps/CStr.h"
#include "maths/Vector2D.h"
#include "maths/Vector3D.h"
#include "maths/Quaternion.h"
#include "lib/file/vfs/vfs_path.h"
#include "renderer/VertexArray.h"
#include <map>
#include <cstring>

class CBoneState;

/**
 * Describes the position of a prop point within its parent model. A prop point is the location within a parent model
 * where the prop's origin will be attached.
 *
 * A prop point is specified by its transformation matrix (or separately by its position and rotation), which
 * can be relative to either the parent model's origin, or one of the parent's bones. If the parent model is boned,
 * then the @ref m_BoneIndex field may specify a bone to which the transformation matrix is relative (see
 * @ref CModel::m_BoneMatrices). Otherwise, the transformation matrix is assumed to be relative to the parent model's
 * origin.
 *
 * @see CModel::m_BoneMatrices
 */
struct SPropPoint
{
	/// Name of the prop point
	CStr m_Name;

	/**
	 * Position of the point within the parent model, relative to either the parent model's origin or one of the parent
	 * model's bones if applicable. Also specified as part of @ref m_Transform.
	 * @see m_Transform
	 */
	CVector3D m_Position;

	/**
	 * Rotation of the prop model that will be attached at this point. Also specified as part of @ref m_Transform.
	 * @see m_Transform
	 */
	CQuaternion m_Rotation;

	/**
	 * Object to parent space transformation. Combines both @ref m_Position and @ref m_Rotation in a single
	 * transformation matrix. This transformation is relative to either the parent model's origin, or one of its
	 * bones, depending on whether it is skeletal. If relative to a bone, then the bone in the parent model to
	 * which this transformation is relative may be found by m_BoneIndex.
	 * @see m_Position, m_Rotation
	 */
	CMatrix3D m_Transform;

	/**
	 * Index of parent bone to which this prop point is relative, if any. The value 0xFF specifies that either the parent
	 * model is unboned, or that this prop point is relative to the parent model's origin rather than one if its bones.
	 */
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

	bool operator==(const SVertexBlend& o) const
	{
		return !memcmp(m_Bone, o.m_Bone, sizeof(m_Bone)) && !memcmp(m_Weight, o.m_Weight, sizeof(m_Weight));
	}
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
	std::vector<float> m_UVs;
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
	NONCOPYABLE(CModelDef);

public:
	// current file version given to saved animations
	enum { FILE_VERSION = 3 };
	// supported file read version - files with a version less than this will be rejected
	enum { FILE_READ_VERSION = 1 };


public:
	CModelDef();
	~CModelDef();

	// model I/O functions

	static void Save(const VfsPath& filename,const CModelDef* mdef);

	/**
	 * Loads a PMD file.
	 * @param filename VFS path of .pmd file to load
	 * @param name arbitrary name to give the model for debugging purposes (usually pathname)
	 * @return the model - always non-NULL
	 * @throw PSERROR_File if it can't load the model
	 */
	static CModelDef* Load(const VfsPath& filename, const VfsPath& name);

public:
	// accessor: get vertex data
	size_t GetNumVertices() const { return m_NumVertices; }
	SModelVertex* GetVertices() const { return m_pVertices; }

	// accessor: get number of UV sets
	size_t GetNumUVsPerVertex() const { return m_NumUVsPerVertex; }

	// accessor: get face data
	size_t GetNumFaces() const { return m_NumFaces; }
	SModelFace* GetFaces() const { return m_pFaces; }

	// accessor: get bone data
	size_t GetNumBones() const { return m_NumBones; }
	CBoneState* GetBones() const { return m_Bones; }
	CMatrix3D* GetInverseBindBoneMatrices() { return m_InverseBindBoneMatrices; }

	// accessor: get blend data
	size_t GetNumBlends() const { return m_NumBlends; }
	SVertexBlend* GetBlends() const { return m_pBlends; }
	size_t* GetBlendIndices() const { return m_pBlendIndices; }

	// find and return pointer to prop point matching given name; return
	// null if no match (case insensitive search)
	const SPropPoint* FindPropPoint(const char* name) const;

	/**
	 * Transform the given vertex's position from the bind pose into the new pose.
	 *
	 * @return new world-space vertex coordinates
	 */
	static CVector3D SkinPoint(const SModelVertex& vtx,
		const CMatrix3D newPoseMatrices[]);

	/**
	 * Transform the given vertex's normal from the bind pose into the new pose.
	 *
	 * @return new world-space vertex normal
	 */
	static CVector3D SkinNormal(const SModelVertex& vtx,
		const CMatrix3D newPoseMatrices[]);

	/**
	 * Transform vertices' positions and normals.
	 * (This is equivalent to looping over SkinPoint and SkinNormal,
	 * but slightly more efficient.)
	 */
	static void SkinPointsAndNormals(
		size_t numVertices,
		const VertexArrayIterator<CVector3D>& Position,
		const VertexArrayIterator<CVector3D>& Normal,
		const SModelVertex* vertices,
		const size_t* blendIndices,
		const CMatrix3D newPoseMatrices[]);

#if HAVE_SSE
	/**
	 * SSE-optimised version of SkinPointsAndNormals.
	 */
	static void SkinPointsAndNormals_SSE(
		size_t numVertices,
		const VertexArrayIterator<CVector3D>& Position,
		const VertexArrayIterator<CVector3D>& Normal,
		const SModelVertex* vertices,
		const size_t* blendIndices,
		const CMatrix3D newPoseMatrices[]);
#endif

	/**
	 * Blend bone matrices together to fill bone palette.
	 */
	void BlendBoneMatrices(CMatrix3D boneMatrices[]);

	/**
	 * Register renderer private data. Use the key to
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
	const VfsPath& GetName() const { return m_Name; }

public:
	// vertex data
	size_t m_NumVertices;
	SModelVertex* m_pVertices;
	size_t m_NumUVsPerVertex; // number of UV pairs per vertex
	// face data
	size_t m_NumFaces;
	SModelFace* m_pFaces;
	// bone data - default model pose
	size_t m_NumBones;
	CBoneState* m_Bones;
	CMatrix3D* m_InverseBindBoneMatrices;
	// blend data
	size_t m_NumBlends;
	SVertexBlend *m_pBlends;
	size_t* m_pBlendIndices;
	// prop point data
	std::vector<SPropPoint> m_PropPoints;

private:
	VfsPath m_Name;	// filename

	// renderdata shared by models of the same modeldef,
	// by render path
	typedef std::map<const void*, CModelDefRPrivate*> RenderDataMap;
	RenderDataMap m_RenderData;
};

#endif

