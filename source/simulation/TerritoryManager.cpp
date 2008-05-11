#include "precompiled.h"

#include "TerritoryManager.h"

#include "graphics/Frustum.h"
#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "graphics/Model.h"
#include "graphics/Terrain.h"
#include "graphics/Unit.h"
#include "lib/allocators/allocators.h"	// matrix_alloc
#include "lib/ogl.h"
#include "lib/timer.h"
#include "maths/Bound.h"
#include "maths/MathUtil.h"
#include "ps/Game.h"
#include "ps/Player.h"
#include "ps/Profile.h"
#include "simulation/Entity.h"
#include "simulation/EntityManager.h"
#include "simulation/EntityManager.h"
#include "simulation/EntityTemplate.h"
#include "simulation/LOSManager.h"

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

	const size_t mapSize = m_TilesPerSide * CELL_SIZE;

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

		for( size_t x=0; x<m_TilesPerSide; x++ )
		{
			for( size_t z=0; z<m_TilesPerSide; z++ )
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

		for( size_t x=0; x<m_TilesPerSide; x++ )
		{
			for( size_t z=0; z<m_TilesPerSide; z++ )
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
	debug_assert( (size_t) x < m_TilesPerSide && (size_t) z < m_TilesPerSide );
	return m_TerritoryMatrix[x][z];
}

CTerritory* CTerritoryManager::GetTerritory(float x, float z)
{
	ssize_t ix, iz;
	CTerrain::CalcFromPosition(x, z, ix, iz);
	return GetTerritory(ix, iz);
}

// Calculate the boundary points of a given territory into the given vector
void CTerritoryManager::CalculateBoundary( std::vector<CEntity*>& centres, size_t myIndex, std::vector<CVector2D>& boundary )
{
	// Start with a boundary equal to the whole map
	const size_t mapSize = m_TilesPerSide * CELL_SIZE;
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
				float dot = (pos - midpoint).Dot(normal);
				bool inside = dot < 0.0f;
					
				size_t nextJ = (j+1) % boundary.size();		// index of next point
				CVector2D& nextPos = boundary[nextJ];
				float nextDot = (nextPos - midpoint).Dot(normal);
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
void CTerritoryManager::RenderTerritories()
{
	PROFILE( "render territories" );

	if (m_DelayedRecalculate)
	{
		Recalculate();
		m_DelayedRecalculate = false;
	}

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(1.5f);

	CLOSManager* losMgr = g_Game->GetWorld()->GetLOSManager();
	CFrustum frustum = g_Game->GetView()->GetCamera()->GetFrustum();
	std::vector<CTerritory*>::iterator terr=m_Territories.begin();

	for ( ; terr != m_Territories.end(); ++terr )
	{
		float r, g, b;

		if ( (*terr)->owner->GetPlayerID() == 0 )
		{
			// Use a dark gray for Gaia territories since white looks a bit weird
			//glColor3f( 0.65f, 0.65f, 0.65f );
			r = g = b = 0.65f;
		}
		else
		{
			// Use the player's colour
			const SPlayerColour& col = (*terr)->owner->GetColour();
			//glColor3f(col.r, col.g, col.b);
			r = col.r;
			g = col.g;
			b = col.b;
		}
		
		for ( size_t edge=0; edge < (*terr)->boundary.size(); edge++ )
		{
			const std::vector<CVector3D>& coords = (*terr)->GetEdgeCoords(edge);
			CVector3D start = coords[0];
			CVector3D end = coords[coords.size() - 1];

			if ( !frustum.DoesSegmentIntersect(start, end) )
				continue;
			
			glBegin( GL_LINE_STRIP );

			for( size_t i=0; i<coords.size(); i++ )
			{
				float losScale = 0.0f;
				ELOSStatus los = losMgr->GetStatus(coords[i].X, coords[i].Z, g_Game->GetLocalPlayer());
				if( los & LOS_VISIBLE ) losScale = 1.0f;
				else if( los & LOS_EXPLORED ) losScale = 0.7f;
				glColor3f( r*losScale, g*losScale, b*losScale );
				glVertex3f( coords[i].X, coords[i].Y, coords[i].Z );
			}

			glEnd();
		}
	}

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);
	glColor4f(1,1,1,1);
}

const std::vector<CVector3D>& CTerritory::GetEdgeCoords(size_t edge)
{
	if ( edgeCoords.size() == 0 )
	{
		// Edge coords have not been calculated - calculate them now
		edgeCoords.resize( boundary.size() );

		const CTerrain* pTerrain = g_Game->GetWorld()->GetTerrain();

		// Tweak the boundary to shift all edges "inwards" by 0.3 units towards the territory's centre,
		// so that boundaries for adjacent territories don't overlap
		std::vector<CVector2D> tweakedBoundary = boundary;
		for ( size_t i=0; i<boundary.size(); i++ ) 
		{
			size_t prevI = (i+boundary.size()-1) % boundary.size();
			size_t nextI = (i+1) % boundary.size();

			// Figure out the direction perpendicular to each of the two edges that meet at this point.
			CVector2D dir1 = (boundary[i]-boundary[prevI]).beta().Normalize();
			CVector2D dir2 = (boundary[nextI]-boundary[i]).beta().Normalize();

			// If you draw a picture of what our point looks like and what the two lines 0.3 units 
			// away from it look like, and draw a line between our point and that one as well as 
			// drop perpendicular lines from it to the original edges, you get this formula for the
			// length and direction we have to be moved.
			float angle = acosf(dir1.Dot(dir2));
			tweakedBoundary[i] += (dir1 + dir2).Normalize() * 0.3f / cosf(angle/2);
		}

		// Calculate the heights at points TERRITORY_PRECISION_STEP apart on our edges
		// and store the final vertices in edgeCoords.
		for ( size_t e=0; e<boundary.size(); e++ )
		{
			std::vector<CVector3D>& coords = edgeCoords[e];

			CVector2D start = tweakedBoundary[e];
			CVector2D end = tweakedBoundary[(e+1) % boundary.size()];

			float iterf = (end - start).Length() / TERRITORY_PRECISION_STEP;
			for ( float i=0; i < iterf; i += TERRITORY_PRECISION_STEP )
			{
				CVector2D pos = Interpolate( start, end, i/iterf );
				coords.push_back( CVector3D( pos.x, pTerrain->GetExactGroundLevel(pos)+0.25f, pos.y ) );
			}

			coords.push_back( CVector3D( end.x, pTerrain->GetExactGroundLevel(end)+0.25f, end.y ) );
		}
	}
	
	return edgeCoords[edge];
}

void CTerritory::ClearEdgeCache()
{
	edgeCoords.clear();
	edgeCoords.resize( boundary.size() );
}
