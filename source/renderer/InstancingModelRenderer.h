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
 * Special ModelVertexRender that only works for non-animated models,
 * but is very fast for instanced models.
 */

#ifndef INCLUDED_INSTANCINGMODELRENDERER
#define INCLUDED_INSTANCINGMODELRENDERER

#include "renderer/ModelVertexRenderer.h"

struct InstancingModelRendererInternals;

/**
 * Render non-animated (but potentially moving) models using a ShaderRenderModifier.
 * This just passes through the vertex data directly; the modifier is responsible
 * for setting any shader uniforms etc (including the instancing transform).
 */
class InstancingModelRenderer : public ModelVertexRenderer
{
public:
	InstancingModelRenderer();
	~InstancingModelRenderer();

	// Implementations
	void* CreateModelData(CModel* model);
	void UpdateModelData(CModel* model, void* data, int updateflags);
	void DestroyModelData(CModel* model, void* data);

	void BeginPass(int streamflags);
	void EndPass(int streamflags);
	void PrepareModelDef(CShaderProgramPtr& shader, int streamflags, const CModelDefPtr& def);
	void RenderModel(CShaderProgramPtr& shader, int streamflags, CModel* model, void* data);

protected:
	InstancingModelRendererInternals* m;
};

#endif // INCLUDED_INSTANCINGMODELRENDERER
