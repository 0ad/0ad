///////////////////////////////////////////////////////////////////////////////
//
// Name:		Model.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MODEL_H
#define _MODEL_H

#include "Texture.h"
#include "ModelDef.h"
#include "RenderableObject.h"


///////////////////////////////////////////////////////////////////////////////
// CModel: basically, a mesh object - holds the texturing and skinning 
// information for a model in game
class CModel : public CRenderableObject
{
public:
	// constructor
	CModel();
	// destructor
	~CModel();

	// setup model from given geometry
	bool InitModel(CModelDef *modeldef);
	// calculate the world space bounds of this model
	void CalcBounds();
	// update this model's state; 'time' is the time since the last update, in MS
	void Update(float time);

	// get the model's geometry data
	CModelDef *GetModelDef() { return m_pModelDef; }

	// set the model's texture
	void SetTexture(const CTexture& tex) { m_Texture=tex; }
	// get the model's texture
	CTexture* GetTexture() { return &m_Texture; }

	// set the given animation as the current animation on this model
	bool SetAnimation(CSkeletonAnim* anim); 
	// get the currently playing animation, if any
	CSkeletonAnim* GetAnimation() { return m_Anim; }

	// return the models bone matrices
	const CMatrix3D* GetBoneMatrices() { return m_BoneMatrices; }
	// return the models inverted bone matrices
	const CMatrix3D* GetInvBoneMatrices() { return m_InvBoneMatrices; }

	// return a clone of this model
	CModel* Clone() const;

private:
	// delete anything allocated by the model
	void ReleaseData();

	// texture used by model
	CTexture m_Texture;
	// pointer to the model's raw 3d data
	CModelDef* m_pModelDef;
	// animation currently playing on this model, if any
	CSkeletonAnim* m_Anim;
	// time (in MS) into the current animation
	float m_AnimTime;
	// current state of all bones on this model; null if associated modeldef isn't skeletal
	CMatrix3D* m_BoneMatrices;
	// inverse of all the above matrices
	CMatrix3D* m_InvBoneMatrices;
};

#endif
