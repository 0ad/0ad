/**
 * =========================================================================
 * File        : ModelVertexRenderer.h
 * Project     : Pyrogenesis
 * Description : Definition of ModelVertexRenderer, the abstract base class
 *             : for model vertex transformation implementations.
 * =========================================================================
 */

#ifndef INCLUDED_MODELVERTEXRENDERER
#define INCLUDED_MODELVERTEXRENDERER

#include "graphics/MeshManager.h"

class CModel;

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
	 * model that will later be rendered.
	 *
	 * ModelVertexRenderer implementations should use this function to
	 * create per-CModel and per-CModelDef data like vertex arrays.
	 *
	 * @param model The model.
	 *
	 * @return An opaque pointer that will be passed to other
	 * ModelVertexRenderer functions whenever the CModel is passed again.
	 * Note that returning 0 is allowed and does not indicate an error
	 * condition.
	 */
	virtual void* CreateModelData(CModel* model) = 0;


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
	virtual void UpdateModelData(CModel* model, void* data, int updateflags) = 0;


	/**
	 * DestroyModelData: Release all per-model data that has been allocated
	 * by CreateModelData or UpdateModelData.
	 *
	 * ModelRenderer implementations must ensure that this function is
	 * called exactly once for every call to CreateModelData. This can be
	 * achieved by deriving from CModelRData and calling DestroyModelData
	 * in the derived class' destructor.
	 *
	 * ModelVertexRenderer implementations need not track the CModel
	 * instances for which per-model data has been created.
	 *
	 * @param model The model.
	 * @param data Private data as returned by CreateModelData.
	 */
	virtual void DestroyModelData(CModel* model, void* data) = 0;


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
	 * @param texturematrix if texgen is requested in streamflags, this points to the
	 * texture matrix that must be used to transform vertex positions into texture
	 * coordinates
	 */
	virtual void BeginPass(int streamflags, const CMatrix3D* texturematrix) = 0;


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
	virtual void PrepareModelDef(int streamflags, const CModelDefPtr& def) = 0;


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
	virtual void RenderModel(int streamflags, CModel* model, void* data) = 0;
};


#endif // INCLUDED_MODELVERTEXRENDERER
