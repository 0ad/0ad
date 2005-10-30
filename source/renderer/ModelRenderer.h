/**
 * =========================================================================
 * File        : ModelRenderer.h
 * Project     : Pyrogenesis
 * Description : Home to the ModelRenderer class, an abstract base class
 *             : that manages a per-frame list of submitted models,
 *             : as well as simple helper classes.
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#ifndef MODELRENDERER_H
#define MODELRENDERER_H

#include "boost/shared_ptr.hpp"

#include "graphics/MeshManager.h"
#include "graphics/RenderableObject.h"
#include "renderer/VertexArray.h"

class RenderModifier;
typedef boost::shared_ptr<RenderModifier> RenderModifierPtr;

class CModel;


/**
 * Class CModelRData: Render data that is maintained per CModel. 
 * ModelRenderer implementations may derive from this class to store
 * per-CModel data.
 * 
 * The main purpose of this class over CRenderData is to track which
 * ModelRenderer the render data belongs to (via the key that is passed
 * to the constructor). When a model changes the renderer it uses
 * (e.g. via run-time modification of the renderpath configuration),
 * the old ModelRenderer's render data is supposed to be replaced by
 * the new data.
 */
class CModelRData : public CRenderData
{
public:
	CModelRData(const void* key, CModel* model) : m_Key(key), m_Model(model) { }

	/**
	 * GetKey: Retrieve the key that can be used to identify the
	 * ModelRenderer that created this data.
	 *
	 * @return The opaque key that was passed to the constructor.
	 */
	const void* GetKey() const { return m_Key; }

	/**
	 * GetModel: Retrieve the model that this render data object
	 * belongs to.
	 *
	 * @return The model pointer that was passed to the constructor.
	 */
	CModel* GetModel() const { return m_Model; }
	
private:
	/// The key for model renderer identification
	const void* m_Key;
	
	/// The model this object was created for
	CModel* m_Model;
};


/**
 * Class ModelRenderer: Abstract base class for all model renders.
 * 
 * A ModelRenderer manages a per-frame list of models.
 * 
 * It is supposed to be derived in order to create a new render path
 * for models (e.g. for models using special effects).
 * However, consider deriving from BatchModelRenderer or another
 * specialized base class to use their features.
 * 
 * It is suggested that a derived class implement the provided generic
 * Render function, however in some cases it may be necessary to supply
 * a Render function with a different prototype.
 * 
 * ModelRenderer also contains a number of static helper functions
 * for building vertex arrays.
 */
class ModelRenderer
{
public:
	ModelRenderer() { }
	virtual ~ModelRenderer() { }
	
	/**
	 * Submit: Submit a model for rendering this frame.
	 *
	 * preconditions : The model must not have been submitted to any
	 * ModelRenderer in this frame. Submit may only be called
	 * after EndFrame and before PrepareModels.
	 *
	 * @param model The model that will be added to the list of models
	 * submitted this frame.
	 */
	virtual void Submit(CModel* model) = 0;
	
	/**
	 * PrepareModels: Calculate renderer data for all previously
	 * submitted models.
	 *
	 * Must be called before any rendering calls and after all models
	 * for this frame have been submitted.
	 */
	virtual void PrepareModels() = 0;
	
	/**
	 * EndFrame: Remove all models from the list of submitted
	 * models.
	 */
	virtual void EndFrame() = 0;
	
	/**
	 * HaveSubmissions: Return whether any models have been submitted this frame.
	 *
	 * @return true if models have been submitted, false otherwise.
	 */
	virtual bool HaveSubmissions() = 0;
	
	/**
	 * Render: Render submitted models, using the given RenderModifier to setup
	 * the fragment stage.
	 *
	 * @note It is suggested that derived model renderers implement and use 
	 * this Render functions. However, a highly specialized model renderer
	 * may need to "disable" this function and provide its own Render function
	 * with a different prototype.
	 *
	 * preconditions  : PrepareModels must be called after all models have been
	 * submitted and before calling Render.
	 *
	 * @param modifier The RenderModifier that specifies the fragment stage.
	 * @param flags If flags is 0, all submitted models are rendered.
	 * If flags is non-zero, only models that contain flags in their
	 * CModel::GetFlags() are rendered.
	 */
	virtual void Render(RenderModifierPtr modifier, u32 flags) = 0;
	
	/**
	 * CopyPositionAndNormals: Copy unanimated object-space vertices and
	 * normals into the given vertex array.
	 *
	 * @param mdef The underlying CModelDef that contains mesh data.
	 * @param Position Points to the array that will receive
	 * position vectors. The array behind the iterator
	 * must be large enough to hold model->GetModelDef()->GetNumVertices()
	 * vertices.
	 * @param Normal Points to the array that will receive normal vectors.
	 * The array behind the iterator must be as large as the Position array.
	 */
	static void CopyPositionAndNormals(
			CModelDefPtr mdef,
			VertexArrayIterator<CVector3D> Position,
			VertexArrayIterator<CVector3D> Normal);
	
	/**
	 * BuildPositionAndNormals: Build animated vertices and normals, 
	 * transformed into world space.
	 *
	 * @param model The model that is to be transformed.
	 * @param Position Points to the array that will receive
	 * transformed position vectors. The array behind the iterator
	 * must be large enough to hold model->GetModelDef()->GetNumVertices()
	 * vertices.
	 * @param Normal Points to the array that will receive transformed
	 * normal vectors. The array behind the iterator must be as large as
	 * the Position array.
	 */
	static void BuildPositionAndNormals(
			CModel* model,
			VertexArrayIterator<CVector3D> Position,
			VertexArrayIterator<CVector3D> Normal);
	
	/**
	 * BuildColor4ub: Build lighting colors for the given model,
	 * based on previously calculated world space normals.
	 *
	 * @param model The model that is to be lit.
	 * @param Normal Array of the model's normal vectors, animated and
	 * transformed into world space.
	 * @param Color Points to the array that will receive the lit vertex color.
	 * The array behind the iterator must large enough to hold
	 * model->GetModelDef()->GetNumVertices() vertices.
	 */
	static void BuildColor4ub(
			CModel* model,
			VertexArrayIterator<CVector3D> Normal,
			VertexArrayIterator<SColor4ub> Color);
	
	/**
	 * BuildUV: Copy UV coordinates into the given vertex array.
	 *
	 * @param mdef The model def.
	 * @param UV Points to the array that will receive UV coordinates.
	 * The array behind the iterator must large enough to hold
	 * mdef->GetNumVertices() vertices.
	 */
	static void BuildUV(
			CModelDefPtr mdef,
			VertexArrayIterator<float[2]> UV);

	/**
	 * BuildIndices: Create the indices array for the given CModelDef.
	 *
	 * @param mdef The model definition object.
	 * @param Indices The index array, must be able to hold 
	 * mdef->GetNumFaces()*3 elements.
	 */
	static void BuildIndices(
			CModelDefPtr mdef,
			u16* Indices);
};


struct BatchModelRendererInternals;

/**
 * Class BatchModelRenderer: Base class for model renderers that sorts
 * submitted models by CModelDef and texture for batching.
 * 
 * A derived class must provide its own Render function. Depending on
 * the ModelRenderer's purpose, this function may take renderer-dependent
 * arguments and prepare OpenGL state or private data. It should then
 * call RenderAllModels, which will call back into the various PrepareXYZ
 * functions for the actual rendering.
 * 
 * Thus a derived class must also provide implementations for
 * RenderModel and PrepareXYZ.
 * 
 * @note Model renderers that derive from this class MUST NOT set their
 * own per-CModel render data. Use the CreateModelData facility instead.
 */
class BatchModelRenderer : public ModelRenderer
{
	friend struct BatchModelRendererInternals;
	
public:
	BatchModelRenderer();
	virtual ~BatchModelRenderer();

	// Batching implementations
	virtual void Submit(CModel* model);
	virtual void PrepareModels();
	virtual void EndFrame();
	virtual bool HaveSubmissions();

protected:
	/**
	 * CreateModelData: Create renderer's internal data for one model.
	 *
	 * This function must be implemented by derived classes. It will be
	 * called once by Submit for every model, and allows derived classes
	 * to allocate per-model vertex buffers and other data.
	 *
	 * @param model The model.
	 *
	 * @return A data pointer that is opaque to this class, but will be
	 * passed to other functions whenever the CModel is passed again.
	 */
	virtual void* CreateModelData(CModel* model) = 0;
	
	/**
	 * UpdateModelData: Calculate per-model data for each frame.
	 *
	 * This function must be implemented by derived classes. It will be
	 * called once per frame by PrepareModels, regardless of the value
	 * of CRenderData::m_UpdateFlags.
	 * 
	 * This implies that UpdateModelData will be called at least once
	 * between the call to CreateModelData and the first call to RenderModel
	 * for this model.
	 * 
	 * @param model The model.
	 * @param data Private data as returned by CreateModelData.
	 * @param updateflags Flags indicating which data has changed during
	 * the frame. The value is the same as the value of the model's
	 * CRenderData::m_UpdateFlags.
	 */
	virtual void UpdateModelData(CModel* model, void* data, u32 updateflags) = 0;

	/**
	 * DestroyModelData: Release all per-model data that has been allocated
	 * by CreateModelData or UpdateModelData.
	 *
	 * @param model The model.
	 * @param data Private data as returned by CreateModelData.
	 */
	virtual void DestroyModelData(CModel* model, void* data) = 0;

	/**
	 * RenderAllModels: Loop through submitted models and render them
	 * by calling the PrepareXYZ functions and RenderModel as appropriate.
	 *
	 * @param flags Filter models based on CModel::GetFlags(). If flags
	 * is 0, all models are rendered. Otherwise, only models are rendered
	 * for which CModel::GetFlags() & flags returns true.
	 * 
	 * postconditions : RenderAllModels does not affect OpenGL state 
	 * itself. However, it changes OpenGL state indirectly by calling
	 * the PrepareXYZ and RenderModel functions.
	 */
	void RenderAllModels(u32 flags);

	/**
	 * PrepareModelDef: Setup OpenGL state for rendering of models that
	 * use the given CModelDef object as base.
	 * 
	 * This function is called by RenderAllModels before rendering any
	 * models using def as base model definition.
	 *
	 * @param def The model definition.
	 */
	virtual void PrepareModelDef(CModelDefPtr def) = 0;

	/**
	 * PrepareTexture: Setup OpenGL state for rendering of models that
	 * use the given texture.
	 *
	 * This function is called by RenderAllModels before rendering any
	 * models using the given texture.
	 * 
	 * @param texture The texture.
	 */
	virtual void PrepareTexture(CTexture* texture) = 0;
	
	/**
	 * RenderModel: Render the given model.
	 * 
	 * This function is called by RenderAllModels.
	 *
	 * preconditions  : PrepareModelDef and PrepareTexture are called
	 * before calling RenderModel.
	 *
	 * @param model The model that should be rendered.
	 * @param data Private data for the model as returned by CreateModelData.
	 *
	 * postconditions : Subsequent calls to RenderModel for models
	 * that use the same CModelDef object and the same texture must
	 * succeed.
	 */
	virtual void RenderModel(CModel* model, void* data) = 0;

private:
	BatchModelRendererInternals* m;
};


#endif // MODELRENDERER_H
