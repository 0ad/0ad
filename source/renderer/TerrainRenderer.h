/**
 * =========================================================================
 * File        : TerrainRenderer.h
 * Project     : Pyrogenesis
 * Description : Terrain rendering (everything related to patches and
 *             : water) is encapsulated in TerrainRenderer
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#ifndef TERRAINRENDERER_H
#define TERRAINRENDERER_H

class CPatch;
class WaterManager;

struct TerrainRendererInternals;

/**
 * Class TerrainRenderer: Render everything related to the terrain,
 * especially patches and water.
 */
class TerrainRenderer
{
public:
	TerrainRenderer();
	~TerrainRenderer();

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
	void Submit(CPatch* patch);

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
	 * HaveSubmissions: Query whether any patches have been submitted
	 * for this frame.
	 *
	 * @return @c true if a patch has been submitted for this frame,
	 * @c false otherwise.
	 */
	bool HaveSubmissions();

	/**
	 * RenderTerrain: Render textured terrain (including blends between
	 * different terrain types).
	 *
	 * preconditions  : PrepareForRendering must have been called this
	 * frame before calling RenderTerrain.
	 */
	void RenderTerrain();

	/**
	 * RenderPatches: Render all patches un-textured as polygons.
	 *
	 * preconditions  : PrepareForRendering must have been called this
	 * frame before calling RenderPatches.
	 */
	void RenderPatches();

	/**
	 * RenderOutlines: Render the outline of patches as lines.
	 *
	 * preconditions  : PrepareForRendering must have been called this
	 * frame before calling RenderOutlines.
	 */
	void RenderOutlines();

	/**
	 * RenderWater: Render water for all patches that have been submitted
	 * this frame.
	 *
	 * preconditions  : PrepareForRendering must have been called this
	 * frame before calling RenderWater.
	 */
	void RenderWater();

	/**
	 * ApplyShadowMap: transition measure during refactoring
	 * 
	 * @todo fix this
	 * @deprecated
	 */
	void ApplyShadowMap(GLuint handle);
	
private:
	TerrainRendererInternals* m;
};

#endif // TERRAINRENDERER_H
