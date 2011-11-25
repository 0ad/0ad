/* Copyright (C) 2009 Wildfire Games.
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
 * Shadow mapping related texture and matrix management
 */

#ifndef INCLUDED_SHADOWMAP
#define INCLUDED_SHADOWMAP

#include "lib/ogl.h"

class CBoundingBoxAligned;
class CMatrix3D;

struct ShadowMapInternals;

/**
 * Class ShadowMap: Maintain the shadow map texture and perform necessary OpenGL setup,
 * including matrix calculations.
 *
 * The class will automatically generate a texture the first time the shadow map is rendered into.
 * The texture will not be resized afterwards.
 */
class ShadowMap
{
public:
	ShadowMap();
	~ShadowMap();

	/**
	 * RecreateTexture: Destroy the current shadow texture and force creation of
	 * a new one. Useful when the renderer's size has changed and the texture
	 * should be resized too.
	 */
	void RecreateTexture();

	/**
	 * GetDepthTextureBits: Return the number of bits to use for depth textures when
	 * enabled.
	 *
	 * @return depth texture bit depth
	 */
	int GetDepthTextureBits() const;

	/**
	 * SetDepthTextureBits: Sets the number of bits to use for depth textures when enabled.
	 * Possible values are 16, 24, 32 and 0 (= use default)
	 *
	 * @param bits number of bits
	 */
	void SetDepthTextureBits(int bits);

	/**
	 * SetupFrame: Configure light space for the given camera and light direction,
	 * create the shadow texture if necessary, etc.
	 *
	 * @param camera the camera that will be used for world rendering
	 * @param lightdir the direction of the (directional) sunlight
	 */
	void SetupFrame(const CCamera& camera, const CVector3D& lightdir);

	/**
	 * AddShadowedBound: Add the bounding box of an object that has to be shadowed.
	 * This is used to calculate the bounds for the shadow map.
	 *
	 * @param bounds world space bounding box
	 */
	void AddShadowedBound(const CBoundingBoxAligned& bounds);

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
	GLuint GetTexture() const;

	/**
	 * GetTextureMatrix: Retrieve the world-space to shadow map texture coordinates
	 * transformation matrix.
	 *
	 * @return the matrix that transforms world-space coordinates into homogenous
	 * shadow map texture coordinates
	 */
	const CMatrix3D& GetTextureMatrix() const;

	/**
	 * RenderDebugDisplay: Visualize shadow mapping calculations to help in
	 * debugging and optimal shadow map usage.
	 */
	void RenderDebugDisplay();

	/**
	 * Get offsets for PCF filtering.
	 */
	const float* GetFilterOffsets() const;

private:
	ShadowMapInternals* m;
};

#endif // INCLUDED_SHADOWMAP
