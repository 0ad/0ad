/************************************************************
 *
 * File Name: Model.Cpp
 *
 * Description: CModel is a specific instance of a model.
 *				It contains a pointer to CModelDef, and
 *				includes all instance information, such as
 *				current animation pose.
 *
 ************************************************************/

#include "Model.h"
#include "Quaternion.h"

CModel::CModel()
{
	m_pModelDef = NULL;
	m_pBonePoses = NULL;
}

CModel::~CModel()
{
	Destroy();
}

bool CModel::InitModel(CModelDef *modeldef)
{
	m_pModelDef = modeldef;

	m_pBonePoses = new CMatrix3D[m_pModelDef->GetNumBones()];

	ClearPose();

	return true;
}

void CModel::Destroy()
{
	m_pModelDef = NULL;

	if (m_pBonePoses)
		delete [] m_pBonePoses;

	m_pBonePoses = NULL;
}


void CModel::SetPose (const char *anim_name, float time)
{
	int AnimI = -1;

	for (int i=0; i<m_pModelDef->GetNumAnimations(); i++)
	{
		if ( strcmp(anim_name, m_pModelDef->GetAnimations()[i].m_Name) == 0 )
		{
			AnimI = i;
			break;
		}
	}

	if (AnimI == -1)
		return;

	SModelAnimation *pAnim = &m_pModelDef->GetAnimations()[AnimI];

	int StartI = (int)time;
	int EndI;// = (int)(time + 0.999999f);

	if ((float)StartI == time)
		EndI = StartI;
	else
		EndI = StartI+1;

	float factor = time - (float)StartI;

	if (EndI > pAnim->m_NumFrames-1)
		EndI = 0;

	SModelAnimationFrame *pStartFrame = &m_pModelDef->GetAnimationFrames()[pAnim->m_FirstFrame + StartI];
	SModelAnimationFrame *pEndFrame = &m_pModelDef->GetAnimationFrames()[pAnim->m_FirstFrame + EndI];

	for (i=0; i<m_pModelDef->GetNumBones(); i++)
	{
		SModelAnimationKey *pKey1 = &m_pModelDef->GetAnimationKeys()[pStartFrame->m_FirstKey+i];
		SModelAnimationKey *pKey2 = &m_pModelDef->GetAnimationKeys()[pEndFrame->m_FirstKey+i];

		CVector3D Translation = pKey1->m_Translation + (pKey2->m_Translation - pKey1->m_Translation)*factor;

		m_pBonePoses[i].SetIdentity();

		CQuaternion from, to, rotation;
		from.FromEularAngles (pKey1->m_Rotation.X, pKey1->m_Rotation.Y, pKey1->m_Rotation.Z);
		to.FromEularAngles (pKey2->m_Rotation.X, pKey2->m_Rotation.Y, pKey2->m_Rotation.Z);
		rotation.Slerp (from, to, factor);

		m_pBonePoses[i] = rotation.ToMatrix();

		m_pBonePoses[i].Translate(Translation);

		int Parent = m_pModelDef->GetBones()[i].m_Parent;

		if (Parent > -1)
			m_pBonePoses[i] = m_pBonePoses[Parent] * m_pBonePoses[i];
	}
}

void CModel::ClearPose()
{
	//for each bone, set the bone's pose to the intial pose
	for (int i=0; i<m_pModelDef->GetNumBones(); i++)
		m_pBonePoses[i] = m_pModelDef->GetBones()[i].m_Absolute;
}


void RotateX (CMatrix3D *mat, float angle1, float angle2, float factor)
{
	float Cos = cosf(angle1) + (cosf(angle2)-cosf(angle1))*factor;
	float Sin = sinf(angle1) + (sinf(angle2)-cosf(angle1))*factor;

	CMatrix3D RotX;
	
	RotX._11=1.0f; RotX._12=0.0f; RotX._13=0.0f; RotX._14=0.0f;
	RotX._21=0.0f; RotX._22=Cos;  RotX._23=-Sin; RotX._24=0.0f;
	RotX._31=0.0f; RotX._32=Sin;  RotX._33=Cos;  RotX._34=0.0f;
	RotX._41=0.0f; RotX._42=0.0f; RotX._43=0.0f; RotX._44=1.0f;

	*mat = RotX * (*mat);
}
