/************************************************************
 *
 * File Name: ModelFile.Cpp
 *
 * Description: A CModelFile holds the structure of a model
 *				file. A model is easily read/written to disk
 *				using this interface.
 *
 ************************************************************/

#include <stdio.h>

#include "ModelFile.h"

CModelFile::CModelFile ()
{
	m_pVertices = NULL;
	m_pFaces = NULL;
	m_pBones = NULL;
	m_pAnimationKeys = NULL;
	m_pAnimationFrames = NULL;
	m_pAnimations = NULL;
	
	m_NumVertices = 0;
	m_NumFaces = 0;
	m_NumBones = 0;
	m_NumAnimationKeys = 0;
	m_NumAnimationFrames = 0;
	m_NumAnimations = 0;
}

CModelFile::~CModelFile()
{
	Destroy();
}

void CModelFile::Destroy()
{
	if (m_pVertices)
		delete [] m_pVertices;
	if (m_pFaces)
		delete [] m_pFaces;
	if (m_pBones)
		delete [] m_pBones;
	if (m_pAnimationKeys)
		delete [] m_pAnimationKeys;
	if (m_pAnimationFrames)
		delete [] m_pAnimationFrames;
	if (m_pAnimations)
		delete [] m_pAnimations;

	m_pVertices = NULL;
	m_pFaces = NULL;
	m_pBones = NULL;
	m_pAnimationKeys = NULL;
	m_pAnimationFrames = NULL;
	m_pAnimations = NULL;
	
	m_NumVertices = 0;
	m_NumFaces = 0;
	m_NumBones = 0;
	m_NumAnimationKeys = 0;
	m_NumAnimationFrames = 0;
	m_NumAnimations = 0;
}

bool CModelFile::WriteModelDef (const char *filename, CModelDef *modeldef)
{
	FILE *out_f = NULL;

	out_f = fopen (filename, "wb");

	m_NumVertices = modeldef->m_NumVertices;
	m_NumFaces = modeldef->m_NumFaces;
	m_NumBones = modeldef->m_NumBones;
	m_NumAnimationKeys = modeldef->m_NumAnimationKeys;
	m_NumAnimationFrames = modeldef->m_NumAnimationFrames;
	m_NumAnimations = modeldef->m_NumAnimations;

	m_pVertices = new SModelFile_Vertex[m_NumVertices];
	m_pFaces = new SModelFile_Face[m_NumFaces];
	m_pBones = new SModelFile_Bone[m_NumBones];
	m_pAnimationKeys = new SModelFile_AnimationKey[m_NumAnimationKeys];
	m_pAnimationFrames = new SModelFile_AnimationFrame[m_NumAnimationFrames];
	m_pAnimations = new SModelFile_Animation[m_NumAnimations];


	for (int i=0; i<m_NumVertices; i++)
	{
		m_pVertices[i].m_Coords = modeldef->m_pVertices[i].m_Coords;
		m_pVertices[i].m_Norm = modeldef->m_pVertices[i].m_Norm;
		m_pVertices[i].m_U = modeldef->m_pVertices[i].m_U;
		m_pVertices[i].m_V = modeldef->m_pVertices[i].m_V;
		m_pVertices[i].m_Bone = modeldef->m_pVertices[i].m_Bone;

		if (m_pVertices[i].m_Bone >= 0)
		{
			SModelBone *pBone = &modeldef->m_pBones[m_pVertices[i].m_Bone];
			m_pVertices[i].m_Coords = pBone->m_Absolute.Transform (m_pVertices[i].m_Coords);
		}
	}

	for (i=0; i<m_NumFaces; i++)
	{
		m_pFaces[i].m_Verts[0] = modeldef->m_pFaces[i].m_Verts[0];
		m_pFaces[i].m_Verts[1] = modeldef->m_pFaces[i].m_Verts[1];
		m_pFaces[i].m_Verts[2] = modeldef->m_pFaces[i].m_Verts[2];
	}

	for (i=0; i<m_NumBones; i++)
	{
		strcpy (m_pBones[i].m_Name, modeldef->m_pBones[i].m_Name);
		m_pBones[i].m_Position = modeldef->m_pBones[i].m_Position;
		m_pBones[i].m_Rotation = modeldef->m_pBones[i].m_Rotation;
		m_pBones[i].m_Parent = modeldef->m_pBones[i].m_Parent;
	}

	for (i=0; i<m_NumAnimationKeys; i++)
	{
		m_pAnimationKeys[i].m_Translation = modeldef->m_pAnimationKeys[i].m_Translation;
		m_pAnimationKeys[i].m_Rotation = modeldef->m_pAnimationKeys[i].m_Rotation;
	}

	for (i=0; i<m_NumAnimationFrames; i++)
	{
		m_pAnimationFrames[i].m_FirstKey = modeldef->m_pAnimationFrames[i].m_FirstKey;
		m_pAnimationFrames[i].m_NumKeys = modeldef->m_pAnimationFrames[i].m_NumKeys;
	}

	for (i=0; i<m_NumAnimations; i++)
	{
		strcpy (m_pAnimations[i].m_Name, modeldef->m_pAnimations[i].m_Name);
		m_pAnimations[i].m_FirstFrame = modeldef->m_pAnimations[i].m_FirstFrame;
		m_pAnimations[i].m_NumFrames = modeldef->m_pAnimations[i].m_NumFrames;
	}


	m_Lumps[MF_VERTICES].m_Offset = MF_NUM_LUMPS*sizeof(SLump);
	m_Lumps[MF_VERTICES].m_Length = m_NumVertices*sizeof(SModelFile_Vertex);

	m_Lumps[MF_FACES].m_Offset = m_Lumps[MF_VERTICES].m_Offset + m_Lumps[MF_VERTICES].m_Length;
	m_Lumps[MF_FACES].m_Length = m_NumFaces*sizeof(SModelFile_Face);

	m_Lumps[MF_BONES].m_Offset = m_Lumps[MF_FACES].m_Offset + m_Lumps[MF_FACES].m_Length;
	m_Lumps[MF_BONES].m_Length = m_NumBones*sizeof(SModelFile_Bone);

	m_Lumps[MF_ANIMKEYS].m_Offset = m_Lumps[MF_BONES].m_Offset + m_Lumps[MF_BONES].m_Length;
	m_Lumps[MF_ANIMKEYS].m_Length = m_NumAnimationKeys*sizeof(SModelFile_AnimationKey);

	m_Lumps[MF_ANIMFRAMES].m_Offset = m_Lumps[MF_ANIMKEYS].m_Offset + m_Lumps[MF_ANIMKEYS].m_Length;
	m_Lumps[MF_ANIMFRAMES].m_Length = m_NumAnimationFrames*sizeof(SModelFile_AnimationFrame);

	m_Lumps[MF_ANIMS].m_Offset = m_Lumps[MF_ANIMFRAMES].m_Offset + m_Lumps[MF_ANIMFRAMES].m_Length;
	m_Lumps[MF_ANIMS].m_Length = m_NumAnimations*sizeof(SModelFile_Animation);

	//write the lumps
	if (fwrite (m_Lumps, sizeof(SLump), MF_NUM_LUMPS, out_f) != MF_NUM_LUMPS)
	{
		fclose (out_f);
		return false;
	}

	//write all the data
	if (fwrite (m_pVertices, sizeof (SModelFile_Vertex), m_NumVertices, out_f) != (unsigned)m_NumVertices)
	{
		fclose (out_f);
		return false;
	}

	if (fwrite (m_pFaces, sizeof (SModelFile_Face), m_NumFaces, out_f) != (unsigned)m_NumFaces)
	{
		fclose (out_f);
		return false;
	}

	if (fwrite (m_pBones, sizeof (SModelFile_Bone),	m_NumBones, out_f) != (unsigned)m_NumBones)
	{
		fclose (out_f);
		return false;
	}

	if (fwrite (m_pAnimationKeys, sizeof (SModelFile_AnimationKey),	m_NumAnimationKeys, out_f) != (unsigned)m_NumAnimationKeys)
	{
		fclose (out_f);
		return false;
	}

	if (fwrite (m_pAnimationFrames, sizeof (SModelFile_AnimationFrame), m_NumAnimationFrames, out_f) != (unsigned)m_NumAnimationFrames)
	{
		fclose (out_f);
		return false;
	}

	if (fwrite (m_pAnimations, sizeof (SModelFile_Animation), m_NumAnimations, out_f) != (unsigned)m_NumAnimations)
	{
		fclose (out_f);
		return false;
	}

	fclose (out_f);

	Destroy();

	return true;
}


bool CModelFile::ReadModelDef (const char *filename, CModelDef *modeldef)
{
	FILE *in_f = NULL;

	in_f = fopen (filename, "rb");
	if (!in_f) {
		return false;
	}

	//read the lumps first
	if (fread (m_Lumps, sizeof(SLump), MF_NUM_LUMPS, in_f) != MF_NUM_LUMPS)
	{
		fclose (in_f);
		return false;
	}

	//calculate the number of each element
	m_NumVertices = m_Lumps[MF_VERTICES].m_Length / sizeof(SModelFile_Vertex);
	m_NumFaces = m_Lumps[MF_FACES].m_Length / sizeof(SModelFile_Face);
	m_NumBones = m_Lumps[MF_BONES].m_Length / sizeof(SModelFile_Bone);
	m_NumAnimationKeys = m_Lumps[MF_ANIMKEYS].m_Length / sizeof(SModelFile_AnimationKey);
	m_NumAnimationFrames = m_Lumps[MF_ANIMFRAMES].m_Length / sizeof(SModelFile_AnimationFrame);
	m_NumAnimations = m_Lumps[MF_ANIMS].m_Length / sizeof(SModelFile_Animation);

	//allocate memory
	m_pVertices = new SModelFile_Vertex[m_NumVertices];
	m_pFaces = new SModelFile_Face[m_NumFaces];
	m_pBones = new SModelFile_Bone[m_NumBones];
	m_pAnimationKeys = new SModelFile_AnimationKey[m_NumAnimationKeys];
	m_pAnimationFrames = new SModelFile_AnimationFrame[m_NumAnimationFrames];
	m_pAnimations = new SModelFile_Animation[m_NumAnimations];


	//read all the data
	fseek (in_f, m_Lumps[MF_VERTICES].m_Offset, SEEK_SET);
	if (fread (m_pVertices, sizeof(SModelFile_Vertex), m_NumVertices, in_f) != (unsigned)m_NumVertices)
	{
		fclose (in_f);
		return false;
	}

	fseek (in_f, m_Lumps[MF_FACES].m_Offset, SEEK_SET);
	if (fread (m_pFaces, sizeof (SModelFile_Face), m_NumFaces, in_f) != (unsigned)m_NumFaces)
	{
		fclose (in_f);
		return false;
	}

	fseek (in_f, m_Lumps[MF_BONES].m_Offset, SEEK_SET);
	if (fread (m_pBones, sizeof (SModelFile_Bone), m_NumBones, in_f) != (unsigned)m_NumBones)
	{
		fclose (in_f);
		return false;
	}

	fseek (in_f, m_Lumps[MF_ANIMKEYS].m_Offset, SEEK_SET);
	if (fread (m_pAnimationKeys, sizeof (SModelFile_AnimationKey), m_NumAnimationKeys, in_f) != (unsigned)m_NumAnimationKeys)
	{
		fclose (in_f);
		return false;
	}

	fseek (in_f, m_Lumps[MF_ANIMFRAMES].m_Offset, SEEK_SET);
	if (fread (m_pAnimationFrames, sizeof (SModelFile_AnimationFrame), m_NumAnimationFrames, in_f) != (unsigned)m_NumAnimationFrames)
	{
		fclose (in_f);
		return false;
	}

	fseek (in_f, m_Lumps[MF_ANIMS].m_Offset, SEEK_SET);
	if (fread (m_pAnimations, sizeof (SModelFile_Animation), m_NumAnimations, in_f) != (unsigned)m_NumAnimations)
	{
		fclose (in_f);
		return false;
	}

	fclose (in_f);

	modeldef->Destroy();

	modeldef->m_NumVertices = m_NumVertices;
	modeldef->m_NumFaces = m_NumFaces;
	modeldef->m_NumBones = m_NumBones;
	modeldef->m_NumAnimationKeys = m_NumAnimationKeys;
	modeldef->m_NumAnimationFrames = m_NumAnimationFrames;
	modeldef->m_NumAnimations = m_NumAnimations;

	modeldef->m_pVertices = new SModelVertex[m_NumVertices];
	modeldef->m_pFaces = new SModelFace[m_NumFaces];
	modeldef->m_pBones = new SModelBone[m_NumBones];
	modeldef->m_pAnimationKeys = new SModelAnimationKey[m_NumAnimationKeys];
	modeldef->m_pAnimationFrames = new SModelAnimationFrame[m_NumAnimationFrames];
	modeldef->m_pAnimations = new SModelAnimation[m_NumAnimations];


	for (int i=0; i<m_NumVertices; i++)
	{
		modeldef->m_pVertices[i].m_Coords = m_pVertices[i].m_Coords;
		modeldef->m_pVertices[i].m_Norm = m_pVertices[i].m_Norm;
		modeldef->m_pVertices[i].m_U = m_pVertices[i].m_U;
		modeldef->m_pVertices[i].m_V = m_pVertices[i].m_V;
		modeldef->m_pVertices[i].m_Bone = m_pVertices[i].m_Bone;
	}

	for (i=0; i<m_NumFaces; i++)
	{
		modeldef->m_pFaces[i].m_Verts[0] = m_pFaces[i].m_Verts[0];
		modeldef->m_pFaces[i].m_Verts[1] = m_pFaces[i].m_Verts[1];
		modeldef->m_pFaces[i].m_Verts[2] = m_pFaces[i].m_Verts[2];
	}

	for (i=0; i<m_NumBones; i++)
	{
		strcpy (modeldef->m_pBones[i].m_Name, m_pBones[i].m_Name);
		modeldef->m_pBones[i].m_Position = m_pBones[i].m_Position;
		modeldef->m_pBones[i].m_Rotation = m_pBones[i].m_Rotation;
		modeldef->m_pBones[i].m_Parent = m_pBones[i].m_Parent;
	}

	for (i=0; i<m_NumAnimationKeys; i++)
	{
		modeldef->m_pAnimationKeys[i].m_Translation = m_pAnimationKeys[i].m_Translation;
		modeldef->m_pAnimationKeys[i].m_Rotation = m_pAnimationKeys[i].m_Rotation;
	}

	for (i=0; i<m_NumAnimationFrames; i++)
	{
		modeldef->m_pAnimationFrames[i].m_FirstKey = m_pAnimationFrames[i].m_FirstKey;
		modeldef->m_pAnimationFrames[i].m_NumKeys = m_pAnimationFrames[i].m_NumKeys;
	}

	for (i=0; i<m_NumAnimations; i++)
	{
		strcpy (modeldef->m_pAnimations[i].m_Name, m_pAnimations[i].m_Name);
		modeldef->m_pAnimations[i].m_FirstFrame = m_pAnimations[i].m_FirstFrame;
		modeldef->m_pAnimations[i].m_NumFrames = m_pAnimations[i].m_NumFrames;
	}

	modeldef->SetupBones();

	Destroy();

	return true;
}
	
