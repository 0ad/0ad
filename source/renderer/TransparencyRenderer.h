/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * ModelRenderer implementation that sorts models and/or polygons based
 * on distance from viewer, for transparency rendering.
 */

#ifndef INCLUDED_TRANSPARENCYRENDERER
#define INCLUDED_TRANSPARENCYRENDERER

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
	void UpdateModelData(CModel* model, void* data, int updateflags);
	void DestroyModelData(CModel* model, void* data);

	void BeginPass(int streamflags, const CMatrix3D* texturematrix);
	void EndPass(int streamflags);
	void PrepareModelDef(int streamflags, const CModelDefPtr& def);
	void RenderModel(int streamflags, CModel* model, void* data);

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
	void Render(const RenderModifierPtr& modifier, int flags);

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
	int BeginPass(int pass);
	bool EndPass(int pass);
	void PrepareTexture(int pass, CTexturePtr& texture);
	void PrepareModel(int pass, CModel* model);

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
	int BeginPass(int pass);
	bool EndPass(int pass);
	void PrepareTexture(int pass, CTexturePtr& texture);
	void PrepareModel(int pass, CModel* model);
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
	int BeginPass(int pass);
	bool EndPass(int pass);
	void PrepareTexture(int pass, CTexturePtr& texture);
	void PrepareModel(int pass, CModel* model);
};

#endif
