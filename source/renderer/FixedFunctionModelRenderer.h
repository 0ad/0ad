/**
 * =========================================================================
 * File        : FixedFunctionModelRenderer.h
 * Project     : Pyrogenesis
 * Description : ModelVertexRenderer that uses only fixed function pipeline
 *             : to render animated models.
 * =========================================================================
 */

#ifndef INCLUDED_FIXEDFUNCTIONMODELRENDERER
#define INCLUDED_FIXEDFUNCTIONMODELRENDERER

#include "renderer/ModelVertexRenderer.h"

struct FixedFunctionModelRendererInternals;

/**
 * Class FixedFunctionModelRenderer: Render animated models using only
 * OpenGL fixed function.
 */
class FixedFunctionModelRenderer : public ModelVertexRenderer
{
public:
	/**
	 * FixedFunctionModelRenderer: Constructor.
	 *
	 * @param colorIsDiffuseOnly if true, the primary color sent to the fragment stage
	 * contains only the diffuse term, and not the ambient
	 */
	FixedFunctionModelRenderer(bool colorIsDiffuseOnly);
	~FixedFunctionModelRenderer();

	// Implementations
	void* CreateModelData(CModel* model);
	void UpdateModelData(CModel* model, void* data, int updateflags);
	void DestroyModelData(CModel* model, void* data);

	void BeginPass(int streamflags, const CMatrix3D* texturematrix);
	void EndPass(int streamflags);
	void PrepareModelDef(int streamflags, const CModelDefPtr& def);
	void RenderModel(int streamflags, CModel* model, void* data);

private:
	FixedFunctionModelRendererInternals* m;
};


#endif // INCLUDED_FIXEDFUNCTIONMODELRENDERER
