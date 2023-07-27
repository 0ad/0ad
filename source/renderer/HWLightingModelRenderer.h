/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * ModelVertexRenderer that transforms models on the CPU but performs
 * lighting in a vertex shader.
 */

#ifndef INCLUDED_HWLIGHTINGMODELRENDERER
#define INCLUDED_HWLIGHTINGMODELRENDERER

#include "renderer/ModelVertexRenderer.h"

/**
 * Render animated models using a ShaderRenderModifier.
 * This computes and binds per-vertex data; the modifier is responsible
 * for setting any shader uniforms etc.
 */
class ShaderModelVertexRenderer : public ModelVertexRenderer
{
public:
	ShaderModelVertexRenderer();
	~ShaderModelVertexRenderer();

	CModelRData* CreateModelData(const void* key, CModel* model) override;
	void UpdateModelData(CModel* model, CModelRData* data, int updateflags) override;

	void UploadModelData(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		CModel* model, CModelRData* data) override;
	void PrepareModelDef(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		const CModelDef& def) override;
	void RenderModel(
		Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
		Renderer::Backend::IShaderProgram* shader, CModel* model, CModelRData* data) override;

protected:
	struct ShaderModelRendererInternals;
	ShaderModelRendererInternals* m;
};


#endif // INCLUDED_HWLIGHTINGMODELRENDERER
