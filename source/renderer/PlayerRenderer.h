/**
 * =========================================================================
 * File        : PlayerRenderer
 * Project     : Pyrogenesis
 * Description : RenderModifier for player color rendering, to be used
 *             : with e.g. FixedFunctionModelRenderer
 * =========================================================================
 */

#ifndef INCLUDED_PLAYERRENDERER
#define INCLUDED_PLAYERRENDERER

#include "RenderModifiers.h"


/**
 * Class FastPlayerColorRender: Render models fully textured and lit
 * plus player color in a single pass using multi-texturing (at least 3 TMUs
 * required).
 */
class FastPlayerColorRender : public RenderModifier
{
public:
	FastPlayerColorRender();
	~FastPlayerColorRender();

	// Implementation
	int BeginPass(int pass);
	bool EndPass(int pass);
	void PrepareTexture(int pass, CTexture* texture);
	void PrepareModel(int pass, CModel* model);

	/**
	 * IsAvailable: Determines whether this RenderModifier can be used
	 * given the OpenGL implementation specific limits.
	 *
	 * @note Do not attempt to construct a FastPlayerColorRender object
	 * when IsAvailable returns false.
	 *
	 * @return true if the OpenGL implementation can support this
	 * RenderModifier.
	 */
	static bool IsAvailable();
};


/**
 * Class SlowPlayerColorRender: Render models fully textured and lit
 * plus player color using multi-pass.
 *
 * It has the same visual result as FastPlayerColorRender (except for
 * potential precision issues due to the multi-passing).
 */
class SlowPlayerColorRender : public RenderModifier
{
public:
	SlowPlayerColorRender();
	~SlowPlayerColorRender();

	// Implementation
	int BeginPass(int pass);
	bool EndPass(int pass);
	void PrepareTexture(int pass, CTexture* texture);
	void PrepareModel(int pass, CModel* model);
};


/**
 * Class LitPlayerColorRender: Render models fully textured and lit including shadows
 * and player color.
 *
 * @note Only use a LitPlayerColorRenderer instance when depth texture based shadows
 * are supported by the OpenGL implementation (as verified by CRenderer::m_Caps::m_DepthTextureShadows).
 */
class LitPlayerColorRender : public LitRenderModifier
{
public:
	LitPlayerColorRender();
	~LitPlayerColorRender();

	// Implementation
	int BeginPass(int pass);
	bool EndPass(int pass);
	const CMatrix3D* GetTexGenMatrix(int pass);
	void PrepareTexture(int pass, CTexture* texture);
	void PrepareModel(int pass, CModel* model);
};


#endif
