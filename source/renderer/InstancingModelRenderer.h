/**
 * =========================================================================
 * File        : InstancingModelRenderer.h
 * Project     : Pyrogenesis
 * Description : Special BatchModelRenderer that only works for non-animated
 *             : models, but performs all transformation in a vertex shader.
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#ifndef INSTANCINGMODELRENDERER_H
#define INSTANCINGMODELRENDERER_H

#include "renderer/ModelRenderer.h"

struct InstancingModelRendererInternals;

/**
 * Class InstancingModelRenderer: Render non-animated (but potentially
 * moving models) using vertex shaders.
 * 
 * Use the RenderModifier to enable normal model rendering as well
 * as player colour rendering using this model renderer.
 * 
 * @note You should verify hardware capabilities using IsAvailable
 * before creating this model renderer.
 */
class InstancingModelRenderer : public BatchModelRenderer
{
public:
	InstancingModelRenderer();
	~InstancingModelRenderer();
	
	/**
	 * Render: Render submitted models using the given RenderModifier
	 * for fragment stages.
	 *
	 * preconditions  : PrepareModels must be called before Render.
	 *
	 * @param modifier The RenderModifier that specifies the fragment stage.
	 * @param flags If flags is 0, all submitted models are rendered.
	 * If flags is non-zero, only models that contain flags in their
	 * CModel::GetFlags() are rendered.
	 */
	void Render(RenderModifierPtr modifier, u32 flags);
	
	/**
	 * IsAvailable: Determines whether this model renderer can be used
	 * given the OpenGL implementation specific limits.
	 * 
	 * @note Do not attempt to construct a InstancingModelRenderer object
	 * when IsAvailable returns false.
	 * 
	 * @return true if the OpenGL implementation can support this
	 * model renderer.
	 */
	static bool IsAvailable();
	
protected:
	// Implementations
	void* CreateModelData(CModel* model);
	void UpdateModelData(CModel* model, void* data, u32 updateflags);
	void DestroyModelData(CModel* model, void* data);

	void PrepareModelDef(CModelDefPtr def);
	void PrepareTexture(CTexture* texture);
	void RenderModel(CModel* model, void* data);

private:
	InstancingModelRendererInternals* m;
};


#endif // INSTANCINGMODELRENDERER_H
