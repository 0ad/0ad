/* Copyright (C) 2022 Wildfire Games.
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
 * System for representing tile-based information on top of the
 * terrain.
 */

#ifndef INCLUDED_TERRAINOVERLAY
#define INCLUDED_TERRAINOVERLAY

#include "renderer/backend/gl/DeviceCommandContext.h"
#include "renderer/backend/gl/Texture.h"

struct CColor;
struct SColor4ub;
class CTerrain;
class CSimContext;

/**
 * Common interface for terrain-tile-based and texture-based debug overlays.
 *
 * An overlay object will be rendered for as long as it is allocated
 * (it is automatically registered/deregistered by constructor/destructor).
 */
class ITerrainOverlay
{
	NONCOPYABLE(ITerrainOverlay);

public:
	virtual ~ITerrainOverlay();

	virtual void RenderBeforeWater(Renderer::Backend::GL::CDeviceCommandContext* UNUSED(deviceCommandContext)) { }

	virtual void RenderAfterWater(
		Renderer::Backend::GL::CDeviceCommandContext* UNUSED(deviceCommandContext), int UNUSED(cullGroup)) { }

	/**
	 * Draw all ITerrainOverlay objects that exist
	 * and that should be drawn before water.
	 */
	static void RenderOverlaysBeforeWater(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext);

	/**
	 * Draw all ITerrainOverlay objects that exist
	 * and that should be drawn after water.
	 */
	static void RenderOverlaysAfterWater(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext, int cullGroup);

protected:
	ITerrainOverlay(int priority);
};

/**
 * Base class for (relatively) simple drawing of
 * data onto terrain tiles, intended for debugging purposes and for the Atlas
 * editor (hence not trying to be very efficient).
 *
 * To start drawing a terrain overlay, first create a subclass of TerrainOverlay.
 * Override the method GetTileExtents if you want to change the range over which
 * it is drawn.
 * Override ProcessTile to do your processing for each tile, which should call
 * RenderTile and RenderTileOutline as appropriate.
 */
class TerrainOverlay : public ITerrainOverlay
{
protected:
	/**
	 * Construct the object and register it with the global
	 * list of terrain overlays.
	 * <p>
	 * The priority parameter controls the order in which overlays are drawn,
	 * if several exist - they are processed in order of increasing priority,
	 * so later ones draw on top of earlier ones.
	 * Most should use the default of 100. Numbers from 200 are used
	 * by Atlas.
	 *
	 * @param priority  controls the order of drawing
	 */
	TerrainOverlay(const CSimContext& simContext, int priority = 100);

	/**
	 * Override to perform processing at the start of the overlay rendering,
	 * before the ProcessTile calls
	 */
	virtual void StartRender();

	/**
	 * Override to perform processing at the end of the overlay rendering,
	 * after the ProcessTile calls
	 */
	virtual void EndRender();

	/**
	 * Override to limit the range over which ProcessTile will
	 * be called. Defaults to the size of the map.
	 *
	 * @param min_i_inclusive  [output] smallest <i>i</i> coordinate, in tile-space units
	 * (1 unit per tile, <i>+i</i> is world-space <i>+x</i> and game-space East)
	 * @param min_j_inclusive  [output] smallest <i>j</i> coordinate
	 * (<i>+j</i> is world-space <i>+z</i> and game-space North)
	 * @param max_i_inclusive  [output] largest <i>i</i> coordinate
	 * @param max_j_inclusive  [output] largest <i>j</i> coordinate
	 */
	virtual void GetTileExtents(ssize_t& min_i_inclusive, ssize_t& min_j_inclusive,
	                            ssize_t& max_i_inclusive, ssize_t& max_j_inclusive);

	/**
	 * Override to perform processing of each tile. Typically calls
	 * RenderTile and/or RenderTileOutline.
	 *
	 * @param i  <i>i</i> coordinate of tile being processed
	 * @param j  <i>j</i> coordinate of tile being processed
	 */
	virtual void ProcessTile(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		ssize_t i, ssize_t j) = 0;

	/**
	 * Draw a filled quad on top of the current tile.
	 *
	 * @param color  color to draw. May be transparent (alpha &lt; 1)
	 * @param drawHidden  true if hidden tiles (i.e. those behind other tiles)
	 * should be drawn
	 */
	void RenderTile(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CColor& color, bool drawHidden);

	/**
	 * Draw a filled quad on top of the given tile.
	 */
	void RenderTile(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CColor& color, bool drawHidden, ssize_t i, ssize_t j);

	/**
	 * Draw an outlined quad on top of the current tile.
	 *
	 * @param color  color to draw. May be transparent (alpha &lt; 1)
	 * @param lineWidth  width of lines in pixels. 1 is a sensible value
	 * @param drawHidden  true if hidden tiles (i.e. those behind other tiles)
	 * should be drawn
	 */
	void RenderTileOutline(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CColor& color, int lineWidth, bool drawHidden);

	/**
	 * Draw an outlined quad on top of the given tile.
	 */
	void RenderTileOutline(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
		const CColor& color, int lineWidth, bool drawHidden, ssize_t i, ssize_t j);

private:
	// Process all tiles
	void RenderBeforeWater(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext) override;

	// Temporary storage of tile coordinates, so ProcessTile doesn't need to
	// pass it to RenderTile/etc (and doesn't have a chance to get it wrong)
	ssize_t m_i, m_j;

	CTerrain* m_Terrain;
};

/**
 * Base class for texture-based terrain overlays, with an arbitrary number of
 * texels per terrain tile, intended for debugging purposes.
 * Subclasses must implement BuildTextureRGBA which will be called each frame.
 */
class TerrainTextureOverlay : public ITerrainOverlay
{
public:
	TerrainTextureOverlay(float texelsPerTile, int priority = 100);

	~TerrainTextureOverlay() override;

protected:
	/**
	 * Called each frame to generate the texture to render on the terrain.
	 * @p data is w*h*4 bytes, where w and h are the terrain size multiplied
	 * by texelsPerTile. @p data defaults to fully transparent, and should
	 * be filled with data in RGBA order.
	 */
	virtual void BuildTextureRGBA(u8* data, size_t w, size_t h) = 0;

	/**
	 * Returns an arbitrary color, for subclasses that want to distinguish
	 * different integers visually.
	 */
	SColor4ub GetColor(size_t idx, u8 alpha) const;

private:
	void RenderAfterWater(
		Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext, int cullGroup) override;

	float m_TexelsPerTile;
	std::unique_ptr<Renderer::Backend::GL::CTexture> m_Texture;
};

#endif // INCLUDED_TERRAINOVERLAY
