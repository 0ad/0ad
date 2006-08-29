#include "precompiled.h"

#include "TerritoryManager.h"

#include "ps/Game.h"
#include "ps/Player.h"
#include "graphics/Terrain.h"
#include "graphics/GameView.h"
#include "Entity.h"
#include "EntityManager.h"
#include "graphics/Unit.h"
#include "maths/Bound.h"
#include "graphics/Model.h"
#include "lib/allocators.h"
#include "lib/timer.h"
#include "lib/ogl.h"
#include "EntityManager.h"
#include "EntityTemplate.h"

CTerritoryManager::CTerritoryManager()
{
	m_TerritoryMatrix = 0;
	m_DelayedRecalculate = false;
}

CTerritoryManager::~CTerritoryManager() 
{
	if(m_TerritoryMatrix) 
	{
		matrix_free( (void**) m_TerritoryMatrix );
		m_TerritoryMatrix = 0;
	}

	for( size_t i=0; i<m_Territories.size(); i++)
		delete m_Territories[i];
	m_Territories.clear();
}

void CTerritoryManager::Initialize() 
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	m_TilesPerSide = terrain->GetVerticesPerSide() - 1;

	m_TerritoryMatrix = (CTerritory***) matrix_alloc( m_TilesPerSide, m_TilesPerSide, sizeof(CTerritory*) );
	Recalculate();
}

void CTerritoryManager::Recalculate() 
{
	// Delete any territories created last time we called Recalculate()

	for( size_t i=0; i<m_Territories.size(); i++)
	{
		if( m_Territories[i]->centre )
			m_Territories[i]->centre->m_associatedTerritory = 0;
		delete m_Territories[i];
	}
	m_Territories.clear();

	// First, find all the units that are territory centres
	std::vector<CEntity*> centres;
	std::vector<CEntity*> entities;
	g_EntityManager.GetExtant(entities);
	for( size_t i=0; i<entities.size(); i++ )
	{
		if( !entities[i]->entf_get(ENTF_DESTROYED) && entities[i]->m_base->m_isTerritoryCentre ) 
			centres.push_back(entities[i]);
	}

	int mapSize = m_TilesPerSide * CELL_SIZE;

	// If there aren't any centre objects, create one big Gaia territory which spans the whole map
	if( centres.empty() )
	{
		std::vector<CVector2D> boundary;
		boundary.push_back( CVector2D(0, 0) );
		boundary.push_back( CVector2D(0, mapSize) );
		boundary.push_back( CVector2D(mapSize, mapSize) );
		boundary.push_back( CVector2D(mapSize, 0) );

		CTerritory* ter = new CTerritory( g_Game->GetPlayer(0), HEntity(), boundary );

		m_Territories.push_back(ter);

		for( uint x=0; x<m_TilesPerSide; x++ )
		{
			for( uint z=0; z<m_TilesPerSide; z++ )
			{
				m_TerritoryMatrix[x][z] = ter;
			}
		}
	}
	else
	{
		// For each centre object, create a territory
		for( size_t i=0; i<centres.size(); i++ )
		{
			std::vector<CVector2D> boundary;
			CalculateBoundary( centres, i, boundary );
			
			CTerritory* ter = new CTerritory( centres[i]->GetPlayer(), centres[i]->me, boundary );

			centres[i]->m_associatedTerritory = ter;
			m_Territories.push_back(ter);
		}

		// For each tile, match it to the closest centre object to it.

		// TODO: Optimize this, for example by intersecting scanlines with the Voronoi polygons.

		for( uint x=0; x<m_TilesPerSide; x++ )
		{
			for( uint z=0; z<m_TilesPerSide; z++ )
			{
				CVector2D tileLoc( (x+0.5f) * CELL_SIZE, (z+0.5f) * CELL_SIZE );
				float bestSquareDist = 1e20f;
				for( size_t i=0; i<centres.size(); i++ )
				{
					CVector2D centreLoc( centres[i]->m_position.X, centres[i]->m_position.Z );
					float squareDist = (centreLoc - tileLoc).length2();
					if( squareDist < bestSquareDist )
					{
						bestSquareDist = squareDist;
						m_TerritoryMatrix[x][z] = m_Territories[i];
					}
				}
			}
		}
	}
}

void CTerritoryManager::DelayedRecalculate()
{
	// This is useful particularly for Atlas, which wants to recalculate
	// the boundaries as you move an object around but which doesn't want
	// to waste time recalculating multiple times per frame
	m_DelayedRecalculate = true;
}

CTerritory* CTerritoryManager::GetTerritory(int x, int z)
{
	debug_assert( (uint) x < m_TilesPerSide && (uint) z < m_TilesPerSide );
	return m_TerritoryMatrix[x][z];
}

CTerritory* CTerritoryManager::GetTerritory(float x, float z)
{
	int ix, iz;
	CTerrain::CalcFromPosition(x, z, ix, iz);
	return GetTerritory(ix, iz);
}

// Calculate the boundary points of a given territory into the given vector
void CTerritoryManager::CalculateBoundary( std::vector<CEntity*>& centres, size_t myIndex, std::vector<CVector2D>& boundary )
{
	// Start with a boundary equal to the whole map
	int mapSize = m_TilesPerSide * CELL_SIZE;
	boundary.push_back( CVector2D(0, 0) );
	boundary.push_back( CVector2D(0, mapSize) );
	boundary.push_back( CVector2D(mapSize, mapSize) );
	boundary.push_back( CVector2D(mapSize, 0) );

	// Clip this polygon against the perpendicular bisector between this centre and each other territory centre
	CVector2D myPos( centres[myIndex]->m_position.X, centres[myIndex]->m_position.Z );

	for( size_t i=0; i<centres.size(); i++ )
	{
		if( i != myIndex )
		{
			CVector2D itsPos( centres[i]->m_position.X, centres[i]->m_position.Z );
			CVector2D midpoint = (myPos + itsPos) / 2.0f;
			CVector2D normal = itsPos - myPos;

			// Clip our polygon to the negative side of the half-space with normal "normal"
			// containing point "midpoint", i.e. the side of the perpendicular bisector
			// between myPos and itsPos that contains myPos. We do this by tracing around
			// the polygon looking at each vertex to decide which ones to add as follows:
			// - If a vertex is inside the half-space, take it.
			// - If a vertex is inside but the next one is outside, also take the
			//   intersection of that edge with the perpendicular bisector.
			// - If a vertex is outside but the next one is inside, take the
			//   intersection of that edge with the perpendicular bisector.

			std::vector<CVector2D> newBoundary;
			for( size_t j=0; j<boundary.size(); j++ )
			{
				CVector2D& pos = boundary[j];
				float dot = (pos - midpoint).dot(normal);
				bool inside = dot < 0.0f;
					
				size_t nextJ = (j+1) % boundary.size();		// index of next point
				CVector2D& nextPos = boundary[nextJ];
				float nextDot = (nextPos - midpoint).dot(normal);
				bool nextInside = nextDot < 0.0f;

				if( inside )
				{
					newBoundary.push_back( pos );

					if( !nextInside )
					{
						// Also add intersection of this line segment and the bisector
						float t = nextDot / (-dot + nextDot);
						newBoundary.push_back( pos * t + nextPos * (1.0f - t) );
					}
				}
				else if( nextInside )
				{
					// Add intersection of this line segment and the bisector
					float t = nextDot / (-dot + nextDot);
					newBoundary.push_back( pos * t + nextPos * (1.0f - t) );
				}
			}
			boundary = newBoundary;
		}
	}
}
void CTerritoryManager::renderTerritories()
{
	if (m_DelayedRecalculate)
	{
		Recalculate();
		m_DelayedRecalculate = false;
	}

	glEnable(GL_LINE_SMOOTH);
	glLineWidth(1.5f);

	const CTerrain* pTerrain = g_Game->GetWorld()->GetTerrain();
	CFrustum frustum = g_Game->GetView()->GetCamera()->GetFrustum();
	std::vector<CTerritory*>::iterator terr=m_Territories.begin();

	for ( ; terr != m_Territories.end(); ++terr )
	{
		if ((*terr)->boundary.empty())
			continue;

		// Tweak the boundary to shift all edges "inwards" by 0.3 units towards the territory's centre,
		// so that boundaries for adjacent territories don't overlap
		std::vector<CVector2D> boundary = (*terr)->boundary;
		for ( size_t i=0; i<boundary.size(); i++ ) 
		{
			size_t prevI = (i+boundary.size()-1) % boundary.size();
			size_t nextI = (i+1) % boundary.size();

			// Figure out the direction perpendicular to each of the two edges that meet at this point.
			CVector2D dir1 = ((*terr)->boundary[i]-(*terr)->boundary[prevI]).beta().normalize();
			CVector2D dir2 = ((*terr)->boundary[nextI]-(*terr)->boundary[i]).beta().normalize();

			// If you draw a picture of what our point looks like and what the two lines 0.3 units 
			// away from it look like, and draw a line between our point and that one as well as 
			// drop perpendicular lines from it to the original edges, you get this formula for the
			// length and direction we have to be moved.
			float angle = acosf(dir1.dot(dir2));
			boundary[i] += (dir1 + dir2).normalize() * 0.3f / cosf(angle/2);
		}

		if ( (*terr)->owner->GetPlayerID() == 0 )
		{
			// Use a dark gray for Gaia territories since white looks a bit weird
			glColor3f( 0.65f, 0.65f, 0.65f );
		}
		else
		{
			// Use the player's colour
			const SPlayerColour& col = (*terr)->owner->GetColour();
			glColor3f(col.r, col.g, col.b);
		}
		
		for ( std::vector<CVector2D>::iterator it = boundary.begin(); it != boundary.end(); it++ )
		{
			std::vector<CVector2D>::iterator it2 = it + 1;
			if( it2 == boundary.end() )	// loop around if we are at the last vertex
				it2 = boundary.begin();

			CVector3D start(it->x, pTerrain->getExactGroundLevel(it->x, it->y), it->y);
			CVector3D end(it2->x, pTerrain->getExactGroundLevel(it2->x, it2->y), it2->y);

			if ( !frustum.DoesSegmentIntersect(start, end) )
				continue;
			
			glBegin(GL_LINE_STRIP);
			float iterf = (end - start).GetLength() / TERRITORY_PRECISION_STEP;
			for ( float i=0; i < iterf; i += TERRITORY_PRECISION_STEP )
			{
				CVector2D pos( Interpolate(start, end, i/iterf) );
				glVertex3f(pos.x, pTerrain->getExactGroundLevel(pos)+.25f, pos.y); 
			}
			glVertex3f(end.X, pTerrain->getExactGroundLevel(end.X, end.Z)+.25f, end.Z); 
			glEnd();
		}
	}

	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);
}