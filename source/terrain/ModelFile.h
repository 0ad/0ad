/************************************************************
 *
 * File Name: ModelFile.H
 *
 * Description: A CModelFile holds the structure of a model
 *				file, and is responsible for reading/writing
 *				a CModelDef from/to a file
 *
 ************************************************************/

#ifndef MODELFILE_H
#define MODELFILE_H

#include <windows.h>

#include "Vector3D.h"
#include "ModelDef.h"

enum EModelFileLumps
{
	MF_VERTICES = 0,
	MF_FACES,
	MF_BONES,
	MF_ANIMKEYS,
	MF_ANIMFRAMES,
	MF_ANIMS,
	MF_NUM_LUMPS
};

struct SLump
{
	int m_Offset;
	int m_Length;
};

struct SModelFile_Vertex
{
	CVector3D m_Coords;
	CVector3D m_Norm;

	float m_U, m_V;
	int m_Bone;
};


struct SModelFile_Face
{
	int m_Verts[3];
};


struct SModelFile_Bone
{
	char m_Name[MAX_NAME_LENGTH];
	
	int m_Parent;
	CVector3D m_Position;
	CVector3D m_Rotation;
};

struct SModelFile_AnimationKey
{
	CVector3D m_Translation;
	CVector3D m_Rotation;
};

//animation keys of one animation for one bone
struct SModelFile_AnimationFrame
{
	int m_FirstKey;
	int m_NumKeys;	//this should always be the same as the number of bones in the model.
};

//a complete animation for all the bones
struct SModelFile_Animation
{
	char m_Name[MAX_NAME_LENGTH];

	int m_FirstFrame;
	int m_NumFrames;
};


class CModelFile
{
	public:
		CModelFile ();
		~CModelFile ();

		bool ReadModelDef (const char *filename, CModelDef *modeldef);
		bool WriteModelDef (const char *filename, CModelDef *modeldef);

	private:
		void Destroy();

	private:
		SLump						m_Lumps[MF_NUM_LUMPS];

		SModelFile_Vertex			*m_pVertices;
		SModelFile_Face				*m_pFaces;
		SModelFile_Bone				*m_pBones;
		SModelFile_AnimationKey		*m_pAnimationKeys;
		SModelFile_AnimationFrame	*m_pAnimationFrames;
		SModelFile_Animation		*m_pAnimations;

		int							m_NumVertices;
		int							m_NumFaces;
		int							m_NumBones;
		int							m_NumAnimationKeys;
		int							m_NumAnimationFrames;
		int							m_NumAnimations;
};


#endif
 
