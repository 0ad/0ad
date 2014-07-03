/* Copyright (C) 2012 Wildfire Games.
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
 * Definition of ModelVertexRenderer, the abstract base class for model
 * vertex transformation implementations.
 */

#ifndef INCLUDED_MODELVERTEXRENDERER
#define INCLUDED_MODELVERTEXRENDERER

#include "graphics/MeshManager.h"
#include "graphics/ShaderProgramPtr.h"

class CModel;
class CModelRData;

/**
 * Class ModelVertexRenderer: Normal ModelRenderer implementations delegate
 * vertex array management and vertex transformation to an implementation of
 * ModelVertexRenderer.
 *
 * ModelVertexRenderer implementations should be designed so that one
 * instance of the implementation can be used with more than one ModelRenderer
 * simultaneously.
 */
class ModelVertexRenderer
{
public:
	virtual ~ModelVertexRenderer() { }


	/**
	 * CreateModelData: Create internal data for one model.
	 *
	 * ModelRenderer implementations must call this once for every
	 * model that will later be rendered, with @p key set to a value
	 * that's unique to that ModelRenderer.
	 *
	 * ModelVertexRenderer implementations should use this function to
	 * create per-CModel and per-CModelDef data like vertex arrays.
	 *
	 * @param key An opaque pointer to pass to the CModelRData constructor
	 * @param model The model.
	 *
	 * @return A new CModelRData that will be passed into other
	 * ModelVertexRenderer functions whenever the same CModel is used again.
	 */
	virtual CModelRData* CreateModelData(const void* key, CModel* model) = 0;


	/**
	 * UpdateModelData: Calculate per-model data for each frame.
	 *
	 * ModelRenderer implementations must call this once per frame for
	 * every model that is to be rendered in this frame, even if the
	 * value of updateflags will be zero.
	 * This implies that this function will also be called at least once
	 * between a call to CreateModelData and a call to RenderModel.
	 *
	 * ModelVertexRenderer implementations should use this function to
	 * perform software vertex transforms and potentially other per-frame
	 * calculations.
	 *
	 * @param model The model.
	 * @param data Private data as returned by CreateModelData.
	 * @param updateflags Flags indicating which data has changed during
	 * the frame. The value is the same as the value of the model's
	 * CRenderData::m_UpdateFlags.
	 */
	virtual void UpdateModelData(CModel* model, CModelRData* data, int updateflags) = 0;


	/**
	 * BeginPass: Setup global OpenGL state for this ModelVertexRenderer.
	 *
	 * ModelVertexRenderer implementations should prepare "heavy" OpenGL
	 * state such as vertex shader state to prepare for rendering models
	 * and delivering vertex data to the fragment stage as described by
	 * streamflags.
	 *
	 * ModelRenderer implementations must call this function before any
	 * calls to other rendering related functions.
	 *
	 * Recursive calls to BeginPass are not allowed, and every BeginPass
	 * is matched by a corresponding call to EndPass.
	 *
	 * @param streamflags Vertex streams required by the fragment stage.
	 */
	virtual void BeginPass(int streamflags) = 0;


	/**
	 * EndPass: Cleanup OpenGL state set up by BeginPass.
	 *
	 * ModelRenderer implementations must call this function after
	 * rendering related functions for one pass have been called.
	 *
	 * @param streamflags Vertex streams required by the fragment stage.
	 * This equals the streamflags parameter passed on the last call to
	 * BeginPass.
	 */
	virtual void EndPass(int streamflags) = 0;


	/**
	 * PrepareModelDef: Setup OpenGL state for rendering of models that
	 * use the given CModelDef object as base.
	 *
	 * ModelRenderer implementations must call this function before
	 * rendering a sequence of models based on the given CModelDef.
	 * When a ModelRenderer switches back and forth between CModelDefs,
	 * it must call PrepareModelDef for every switch.
	 *
	 * @param streamflags Vertex streams required by the fragment stage.
	 * This equals the streamflags parameter passed on the last call to
	 * BeginPass.
	 * @param def The model definition.
	 */
	virtual void PrepareModelDef(const CShaderProgramPtr& shader, int streamflags, const CModelDef& def) = 0;


	/**
	 * RenderModel: Invoke the rendering commands for the given model.
	 *
	 * ModelRenderer implementations must call this function to perform
	 * the actual rendering.
	 *
	 * preconditions  : The most recent call to PrepareModelDef since
	 * BeginPass has been for model->GetModelDef().
	 *
	 * @param streamflags Vertex streams required by the fragment stage.
	 * This equals the streamflags parameter passed on the last call to
	 * BeginPass.
	 * @param model The model that should be rendered.
	 * @param data Private data for the model as returned by CreateModelData.
	 *
	 * postconditions : Subsequent calls to RenderModel for models
	 * that use the same CModelDef object and the same texture must
	 * succeed.
	 */
	virtual void RenderModel(const CShaderProgramPtr& shader, int streamflags, CModel* model, CModelRData* data) = 0;
};


#endif // INCLUDED_MODELVERTEXRENDERER
