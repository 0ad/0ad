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
 * System for representing tile-based information on top of the
 * terrain.
 */

#ifndef INCLUDED_TERRAINOVERLAY
#define INCLUDED_TERRAINOVERLAY

struct CColor;
class CTerrain;
class CSimContext;

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
 * See the end of TerrainOverlay.h for an example.
 *
 * A TerrainOverlay object will be rendered for as long as it exists.
 *
 */
class TerrainOverlay
{
public:
	virtual ~TerrainOverlay();
private:
	TerrainOverlay(){} // private default ctor (must be subclassed)

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
	virtual void ProcessTile(ssize_t i, ssize_t j) = 0;

	/**
	 * Draw a filled quad on top of the current tile.
	 *
	 * @param colour  colour to draw. May be transparent (alpha &lt; 1)
	 * @param draw_hidden  true if hidden tiles (i.e. those behind other tiles)
	 * should be drawn
	 */
	void RenderTile(const CColor& colour, bool draw_hidden);

	/**
	 * Draw a filled quad on top of the given tile.
	 */
	void RenderTile(const CColor& colour, bool draw_hidden, ssize_t i, ssize_t j);

	/**
	 * Draw an outlined quad on top of the current tile.
	 *
	 * @param colour  colour to draw. May be transparent (alpha &lt; 1)
	 * @param line_width  width of lines in pixels. 1 is a sensible value
	 * @param draw_hidden  true if hidden tiles (i.e. those behind other tiles)
	 * should be drawn
	 */
	void RenderTileOutline(const CColor& colour, int line_width, bool draw_hidden);

	/**
	 * Draw an outlined quad on top of the given tile.
	 */
	void RenderTileOutline(const CColor& colour, int line_width, bool draw_hidden, ssize_t i, ssize_t j);
	
public:
	/**
	 * Draw all TerrainOverlay objects that exist.
	 */
	static void RenderOverlays();

private:
	/// Copying not allowed.
	TerrainOverlay(const TerrainOverlay&);

	friend struct render1st;

	// Process all tiles
	void Render();

	// Temporary storage of tile coordinates, so ProcessTile doesn't need to
	// pass it to RenderTile/etc (and doesn't have a chance to get it wrong)
	ssize_t m_i, m_j;

	CTerrain* m_Terrain;
};


/* Example usage:

class ExampleTerrainOverlay : public TerrainOverlay
{
public:
	char random[1021];

	ExampleTerrainOverlay()
	{
		for (size_t i = 0; i < ARRAY_SIZE(random); ++i)
			random[i] = rand(0, 5);
	}

	virtual void GetTileExtents(
		ssize_t& min_i_inclusive, ssize_t& min_j_inclusive,
		ssize_t& max_i_inclusive, ssize_t& max_j_inclusive)
	{
		min_i_inclusive = 5;
		min_j_inclusive = 10;
		max_i_inclusive = 70;
		max_j_inclusive = 50;
	}

	virtual void ProcessTile(ssize_t i, ssize_t j)
	{
		if (!random[(i*97+j*101) % ARRAY_SIZE(random)])
			return;
		RenderTile(CColor(random[(i*79+j*13) % ARRAY_SIZE(random)]/4.f, 1, 0, 0.3f), false);
		RenderTileOutline(CColor(1, 1, 1, 1), 1, true);
	}
};

ExampleTerrainOverlay test; // or allocate it dynamically somewhere
*/

#endif // INCLUDED_TERRAINOVERLAY
