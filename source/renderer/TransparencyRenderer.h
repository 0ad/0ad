/**
 * =========================================================================
 * File        : TransparencyRenderer.h
 * Project     : Pyrogenesis
 * Description : ModelRenderer implementation that sorts polygons based
 *             : on distance from viewer, for transparency rendering.
 *
 * @author Rich Cross <rich@wildfiregames.com>
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#ifndef __TRANSPARENCYRENDERER_H
#define __TRANSPARENCYRENDERER_H

#include "renderer/ModelRenderer.h"
#include "renderer/RenderModifiers.h"


struct TransparencyRendererInternals;

/**
 * Class TransparencyRenderer: Render transparent models that require
 * Z-based sorting.
 * 
 * This is a lot less efficient than batched model
 * renderers, so it should be used only when really necessary for
 * translucent models. Models that use the alpha channel for masking
 * should probably use a batched model renderer and an appropriate
 * RenderModifier.
 * 
 * Use this renderer together with TransparentRenderModifier and
 * TransparentShadowRenderModifier.
 */
class TransparencyRenderer : public ModelRenderer
{
public:
	TransparencyRenderer();
	~TransparencyRenderer();
	
	// Transparency renderer implementation
	void Submit(CModel* model);
	void PrepareModels();
	void EndFrame();
	bool HaveSubmissions();
	
	/**
	 * Render: Render submitted models, using the given RenderModifier to setup
	 * the fragment stage.
	 *
	 * preconditions  : PrepareModels must be called after all models have been
	 * submitted and before calling Render.
	 *
	 * @param modifier The RenderModifier that specifies the fragment stage.
	 * @param flags If flags is 0, all submitted models are rendered.
	 * If flags is non-zero, only models that contain flags in their
	 * CModel::GetFlags() are rendered.
	 */
	void Render(RenderModifierPtr modifier, u32 flags);

private:
	TransparencyRendererInternals* m;
};


/**
 * Class TransparentRenderModifier: Modifier for transparent models,
 * including alpha blending and lighting.
 */
class TransparentRenderModifier : public RenderModifier
{
public:
	TransparentRenderModifier();
	~TransparentRenderModifier();

	// Implementation
	u32 BeginPass(uint pass);
	bool EndPass(uint pass);
	void PrepareTexture(uint pass, CTexture* texture);
	void PrepareModel(uint pass, CModel* model);

};

/**
 * Class TransparentShadowRenderModifier: Use for shadow rendering of
 * transparent models.
 */
class TransparentShadowRenderModifier : public RenderModifier
{
public:
	TransparentShadowRenderModifier();
	~TransparentShadowRenderModifier();

	// Implementation
	u32 BeginPass(uint pass);
	bool EndPass(uint pass);
	void PrepareTexture(uint pass, CTexture* texture);
	void PrepareModel(uint pass, CModel* model);
};

#endif
