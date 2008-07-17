/**
 * =========================================================================
 * File        : InstancingModelRenderer.h
 * Project     : Pyrogenesis
 * Description : Special ModelVertexRender that only works for non-animated
 *             : models, but is very fast for instanced models.
 * =========================================================================
 */

#ifndef INCLUDED_INSTANCINGMODELRENDERER
#define INCLUDED_INSTANCINGMODELRENDERER

#include "renderer/ModelVertexRenderer.h"

struct InstancingModelRendererInternals;

/**
 * Class InstancingModelRenderer: Render non-animated (but potentially
 * moving models) using vertex shaders and minimal state changes.
 *
 * @note You should verify hardware capabilities using IsAvailable
 * before creating this model renderer.
 */
class InstancingModelRenderer : public ModelVertexRenderer
{
public:
	/**
	 * InstancingModelRenderer: Constructor.
	 *
	 * @param colorIsDiffuseOnly if true, the primary color sent to the fragment stage
	 * contains only the diffuse term, and not the ambient
	 */
	InstancingModelRenderer(bool colorIsDiffuseOnly);
	~InstancingModelRenderer();

	// Implementations
	void* CreateModelData(CModel* model);
	void UpdateModelData(CModel* model, void* data, int updateflags);
	void DestroyModelData(CModel* model, void* data);

	void BeginPass(int streamflags, const CMatrix3D* texturematrix);
	void EndPass(int streamflags);
	void PrepareModelDef(int streamflags, const CModelDefPtr& def);
	void RenderModel(int streamflags, CModel* model, void* data);

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

private:
	InstancingModelRendererInternals* m;
};


#endif // INCLUDED_INSTANCINGMODELRENDERER
