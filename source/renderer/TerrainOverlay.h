/**
 * =========================================================================
 * File        : TerrainOverlay.h
 * Project     : Pyrogenesis
 * Description : System for representing tile-based information on top of
 *             : the terrain.
 * =========================================================================
 */

#ifndef INCLUDED_TERRAINOVERLAY
#define INCLUDED_TERRAINOVERLAY

//Kai: added for rendering triangulate Terrain Overlay


#include "ps/Overlay.h" // for CColor
#include <math.h>
#include "ps/Vector2D.h"
#include "graphics/Terrain.h"


//Kai: added for line drawing
#include "simulation/Entity.h"
#include "simulation/EntityManager.h"
#include "simulation/BoundingObjects.h"
#include "dcdt/se/se_dcdt.h"
#include "lib/ogl.h"



class CTerrain;

/**
 * Base class for (relatively) simple drawing of
 * data onto terrain tiles, ssize_tended for debugging purposes and for the Atlas
 * editor (hence not trying to be very efficient).
 * <p>
 * To start drawing a terrain overlay, first create a subclass of TerrainOverlay.
 * Override the method GetTileExtents if you want to change the range over which
 * it is drawn.
 * Override ProcessTile to do your processing for each tile, which should call
 * RenderTile and RenderTileOutline as appropriate.
 * See the end of TerrainOverlay.h for an example.
 * <p>
 * A TerrainOverlay object will be rendered for as long as it exists.
 *
 */
class TerrainOverlay
{
public:
	virtual ~TerrainOverlay();

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
	TerrainOverlay(int priority = 100);

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
	 * Draw an outlined quad on top of the current tile.
	 *
	 * @param colour  colour to draw. May be transparent (alpha &lt; 1)
	 * @param line_width  width of lines in pixels. 1 is a sensible value
	 * @param draw_hidden  true if hidden tiles (i.e. those behind other tiles)
	 * should be drawn
	 */
	void RenderTileOutline(const CColor& colour, int line_width, bool draw_hidden);

	

public:

	

	/// Draw all TerrainOverlay objects that exist.
	static void RenderOverlays();

	/**
	 * Kai: added function to draw out line segments for triangulation
	 */
	static void RenderEntityEdges();



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


class TriangulationTerrainOverlay : public TerrainOverlay
{

	SrArray<SrPnt2> constr;
	SrArray<SrPnt2> unconstr;

	//std::vector<CVector2D> path;

	SrPolygon CurPath;
public:
	

	void RenderCurrentPath()
	{
		glColor3f(1,1,1);

		for(int i=0; i< CurPath.size()-1; i++)
		{
			std::vector<CEntity*> results;
			g_EntityManager.GetExtant(results);
			CEntity* tempHandle = results[0];

			glBegin(GL_LINE_LOOP);

			float x1 = CurPath[i].x;
			float y1 = CurPath[i].y;
			float x2 = CurPath[i+1].x;
			float y2 = CurPath[i+1].y;
			glVertex3f(x1,tempHandle->GetAnchorLevel(x1,y1) + 0.2f,y1);
			glVertex3f(x2,tempHandle->GetAnchorLevel(x2,y2) + 0.2f,y2);
			glEnd();
			

		}
	}


	//
	//Kai: added function to draw out constrained line segments in triangulation
	//
	void RenderConstrainedEdges()
	{

		std::vector<CEntity*> results;
		g_EntityManager.GetExtant(results);
		CEntity* tempHandle = results[0];

		glColor3f( 1, 1, 1 );
		

		for(int i=0; i<constr.size()-2; i=i+2)
		{
			glBegin( GL_LINE_LOOP );

			SrPnt2 p1 = constr[i];
			SrPnt2 p2 = constr[i+1];

			float x1 = p1.x;
			float y1 = p1.y;
			float x2 = p2.x;
			float y2 = p2.y;

			glVertex3f( x1, tempHandle->GetAnchorLevel( x1, y1 ) + 0.2f, y1 );
			glVertex3f( x2, tempHandle->GetAnchorLevel( x2, y2 ) + 0.2f, y2 );
			glEnd();

		}

		
	}


	//
	// Kai: added function to draw out unconstrained line segments in triangulation
	//
	void RenderUnconstrainedEdges()
	{
		std::vector<CEntity*> results;
		g_EntityManager.GetExtant(results);
		CEntity* tempHandle = results[0];

		
		glColor3f( 0, 1, 0 );

		for(int i=0; i<unconstr.size()-2; i=i+2)
		{
			glBegin( GL_LINE_LOOP );

			SrPnt2 p1 = unconstr[i];
			SrPnt2 p2 = unconstr[i+1];

			float x1 = p1.x;
			float y1 = p1.y;
			float x2 = p2.x;
			float y2 = p2.y;

			glVertex3f( x1, tempHandle->GetAnchorLevel( x1, y1 ) + 0.2f, y1 );
			glVertex3f( x2, tempHandle->GetAnchorLevel( x2, y2 ) + 0.2f, y2 );
			glEnd();

		}
		
	}

	void setCurrentPath(SrPolygon _CurPath)
	{
		CurPath = _CurPath;

	}
		

	
	

	void setConstrainedEdges(SrArray<SrPnt2> _constr)
	{
		constr = _constr;

	}

	void setUnconstrainedEdges(SrArray<SrPnt2> _unconstr)
	{
		unconstr = _unconstr;

	}

	void Render()
	{
		
	}
	

	TriangulationTerrainOverlay()
	{
		
	}

	virtual void GetTileExtents(
		ssize_t& min_i_inclusive, ssize_t& min_j_inclusive,
		ssize_t& max_i_inclusive, ssize_t& max_j_inclusive)
	{
		min_i_inclusive = 1;
		min_j_inclusive = 1;
		max_i_inclusive = 2;
		max_j_inclusive = 2;
	}

	virtual void ProcessTile(ssize_t UNUSED(i), ssize_t UNUSED(j))
	{
		
		RenderConstrainedEdges();
		RenderUnconstrainedEdges();
		RenderCurrentPath();
		
	}

	

	
};










class PathFindingTerrainOverlay : public TerrainOverlay
{
public:
	char random[1021];
	std::vector<CVector2D> aPath;

	void setPath(std::vector<CVector2D> _aPath)
	{
		aPath =_aPath;

		for(size_t k = 0 ; k< aPath.size();k++)
		{
			aPath[k] = WorldspaceToTilespace( aPath[k] );
		}

	}

	CVector2D WorldspaceToTilespace( const CVector2D &ws )
	{
		return CVector2D(floor(ws.x/CELL_SIZE), floor(ws.y/CELL_SIZE));
	}

	bool inPath(ssize_t i, ssize_t j)
	{
		for(size_t k = 0 ; k<aPath.size();k++)
		{
			if(aPath[k].x== i && aPath[k].y== j)
				return true;
		}

		return false;

	}

	PathFindingTerrainOverlay()
	{
		
	}

	/*virtual void GetTileExtents(
		ssize_t& min_i_inclusive, ssize_t& min_j_inclusive,
		ssize_t& max_i_inclusive, ssize_t& max_j_inclusive)
	{
		min_i_inclusive = 1;
		min_j_inclusive = 1;
		max_i_inclusive = 50;
		max_j_inclusive = 50;
	}*/

	virtual void ProcessTile(ssize_t i, ssize_t j)
	{
		
		if ( inPath( i, j))
		{		
			RenderTile(CColor(random[(i*79+j*13) % ARRAY_SIZE(random)]/4.f, 1, 0, 0.3f), false);
			RenderTileOutline(CColor(1, 1, 1, 1), 1, true);
		}
	}
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
