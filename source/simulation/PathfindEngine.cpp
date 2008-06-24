#include "precompiled.h"

#include "ps/Profile.h"

#include "EntityOrders.h"
#include "Entity.h"
#include "EntityTemplate.h"
#include "PathfindEngine.h"
#include "graphics/Terrain.h"
#include "ps/World.h"


#include "ps/GameSetup/Config.h"




#define EPSILON 0.00001f


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


CPathfindEngine::CPathfindEngine() : triangulationOverlay(0),
									OABBBOUNDREDUCTION(0.8f),
									CIRCLEBOUNDREDUCTION(0.5f),
									RADIUSINCREMENT(2.0f)
{
	dcdtInitialized = false;
	
	if (g_ShowPathfindingOverlay)
		triangulationOverlay = new TriangulationTerrainOverlay();
}

CPathfindEngine::~CPathfindEngine()
{
	if (triangulationOverlay)
	{
		delete triangulationOverlay;
		triangulationOverlay = 0;
	}
}

//Todo:
// 1; the bouncing problem with the fortress
// 2; update obstacles when things vanishes. done

void CPathfindEngine::initBoundary()
{
	SrPolygon boundary ;
	
	
	CTerrain* m_Terrain = g_Game->GetWorld()->GetTerrain();
	int width = m_Terrain->GetVerticesPerSide() * CELL_SIZE 	;

	boundary.push().set(0.0f, 0.0f);
	boundary.push().set(width, 0.0f);
	boundary.push().set(width, width);
	boundary.push().set(0.0f, width);
	dcdtPathfinder.init(boundary, EPSILON,1);
	dcdtPathfinder.InitializeSectors();

}

void  CPathfindEngine::insertObstacles()
{
	
	std::vector<CEntity*> results;
	g_EntityManager.GetExtant(results);
	SrPolygon poly;
	
	
	for(size_t i =0 ; i < results.size(); i++)
	{
		poly.size(0);

		
		

		CEntity* tempHandle = results[i];
		//debug_printf("Entity position: %f %f %f\n", tempHandle->m_position.X,tempHandle->m_position.Y,tempHandle->m_position.Z);
		

		CVector2D p, q;
			CVector2D u, v;
			q.x = tempHandle->m_position.X;
			q.y = tempHandle->m_position.Z;
			float d = ((CBoundingBox*)tempHandle->m_bounds)->m_d;
			float w = ((CBoundingBox*)tempHandle->m_bounds)->m_w;

			u.x = sin( tempHandle->m_graphics_orientation.Y );
			u.y = cos( tempHandle->m_graphics_orientation.Y );
			v.x = u.y;
			v.y = -u.x;

		CBoundingObject* m_bounds = tempHandle->m_bounds;

		switch( m_bounds->m_type )
		{
			case CBoundingObject::BOUND_CIRCLE:
			{
				if(tempHandle->m_speed == 0)
				{
				
					poly.open(false);

					w = CIRCLEBOUNDREDUCTION;
					d = CIRCLEBOUNDREDUCTION;
				
					p = q + u * d + v * w;
					poly.push().set((float)(p.x), (float)(p.y));

					p = q - u * d + v * w ;
					poly.push().set((float)(p.x), (float)(p.y));

					p = q - u * d - v * w;
					poly.push().set((float)(p.x), (float)(p.y));

					p = q + u * d - v * w;
					poly.push().set((float)(p.x), (float)(p.y));

					int dcdtId = dcdtPathfinder.insert_polygon(poly);
					tempHandle->m_dcdtId = dcdtId;

				
				}
				break;

			}
			case CBoundingObject::BOUND_OABB:
			{

				
				poly.open(false);
				
				// Tighten the bound so the units will not get stuck near the buildings
				//Note: the triangulation pathfinding code will not find a path for the unit if it is pushed into the bound of a unit.
				//
				w = w * OABBBOUNDREDUCTION;
				d = d * OABBBOUNDREDUCTION;
			
				p = q + u * d + v * w;
				poly.push().set((float)(p.x), (float)(p.y));

				p = q - u * d + v * w ;
				poly.push().set((float)(p.x), (float)(p.y));

				p = q - u * d - v * w;
				poly.push().set((float)(p.x), (float)(p.y));

				p = q + u * d - v * w;
				poly.push().set((float)(p.x), (float)(p.y));

				int dcdtId = dcdtPathfinder.insert_polygon(poly);
				tempHandle->m_dcdtId = dcdtId;
				break;
			}

				

		}//end switch


	}//end for loop
	dcdtPathfinder.DeleteAbstraction();
	dcdtPathfinder.Abstract();


}

void CPathfindEngine::drawTriangulation()
{

	
	int polyNum = dcdtPathfinder.num_polygons();

	//debug_printf("Number of polygons: %d",polyNum);
	
	if(polyNum)
	{
		SrArray<SrPnt2> constrainedEdges;
		SrArray<SrPnt2> unconstrainedEdges;

		dcdtPathfinder.get_mesh_edges(&constrainedEdges, &unconstrainedEdges);

		if (triangulationOverlay)
		{
			triangulationOverlay->setConstrainedEdges(constrainedEdges);
			triangulationOverlay->setUnconstrainedEdges(unconstrainedEdges);
		}
	}

	
}

void CPathfindEngine::RequestPath( HEntity entity, const CVector2D& destination,
								  CEntityOrder::EOrderSource orderSource )
{

	
	/* TODO: Add code to generate high level path
	         For now, just the one high level waypoint to the final 
			  destination is added
	*/
	CEntityOrder waypoint;
	waypoint.m_type = CEntityOrder::ORDER_GOTO_WAYPOINT;
	waypoint.m_source = orderSource;
	waypoint.m_target_location = destination;


	//Kai: adding radius for pathfinding
	CBoundingObject* m_bounds = entity->m_bounds;

	waypoint.m_pathfinder_radius = m_bounds->m_radius + RADIUSINCREMENT;
	
	//waypoint.m_pathfinder_radius = 0.0f;
	entity->m_orderQueue.push_front( waypoint );
}


void CPathfindEngine::RequestTriangulationPath( HEntity entity, const CVector2D& destination, bool UNUSED(contact),
										  float radius, CEntityOrder::EOrderSource orderSource )
{
	PROFILE_START("Pathfinding");

	if(g_TriPathfind)
		{
		/* TODO:1. add code to verify the triangulation. done.
				2. add code to convert a path from dcdtpathfinder to world waypoints done
		*/
		if(!dcdtInitialized)
		{
			initBoundary();
			insertObstacles();
			dcdtInitialized =true;
			

			//switch on/off triangulation drawing by command line arg "-showOverlay"
			//it's guarded here to stop setting constrainedEdges and unconstrainedEdges in triangulationOverlay->
			//(efficiency issue)
			//the drawing is disable in the render() function in TerraiOverlay.cpp
			if(g_ShowPathfindingOverlay)
			{
				drawTriangulation();
			}
		}
	}

	
	//Kai: added test for terrain information in entityManager
	//mLowPathfinder.TAStarTest();
	
	CVector2D source( entity->m_position.X, entity->m_position.Z );

	
	
	bool found = mTriangulationPathfinder.FindPath(source, destination, entity,dcdtPathfinder, radius);
	
	


	//push the path onto the order process queue.
		SrPolygon CurPath;
		SrPolygon CurChannel;

		
	   if ( !found )
		{ 
			// If no path was found, then unsolvable
			// TODO: Figure out what to do in this case
	
			CurPath.size(0);
			CurChannel.size(0);
		}
	   else
		{
			CurChannel = dcdtPathfinder.GetChannelBoundary();

			CurPath = dcdtPathfinder.GetPath();
			

			//set and draw the path on the terrain
			if (triangulationOverlay)
			{
				triangulationOverlay->setCurrentPath(CurPath);
			}
			
			// Make the path take as few steps as possible by collapsing steps in the same direction together.
			std::vector<CVector2D> path;

			debug_printf("waypoints: %d  channel size %d \n ",CurPath.size(),CurChannel.size());
			
			for (int i = 0; i < CurPath.size(); i++)
   			{
				CVector2D waypoint ;

				waypoint.x = CurPath[i].x;
				waypoint.y = CurPath[i].y;

				debug_printf("waypoints: %f %f \n",waypoint.x, waypoint.y);
				
				
					path.push_back(waypoint);

				
			}

			if( path.size() > 0 )
			{
				// Push the path onto the front of our order queue in reverse order,
				// so that we run through it before continuing other orders.

				CEntityOrder node;
				node.m_source = orderSource;

				// Hack to make pathfinding slightly more precise:
				// If the radius was 0, make the final node be exactly at the destination
				// (otherwise, go to wherever the pathfinder tells us since we just want to be in range)
				CVector2D finalDest = (radius==0 ? destination : path[path.size()-1]);
				node.m_type = CEntityOrder::ORDER_PATH_END_MARKER;	// push end marker (used as a sentinel when repathing)
				node.m_target_location = finalDest;
				entity->m_orderQueue.push_front(node);
				node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;	// push final goto step
				node.m_target_location = finalDest;
				entity->m_orderQueue.push_front(node);

				for( int i = ((int) path.size()) - 2; i >= 0; i-- )
				{
					node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;	// TODO: For non-contact paths, do we want some other order type?
					node.m_target_location = path[i];
					entity->m_orderQueue.push_front(node);
				}
			}
			else {
				// Hack to make pathfinding slightly more precise:
				// If radius = 0, we have an empty path but the user still wants us to move 
				// within the same tile, so add a GOTO order anyway
				if(radius == 0)
				{
					CEntityOrder node;
					node.m_type = CEntityOrder::ORDER_PATH_END_MARKER;
					node.m_target_location = destination;
					entity->m_orderQueue.push_front(node);
					node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
					node.m_target_location = destination;
					entity->m_orderQueue.push_front(node);
				}
			}
	
	   }
	
	PROFILE_END("Pathfinding");
}

void CPathfindEngine::RequestLowLevelPath( HEntity entity, const CVector2D& destination, bool UNUSED(contact),
										  float radius, CEntityOrder::EOrderSource orderSource )
{
	PROFILE_START("Pathfinding");

	
	//Kai: added test for terrain information in entityManager
	//mLowPathfinder.TAStarTest();
	
	CVector2D source( entity->m_position.X, entity->m_position.Z );

	if ( mLowPathfinder.FindPath(source, destination, entity, radius) )
	{
		std::vector<CVector2D> stepwisePath = mLowPathfinder.GetLastPath();

		// Make the path take as few steps as possible by collapsing steps in the same direction together.
		std::vector<CVector2D> path;
		CVector2D lastDir(0, 0);
		for(size_t i=0; i < stepwisePath.size(); i++)
		{
			if(i >= 2 && stepwisePath[i]-stepwisePath[i-1] == lastDir)
				// We're in a colinear range; just update last point
				path[path.size()-1] = stepwisePath[i];
			else
				path.push_back(stepwisePath[i]);

			if(i >= 1)
				lastDir = stepwisePath[i] - stepwisePath[i-1];
		}

		if( path.size() > 0 )
		{
			// Push the path onto the front of our order queue in reverse order,
			// so that we run through it before continuing other orders.

			CEntityOrder node;
			node.m_source = orderSource;

			// Hack to make pathfinding slightly more precise:
			// If the radius was 0, make the final node be exactly at the destination
			// (otherwise, go to wherever the pathfinder tells us since we just want to be in range)
			CVector2D finalDest = (radius==0 ? destination : path[path.size()-1]);
			node.m_type = CEntityOrder::ORDER_PATH_END_MARKER;	// push end marker (used as a sentinel when repathing)
			node.m_target_location = finalDest;
			entity->m_orderQueue.push_front(node);
			node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;	// push final goto step
			node.m_target_location = finalDest;
			entity->m_orderQueue.push_front(node);

			for( int i = ((int) path.size()) - 2; i >= 0; i-- )
			{
				node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;	// TODO: For non-contact paths, do we want some other order type?
				node.m_target_location = path[i];
				entity->m_orderQueue.push_front(node);
			}
		}
		else {
			// Hack to make pathfinding slightly more precise:
			// If radius = 0, we have an empty path but the user still wants us to move 
			// within the same tile, so add a GOTO order anyway
			if(radius == 0)
			{
				CEntityOrder node;
				node.m_type = CEntityOrder::ORDER_PATH_END_MARKER;
				node.m_target_location = destination;
				entity->m_orderQueue.push_front(node);
				node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
				node.m_target_location = destination;
				entity->m_orderQueue.push_front(node);
			}
		}
	}
	else
	{
		// If no path was found, then unsolvable
		// TODO: Figure out what to do in this case
	}
	
	PROFILE_END("Pathfinding");
}

void CPathfindEngine::RequestContactPath( HEntity entity, CEntityOrder* current, float range )
{
	/* TODO: Same as non-contact: need high-level planner */
	CEntityOrder waypoint;
	waypoint.m_type = CEntityOrder::ORDER_GOTO_WAYPOINT_CONTACT;
	waypoint.m_source = current->m_source;
	HEntity target = current->m_target_entity;
	waypoint.m_target_location = target->m_position;
	waypoint.m_pathfinder_radius = std::max( target->m_bounds->m_radius, range );
	entity->m_orderQueue.push_front( waypoint );

	//PathSparse( entity, current->m_target_entity->m_position );
	//// For attack orders, do some additional postprocessing (replace goto/nopathing 
	//// with attack/nopathing, up until the attack order marker)
	//std::deque<CEntityOrder>::iterator it;
	//for( it = entity->m_orderQueue.begin(); it != entity->m_orderQueue.end(); it++ )
	//{
	//	if( it->m_type == CEntityOrder::ORDER_PATH_END_MARKER )
	//		break;
	//	if( it->m_type == CEntityOrder::ORDER_GOTO_NOPATHING )
	//	{
	//		*it = *current;
	//	}
	//}
}

bool CPathfindEngine::RequestAvoidPath( HEntity entity, CEntityOrder* current, float avoidRange )
{
	/* TODO: Same as non-contact: need high-level planner */

	// TODO: Replace this with a new type of goal which is to avoid some point or line segment
	// (requires changes to pathfinder to support this type of goal)

	CEntityOrder waypoint;
	waypoint.m_type = CEntityOrder::ORDER_GOTO_WAYPOINT_CONTACT;
	waypoint.m_source = current->m_source;

	// Figure out a direction to move
	HEntity target = current->m_target_entity;
	CVector3D dir = entity->m_position - target->m_position;
	if(dir.LengthSquared() == 0) // shouldn't happen, but just in case
		dir = CVector3D(1, 0, 0);
	float dist = dir.Length();
	dir.Normalize();

	waypoint.m_target_location = entity->m_position + dir * (avoidRange - dist);

	if( !g_Game->GetWorld()->GetTerrain()->IsOnMap( waypoint.m_target_location ) )
	{
		return false;
	}

	waypoint.m_pathfinder_radius = 0.0f;
	entity->m_orderQueue.push_front( waypoint );
	return true;
}
