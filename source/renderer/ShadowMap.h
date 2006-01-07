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
	 * SetupFrame: Setup shadow map texture and matrices for this frame.
	 *
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

private:
	ShadowMapInternals* m;
};

#endif // SHADOWMAP_H
