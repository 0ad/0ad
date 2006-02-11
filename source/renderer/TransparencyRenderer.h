/**
 * =========================================================================
 * File        : TransparencyRenderer.h
 * Project     : Pyrogenesis
 * Description : ModelRenderer implementation that sorts models and/or
 *             : polygons based on distance from viewer, for transparency
 *             : rendering.
 *
 * @author Rich Cross <rich@wildfiregames.com>
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#ifndef __TRANSPARENCYRENDERER_H
#define __TRANSPARENCYRENDERER_H

#include "renderer/ModelRenderer.h"
#include "renderer/ModelVertexRenderer.h"
#include "renderer/RenderModifiers.h"


struct PolygonSortModelRendererInternals;

/**
 * Class PolygonSortModelRenderer: Render animated models using only
 * OpenGL fixed function, sorting polygons from back to front.
 *
 * This ModelVertexRenderer should only be used with SortModelRenderer.
 * However, SortModelRenderer can be used with other ModelVertexRenderers
 * than this one.
 */
class PolygonSortModelRenderer : public ModelVertexRenderer
{
public:
	PolygonSortModelRenderer();
	~PolygonSortModelRenderer();

	// Implementations
	void* CreateModelData(CModel* model);
	void UpdateModelData(CModel* model, void* data, u32 updateflags);
	void DestroyModelData(CModel* model, void* data);

	void BeginPass(uint streamflags);
	void EndPass(uint streamflags);
	void PrepareModelDef(uint streamflags, CModelDefPtr def);
	void RenderModel(uint streamflags, CModel* model, void* data);

private:
	PolygonSortModelRendererInternals* m;
};

struct SortModelRendererInternals;


/**
 * Class SortModelRenderer: Render models back-to-front from the
 * camera's point of view.
 *
 * This is less efficient than batched model renderers, but
 * necessary for transparent models.
 *
 * TransparencyRenderer can be used with any ModelVertexRenderer.
 *
 * Use this renderer together with TransparentRenderModifier and
 * TransparentShadowRenderModifier to achieve transparency.
 */
class SortModelRenderer : public ModelRenderer
{
public:
	SortModelRenderer(ModelVertexRendererPtr vertexrenderer);
	~SortModelRenderer();

	// Transparency renderer implementation
	void Submit(CModel* model);
	void PrepareModels();
	void EndFrame();
	bool HaveSubmissions();
	void Render(RenderModifierPtr modifier, u32 flags);

private:
	SortModelRendererInternals* m;
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
 * Class TransparentShadowRenderModifier: Use to render shadow data for
 * transparent models into a luminance map.
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

/**
 * Class TransparentDepthShadowModifier: Use to render shadow data for
 * transparent models into a depth texture: Writes into the depth buffer,
 * color data is undefined.
 */
class TransparentDepthShadowModifier : public RenderModifier
{
public:
	TransparentDepthShadowModifier();
	~TransparentDepthShadowModifier();

	// Implementation
	u32 BeginPass(uint pass);
	bool EndPass(uint pass);
	void PrepareTexture(uint pass, CTexture* texture);
	void PrepareModel(uint pass, CModel* model);
};

#endif
