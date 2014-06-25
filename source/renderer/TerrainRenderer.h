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
 * Terrain rendering (everything related to patches and water) is
 * encapsulated in TerrainRenderer
 */

#ifndef INCLUDED_TERRAINRENDERER
#define INCLUDED_TERRAINRENDERER

class CPatch;
class CSimulation2;
class ShadowMap;
class WaterManager;

struct TerrainRendererInternals;

/**
 * Class TerrainRenderer: Render everything related to the terrain,
 * especially patches and water.
 */
class TerrainRenderer
{
	friend class CPatchRData;
	friend class CDecalRData;
	
public:
	TerrainRenderer();
	~TerrainRenderer();

	/**
	 * Set the simulation context for this frame.
	 * Call at start of frame, before any other Submits.
	 */
	void SetSimulation(CSimulation2* simulation);

	/**
	 * Submit: Add a patch for rendering in this frame.
	 *
	 * preconditions  : PrepareForRendering must not have been called
	 * for this frame yet.
	 * The patch must not have been submitted in this frame yet (i.e. you
	 * can only submit a frame once).
	 *
	 * @param patch the patch
	 */
	void Submit(int cullGroup, CPatch* patch);

	/**
	 * Submit: Add a terrain decal for rendering in this frame.
	 */
	void Submit(int cullGroup, CModelDecal* decal);

	/**
	 * PrepareForRendering: Prepare internal data structures like vertex
	 * buffers for rendering.
	 *
	 * All patches must have been submitted before the call to
	 * PrepareForRendering.
	 * PrepareForRendering must be called before any rendering calls.
	 */
	void PrepareForRendering();

	/**
	 * EndFrame: Remove all patches from the list of submitted patches.
	 */
	void EndFrame();

	/**
	 * RenderTerrain: Render textured terrain (including blends between
	 * different terrain types).
	 *
	 * preconditions  : PrepareForRendering must have been called this
	 * frame before calling RenderTerrain.
	 */
	void RenderTerrain(int cullGroup);

	/**
	 * Render textured terrain, as with RenderTerrain, but using shaders
	 * instead of multitexturing.
	 *
	 * @param shadow A prepared shadow map, in case rendering with shadows is enabled.
	 */
	void RenderTerrainShader(const CShaderDefines& context, int cullGroup, ShadowMap* shadow);

	/**
	 * RenderPatches: Render all patches un-textured as polygons.
	 *
	 * preconditions  : PrepareForRendering must have been called this
	 * frame before calling RenderPatches.
	 *
	 * @param filtered If true then only render objects that passed CullPatches.
	 */
	void RenderPatches(int cullGroup);

	/**
	 * RenderOutlines: Render the outline of patches as lines.
	 *
	 * preconditions  : PrepareForRendering must have been called this
	 * frame before calling RenderOutlines.
	 *
	 * @param filtered If true then only render objects that passed CullPatches.
	 */
	void RenderOutlines(int cullGroup);

	/**
	 * RenderWater: Render water for all patches that have been submitted
	 * this frame.
	 *
	 * preconditions  : PrepareForRendering must have been called this
	 * frame before calling RenderWater.
	 */
	void RenderWater(const CShaderDefines& context, int cullGroup, ShadowMap* shadow);

	/**
	 * Calculate a scissor rectangle for the visible water patches.
	 */
	CBoundingBoxAligned ScissorWater(int cullGroup, const CMatrix3D& viewproj);

	/**
	 * Render priority text for all submitted patches, for debugging.
	 */
	void RenderPriorities(int cullGroup);

	/**
	 * Render texture unit 0 over the terrain mesh, with UV coords calculated
	 * by the given texture matrix.
	 * Intended for use by TerrainTextureOverlay.
	 */
	void RenderTerrainOverlayTexture(int cullGroup, CMatrix3D& textureMatrix);

private:
	TerrainRendererInternals* m;

	/**
	 * RenderFancyWater: internal rendering method for fancy water.
	 * Returns false if unable to render with fancy water.
	 */
	bool RenderFancyWater(const CShaderDefines& context, int cullGroup, ShadowMap* shadow);

	/**
	 * RenderSimpleWater: internal rendering method for water
	 */
	void RenderSimpleWater(int cullGroup);

	static void PrepareShader(const CShaderProgramPtr& shader, ShadowMap* shadow);
};

#endif // INCLUDED_TERRAINRENDERER
