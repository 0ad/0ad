/************************************************************
 *
 * File Name: Model.H
 *
 * Description: CModel is a specific instance of a model.
 *				It contains a pointer to CModelDef, and
 *				includes all instance information, such as
 *				current animation pose.
 *
 ************************************************************/

#ifndef MODEL_H
#define MODEL_H

#include "Texture.h"
#include "ModelDef.h"

class CModel
{
	public:
		CModel();
		~CModel();

		bool InitModel(CModelDef *modeldef);
		void Destroy();

		void SetPose (const char *anim_name, float time);
		void ClearPose ();

	//access functions
	public:
		CModelDef *GetModelDef() { return m_pModelDef; }
		CMatrix3D *GetBonePoses() { return m_pBonePoses; }

		void SetTexture(const CTexture& tex) { m_Texture=tex; }
		CTexture* GetTexture() { return &m_Texture; }

	protected:
		CTexture m_Texture;
		CModelDef *m_pModelDef;
		CMatrix3D *m_pBonePoses; //describes the current pose for each bone		
};

#endif
