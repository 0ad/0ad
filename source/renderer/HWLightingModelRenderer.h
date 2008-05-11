/**
 * =========================================================================
 * File        : HWLightingModelRenderer.h
 * Project     : Pyrogenesis
 * Description : ModelVertexRenderer that transforms models on the CPU
 *             : but performs lighting in a vertex shader.
 * =========================================================================
 */

#ifndef INCLUDED_HWLIGHTINGMODELRENDERER
#define INCLUDED_HWLIGHTINGMODELRENDERER

#include "renderer/ModelVertexRenderer.h"

struct HWLightingModelRendererInternals;

/**
 * Class HWLightingModelRenderer: Render animated models using vertex
 * shaders for lighting.
 *
 * @note You should verify hardware capabilities using IsAvailable
 * before creating this model renderer.
 */
class HWLightingModelRenderer : public ModelVertexRenderer
{
public:
	/**
	 * HWLightingModelRenderer: Constructor.
	 *
	 * @param colorIsDiffuseOnly if true, the primary color sent to the fragment stage
	 * contains only the diffuse term, and not the ambient
	 */
	HWLightingModelRenderer(bool colorIsDiffuseOnly);
	~HWLightingModelRenderer();

	// Implementations
	void* CreateModelData(CModel* model);
	void UpdateModelData(CModel* model, void* data, int updateflags);
	void DestroyModelData(CModel* model, void* data);

	void BeginPass(int streamflags, const CMatrix3D* texturematrix);
	void EndPass(int streamflags);
	void PrepareModelDef(int streamflags, CModelDefPtr def);
	void RenderModel(int streamflags, CModel* model, void* data);

	/**
	 * IsAvailable: Determines whether this model renderer can be used
	 * given the OpenGL implementation specific limits.
	 *
	 * @note Do not attempt to construct a HWLightingModelRenderer object
	 * when IsAvailable returns false.
	 *
	 * @return true if the OpenGL implementation can support this
	 * model renderer.
	 */
	static bool IsAvailable();

private:
	HWLightingModelRendererInternals* m;
};


#endif // INCLUDED_HWLIGHTINGMODELRENDERER
