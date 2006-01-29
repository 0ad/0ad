/**
 * =========================================================================
 * File        : ShadowMap.h
 * Project     : Pyrogenesis
 * Description : Shadow mapping related texture and matrix management
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include "ogl.h"

class CBound;
class CMatrix3D;

struct ShadowMapInternals;

/**
 * Class ShadowMap: Maintain the shadow map texture and perform necessary OpenGL setup,
 * including matrix calculations.
 *
 * The class will automatically generate a texture the first time the shadow map is rendered into.
 * The texture will not be resized afterwards.
 *
 * @todo use depth textures for self-shadowing
 */
class ShadowMap
{
public:
	ShadowMap();
	~ShadowMap();

	/**
	 * SetCameraAndLight: Configure light space for the given camera and light direction
	 *
	 * @param camera the camera that will be used for world rendering
	 * @param lightdir the direction of the (directional) sunlight
	 */
	void SetCameraAndLight(const CCamera& camera, const CVector3D& lightdir);

	/**
	 * AddShadowedBound: Add the bounding box of an object that has to be shadowed.
	 * This is used to calculate the bounds for the shadow map.
	 *
	 * @param bounds world space bounding box
	 */
	void AddShadowedBound(const CBound& bounds);

	/**
	 * SetupFrame: Setup shadow map texture and matrices for this frame.
	 *
	 * @deprecated ???
	 * @param visibleBounds bound around objects that are visible on the screen
	 */
	void SetupFrame(const CBound& visibleBounds);

	/**
	 * BeginRender: Set OpenGL state for rendering into the shadow map texture.
	 *
	 * @todo this depends in non-obvious ways on the behaviour of the call-site
	 */
	void BeginRender();

	/**
	 * EndRender: Finish rendering into the shadow map.
	 *
	 * @todo this depends in non-obvious ways on the behaviour of the call-site
	 */
	void EndRender();

	/**
	 * GetTexture: Retrieve the OpenGL texture object name that contains the shadow map.
	 *
	 * @return the texture name of the shadow map texture
	 */
	GLuint GetTexture();

	/**
	 * GetTextureMatrix: Retrieve the world-space to shadow map texture coordinates
	 * transformation matrix.
	 *
	 * @return the matrix that transforms world-space coordinates into homogenous
	 * shadow map texture coordinates
	 */
	const CMatrix3D& GetTextureMatrix();

	/**
	 * RenderDebugDisplay: Visualize shadow mapping calculations to help in
	 * debugging and optimal shadow map usage.
	 */
	void RenderDebugDisplay();

private:
	ShadowMapInternals* m;
};

#endif // SHADOWMAP_H
