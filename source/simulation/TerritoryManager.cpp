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

using namespace std;

CTerritoryManager::CTerritoryManager()
{
	m_TerritoryMatrix = 0;
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
		if( entities[i]->m_base->m_isTerritoryCentre ) 
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
	const CTerrain* pTerrain = g_Game->GetWorld()->GetTerrain();
	CFrustum frustum = g_Game->GetView()->GetCamera()->GetFrustum();
	std::vector<CTerritory*>::iterator terr=m_Territories.begin();
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(1.4f);

	for ( ; terr != m_Territories.end(); ++terr )
	{
		std::vector<CVector2D>::iterator it=(*terr)->boundary.begin()+1;
		const SPlayerColour& col = (*terr)->owner->GetColour();
		glColor3f(col.r, col.g, col.b);
		glBegin(GL_LINE_STRIP);
	
		for ( ; it != (*terr)->boundary.end(); ++it )
		{
			CVector3D front(it->x, pTerrain->getExactGroundLevel(it->x, it->y), it->y);
			CVector3D prev((it-1)->x, pTerrain->getExactGroundLevel((it-1)->x, (it-1)->y), (it-1)->y);
			if ( !frustum.DoesSegmentIntersect(prev, front) )
				continue;
			
			float iterf = (front - prev).GetLength() / TERRITORY_PRECISION_STEP;
			for ( float i=0; i<iterf; i+= TERRITORY_PRECISION_STEP )
			{
				CVector2D pos( Interpolate(prev, front, i/iterf) );
				glVertex3f(pos.x, pTerrain->getExactGroundLevel(pos)+.25f, pos.y); 
			}
			glVertex3f(front.X, pTerrain->getExactGroundLevel(front.X, front.Z)+.25f, front.Z); 
		}
		//Loop around
		CVector2D first2D((*terr)->boundary.front()), back2D((*terr)->boundary.back());
		CVector3D first(first2D.x, pTerrain->getExactGroundLevel(first2D), first2D.y);
		CVector3D back(back2D.x, pTerrain->getExactGroundLevel(back2D), back2D.y);

		if ( !frustum.DoesSegmentIntersect(back, first) )
		{
			glEnd();
			continue;
		}
		float iterf = (first - back).GetLength() / TERRITORY_PRECISION_STEP;
		for ( float i=0; i<iterf; i+= TERRITORY_PRECISION_STEP )
		{
			CVector2D pos( Interpolate(back, first, i/iterf) );
			glVertex3f(pos.x, pTerrain->getExactGroundLevel(pos)+.25f, pos.y); 
		}
		glVertex3f(first.X, pTerrain->getExactGroundLevel(first2D)+.25f, first.Z); 
	
		glEnd();
	}
	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);	
}