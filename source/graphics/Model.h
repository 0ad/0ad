///////////////////////////////////////////////////////////////////////////////
//
// Name:		Model.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MODEL_H
#define _MODEL_H

#include "Texture.h"
#include "ModelDef.h"
#include "RenderableObject.h"
#include "SkeletonAnim.h"


///////////////////////////////////////////////////////////////////////////////
// CModel: basically, a mesh object - holds the texturing and skinning 
// information for a model in game
class CModel : public CRenderableObject
{
public:
	struct Prop {
		SPropPoint* m_Point;
		CModel* m_Model;
	};

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

	// calculate object space bounds of this model, based solely on vertex positions
	void CalcObjectBounds();
	// calculate bounds encompassing all vertex positions for given animation 
	void CalcAnimatedObjectBound(CSkeletonAnimDef* anim,CBound& result);
	// return object space bounds
	const CBound& GetObjectBounds() const { return m_ObjectBounds; }

	// set transform of this object, and recurse down into props to update their world space transform
	void SetTransform(const CMatrix3D& transform);
	
	// return the models bone matrices
	const CMatrix3D* GetBoneMatrices() { 
		if (!m_BoneMatricesValid) GenerateBoneMatrices(); 
		return m_BoneMatrices;
	}
	// return the models inverted bone matrices
	const CMatrix3D* GetInvBoneMatrices() { 
		if (!m_BoneMatricesValid) GenerateBoneMatrices(); 
		return m_InvBoneMatrices; 
	}

	// load raw animation frame animation from given file, and build a 
	// animation specific to this model
	CSkeletonAnim* BuildAnimation(const char* filename,float speed);

	// add a prop to the model on the given point
	void AddProp(SPropPoint* point,CModel* model);
	// remove a prop from the given point
	void RemoveProp(SPropPoint* point);
	// return prop list
	const std::vector<Prop>& GetProps() { return m_Props; }

	// return a clone of this model
	CModel* Clone() const;

private:
	// delete anything allocated by the model
	void ReleaseData();
	// calculate necessary bone transformation matrices for skinning
	void GenerateBoneMatrices();

	// texture used by model
	CTexture m_Texture;
	// pointer to the model's raw 3d data
	CModelDef* m_pModelDef;
	// object space bounds of model - accounts for bounds of all possible animations
	// that can play on a model
	CBound m_ObjectBounds;
	// animation currently playing on this model, if any
	CSkeletonAnim* m_Anim;
	// time (in MS) into the current animation
	float m_AnimTime;
	// flag stating whether bone matrices are currently valid
	bool m_BoneMatricesValid;
	// current state of all bones on this model; null if associated modeldef isn't skeletal
	CMatrix3D* m_BoneMatrices;
	// inverse of the above world space transform of the above matrices 
	CMatrix3D* m_InvBoneMatrices;
	// list of current props on model
	std::vector<Prop> m_Props;
};

#endif
