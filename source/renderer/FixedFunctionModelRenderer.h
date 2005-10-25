/**
 * =========================================================================
 * File        : FixedFunctionModelRenderer.h
 * Project     : Pyrogenesis
 * Description : BatchModelRenderer that uses only fixed function pipeline
 *             : to render animated models, capable of using RenderModifier.
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#ifndef FIXEDFUNCTIONMODELRENDERER_H
#define FIXEDFUNCTIONMODELRENDERER_H

#include "renderer/ModelRenderer.h"

struct FixedFunctionModelRendererInternals;

/**
 * Class FixedFunctionModelRenderer: Render animated models using only
 * OpenGL fixed function.
 * 
 * Use the RenderModifier to enable normal model rendering as well
 * as player colour rendering using this model renderer.
 */
class FixedFunctionModelRenderer : public BatchModelRenderer
{
public:
	FixedFunctionModelRenderer();
	~FixedFunctionModelRenderer();
	
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
	
protected:
	// Implementations
	void* CreateModelData(CModel* model);
	void UpdateModelData(CModel* model, void* data, u32 updateflags);
	void DestroyModelData(CModel* model, void* data);
	
	void PrepareModelDef(CModelDefPtr def);
	void PrepareTexture(CTexture* texture);
	void RenderModel(CModel* model, void* data);

private:
	FixedFunctionModelRendererInternals* m;
};


#endif // FIXEDFUNCTIONMODELRENDERER_H
