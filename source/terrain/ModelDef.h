/************************************************************
 *
 * File Name: ModelDef.H
 *
 * Description: CModelDef is essentially a CModelFile, except
 *				that the data is stored in a more convenient
 *				way. To create a CModelDef, call
 *				CModelFile::ReadModelDef();
 *
 ************************************************************/

#ifndef MODELDEF_H
#define MODELDEF_H

#include "Vector3D.h"
#include "Matrix3D.h"

#define MAX_NAME_LENGTH	(128)


struct SModelVertex
{
	CVector3D m_Coords;
	CVector3D m_Norm;

	float m_U, m_V;
	int m_Bone;
};


struct SModelFace
{
	int m_Verts[3];
};


struct SModelBone
{
	char m_Name[MAX_NAME_LENGTH];
	
	int	m_Parent;
	CVector3D m_Position;
	CVector3D m_Rotation;

	//absolute and relative orientation of this bone
	CMatrix3D m_Relative;
	CMatrix3D m_Absolute;
};

struct SModelAnimationKey
{
	CVector3D m_Translation;
	CVector3D m_Rotation;
};

//An animation frame contains one animation key for each of
//the bones in the model
struct SModelAnimationFrame
{
	int m_FirstKey;
	int m_NumKeys; //this should be that same as number of bones in the model
};

//a complete animation for all the bones
struct SModelAnimation
{
	char m_Name[MAX_NAME_LENGTH];

	int m_FirstFrame;
	int m_NumFrames;
};


class CModelDef
{
	friend class CModelFile;

	public:
		CModelDef ();
		virtual ~CModelDef ();

		void SetupBones();

		void Destroy();

	//access functions
	public:
		SModelVertex			*GetVertices() { return m_pVertices; }
		SModelFace				*GetFaces() { return m_pFaces; }
		SModelBone				*GetBones() { return m_pBones; }
		SModelAnimationKey		*GetAnimationKeys()  { return m_pAnimationKeys; }
		SModelAnimationFrame	*GetAnimationFrames()  { return m_pAnimationFrames; }
		SModelAnimation			*GetAnimations() { return m_pAnimations; }

		int						GetNumVertices() { return m_NumVertices; }
		int						GetNumFaces() { return m_NumFaces; }
		int						GetNumBones() { return m_NumBones; }
		int						GetNumAnimationKeys() { return m_NumAnimationKeys; }
		int						GetNumAnimationFrames() { return m_NumAnimationFrames; }
		int						GetNumAnimations() { return m_NumAnimations; }

	public:
		SModelVertex			*m_pVertices;
		SModelFace				*m_pFaces;
		SModelBone				*m_pBones;
		SModelAnimationKey		*m_pAnimationKeys;
		SModelAnimationFrame	*m_pAnimationFrames;
		SModelAnimation			*m_pAnimations;

		int						m_NumVertices;
		int						m_NumFaces;
		int						m_NumBones;
		int						m_NumAnimationKeys;
		int						m_NumAnimationFrames;
		int						m_NumAnimations;

		char					m_TextureName[MAX_NAME_LENGTH];
};

#endif
