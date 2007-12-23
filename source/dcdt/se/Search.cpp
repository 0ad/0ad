//Search.cpp

//DJD: Abstract space searching function definitions {
#include "precompiled.h"
#include "0ad_warning_disable.h"

#include "se_dcdt.h"

#include <math.h>
#include <queue>

//function for comparing SearchNodes
struct check : public std::binary_function <SearchNode *, SearchNode *, bool> 
{
      bool operator()(SearchNode *&sn1, SearchNode *&sn2) const
	  {
		  return ((sn1->f() > sn2->f()) || ((sn1->f() == sn2->f()) && (sn1->h() > sn2->h())));;
	  }
};

//gets the closest point on an unconstrained edge of the triangle to the point given
//(that is at least r from a vertex) . . . this function can be sped up
SrPnt2 SeDcdt::ClosestPointTo(SeDcdtFace *face, float x, float y, float r)
{
	//if the point is in the triangle
	if (InTriangle(face, x, y))
	{
		//the closest point is the point itself
		SrPnt2 p;
		p.set(x, y);
		return p;
	}
	SrPnt2 closestPoint, point;
	float closestDistance = INFINITY, distance;
	//get the edges of the triangle and go through them
	SeBase *s = face->se();
	for (int i = 0; i < 3; i++)
	{
		//gets the vertices of the triangle
		float x1, y1, x2, y2, x3, y3;
		TriangleVertices(s, x1, y1, x2, y2, x3, y3);
		//doesn't consider blocked edges
		if (Blocked(s))
		{
			s = s->nxt();
			continue;
		}
		//checks if the angles along this edge and to the point are accute
		bool accute1 = IsAccute(AngleBetween(x1, y1, x2, y2, x, y));
		bool accute2 = IsAccute(AngleBetween(x2, y2, x1, y1, x, y));
		if (accute1 && accute2)
		{
			//if both are, get closest point on this line
			distance = PointLineDistance(x, y, x1, y1, x2, y2);
			float theta = atan2(y2 - y1, x2 - x1);
			theta += (Orientation(x, y, x1, y1, x2, y2) > 0.0f) ? PI / -2.0f : PI / 2.0f;
			theta += (theta <= -PI) ? 2.0f * PI : (theta > PI) ? -2.0f * PI : 0.0f;
			point.set(x + cos(theta) * distance, y + sin(theta) * distance);
			//if it's < r from either end, move it out to distance r
			if (Length(point.x, point.y, x1, y1) < r)
			{
				accute2 = false;
			}
			else if (Length(point.x, point.y, x2, y2) < r)
			{
				accute1 = false;
			}
		}
		if (!accute1 && accute2)
		{
			//get point distance r from (x2, y2) in the direction of (x1, y1)
			float theta = atan2(y1 - y2, x1 - x2);
			point.set(x2 + cos(theta) * r, y2 + sin(theta) * r);
		}
		else if (accute1 && !accute2)
		{
			//get point distance r from (x1, y1) in the direction of (x2, y2)
			float theta = atan2(y2 - y1, x2 - x1);
			point.set(x1 + cos(theta) * r, y1 + sin(theta) * r);
		}
		//measure the distance between this point and the one given
		distance = Length(point.x, point.y, x, y);
		//if this point is closer than others,
		if (distance < closestDistance)
		{
			//set it as the closest point
			closestPoint.set(point.x, point.y);
			closestDistance = distance;
		}
		//move to the next edge
		s = s->nxt();
	}
	//return the closest point found
	return closestPoint;
}

//walks from the source triangle to the destination triangle
//provided the destination triangle is listed as adjacent in the source triangle
//direction is to remove ambiguity when both endpoints of a corridor are the same
bool SeDcdt::WalkBetween(SrArray<SeBase *>& path, SeDcdtFace *sourceFace, SeDcdtFace *destinationFace, float r, int direction)
{
	SeDcdtFace *last = NULL;
	//go through all faces between the source and destination faces
	while (true)
	{
		//if we have reached the destination face,
		if (sourceFace == destinationFace)
		{
			//add it to the path and exit successfully
			path.push() = destinationFace->se();
			return true;
		}
		//get the elements of the current triangle
		SeBase *s = sourceFace->se();
		//go through the edges of the triangle
		for (int i = 0; i < 3; i++)
		{
			//if the destination face is across the current edge (or the direction tells us which way to start),
			if ((i == direction) || ((direction == INVALID)
				&& (sourceFace->link->Adjacent(i) == destinationFace) && (s->sym()->fac() != last)))
			{
				last = sourceFace;
				direction = INVALID;
				//make sure the path is wide enough
				if (sourceFace->link->Choke(i) < 2.0f * r)
				{
					//if not, empty the path and return failure
					path.size(0);
					return false;
				}
				else
				{
					//if so, add the current face to the path
					path.push() = sourceFace->se();
					//and move to the next one
					sourceFace = (SeDcdtFace *)s->sym()->fac();
					break;
				}
			}
			//move to the next edge in the current triangle
			s = s->nxt();
		}
	}
}

//checks if a unit of radius r can go through the second triangle between the first and the third
bool SeDcdt::CanCross(SeDcdtFace *middle, SeBase *end1, SeBase *end2, float r)
{
	SeBase *s = middle->se();
	//and goes through its edges
	for (int i = 0; i < 3; i++)
	{
		//if it is neither the first nor the third
		if ((s->sym()->fac() != end1->fac()) && (s->sym()->fac() != end2->fac()))
		{
			//return if the width between the first and the third is large enough for radius r
			return (middle->link->Width((i + 1) % 3) >= 2.0f * r);
		}
		//move to the next edge
		s = s->nxt();
	}
	sr_out.warning("ERROR: could not find desired face (should never happen)\n");
	return false;
}

//checks if a unit of radius r can go through the second triangle between the first and the third
bool SeDcdt::CanCross(SeBase *first, SeBase *second, SeBase *third, float r)
{
	//and goes through its edges
	for (int i = 0; i < 3; i++)
	{
		//gets the face across the current edge
		SeFace *current = second->sym()->fac();
		//if it is neither the first nor the third
		if ((current != first->fac()) && (current != third->fac()))
		{
			//return if the width between the first and the third is large enough for radius r
			return (((SeDcdtFace *)second)->link->Width((i + 1) % 3) >= 2.0f * r);
		}
		//move to the next edge
		second = second->nxt();
	}
	sr_out.warning("ERROR: could not find desired face (should never happen)\n");
	return false;
}

//finds a path in the degree-1 tree with both the start and goal in it
#if defined EXPERIMENT
bool SeDcdt::Degree1Path(SrArray<SeBase *>& path, float x1, float y1, SeDcdtFace *startFace,
						 float x2, float y2, SeDcdtFace *goalFace, float r, int &nodes)
#else
bool SeDcdt::Degree1Path(SrArray<SeBase *>& path, float x1, float y1, SeDcdtFace *startFace,
						 float x2, float y2, SeDcdtFace *goalFace, float r)
#endif
{
	//starts marking the mesh
	_mesh->begin_marking();
	//create a priority queue for searching the tree
	std::priority_queue<SearchNode *, std::vector<SearchNode *>, check> pq;
	float x, y;
	//each face is represented by its midpoint (since only one path must exist)
	TriangleMidpoint(startFace, x, y);
	SrPnt2 p;
	p.set(x, y);
	//creates the first search node
	SearchNode *current = new SearchNode(Length(x, y, x1, y1), Length(x, y, x2, y2), p, startFace, NULL);
	//put it in the queue to be searched
	pq.push(current);
	//continues until the tree has been exhausted or a path has been found
	while (!pq.empty())
	{
#if defined EXPERIMENT
		nodes++;
#endif
		//gets the first node to search
		current = pq.top();
		pq.pop();
		//checks if that node is the goal
		if (InTriangle(current->Triangle(), x2, y2))
		{
			while (!pq.empty())
			{
				pq.top()->Close();
				delete pq.top();
				pq.pop();
			}
			SearchNode *temp = current;
			//if so, visits all the triangles between the start and the goal
			while (current != NULL)
			{
				//and adds each to the path
				path.push() = current->Triangle()->se();
				//moves to the previous triangle
				current = current->Back();
			}
			temp->Close();
			delete temp;
			//since they were visited in reverse, flip the path around
			path.revert();
			path.pop();
			//stops marking the mesh
			_mesh->end_marking();
			//makes sure the channel is wide enough for the unit
			if (ValidPath(path, x1, y1, x2, y2, r))
			{
				return true;
			}
			else
			{
				//if not, empties the paths and returns failure
				path.size(0);
				return false;
			}
		}
		//gets the triangle associated with the current node
		SeDcdtFace *currentFace = current->Triangle();
		//marks it as visited
		_mesh->mark(currentFace);
		//get the elements of the triangle
		SeBase *s = currentFace->se();
		//go through the edges
		for (int i = 0; i < 3; i++)
		{
			//gets the face on the opposite side of this edge
			SeDcdtFace *tempFace = (SeDcdtFace *)s->sym()->fac();
			//only consider it if it's degree-1 and hasn't been visited before
			if ((!_mesh->marked(tempFace)) && (tempFace->link->Degree() == 1) && (!Blocked(s)))
			{
					//get the new triangle's midpoint
					TriangleMidpoint(tempFace, x, y);
					p.set(x, y);
					//create a search node corresponding to this triangle
					SearchNode *temp = new SearchNode(current->g() + Length(current->Point().x, current->Point().y, x, y),
						Length(x, y, x2, y2), p, tempFace, current);
					//and add it to the search queue
					pq.push(temp);
					current->OpenChild();
			}
			//move to the next edge
			s = s->nxt();
		}
		//if no children were created, close and delete this node
		if (current->OpenChildren() <= 0)
		{
			current->Close();
			delete current;
		}
	}
	//stop marking the mesh
	_mesh->end_marking();
	//the tree was exhausted and a path not found - return failure
	return false;
}

//follows a degree-2 ring from one node to another, making sure that each is wide enough for the given radius
bool SeDcdt::Degree2Path(SrArray<SeBase *>& path, SeDcdtFace *startFace, SeDcdtFace *nextFace,
						 SeDcdtFace *goalFace, float r)
{
	//empties the path first
	path.size(0);
	//and adds the first face to it
	path.push() = startFace->se();
	//goes until it has reached the goal triangle
	while (nextFace != goalFace)
	{
		//gets the elements of the current triangle
		SeBase *s = nextFace->se();
		//goes through its edges
		for (int i = 0; i < 3; i++)
		{
			//when the face across the current edge is the last triangle,
			if (s->sym()->fac() == startFace)
			{
				//check if the triangle across the next edge is the next triangle in the ring
				if (nextFace->link->Angle((i + 1) % 3) == INFINITY)
				{
					//makes sure this triangle is wide enough for radius r
					if (nextFace->link->Width(i) < 2.0f * r)
					{
						//if not, empties the path and returns failure
						path.size(0);
						return false;
					}
					//otherwise, adds the current triangle to the path
					path.push() = nextFace->se();
					//and moves to the next triangle in the ring
					startFace = nextFace;
					nextFace = (SeDcdtFace *)s->nxt()->sym()->fac();
				}
				else
				{
					//similar to above, checks the width
					if (nextFace->link->Width((i + 2) % 3) < 2.0f * r)
					{
						path.size(0);
						return false;
					}
					//and continues along the ring
					path.push() = nextFace->se();
					startFace = nextFace;
					nextFace = (SeDcdtFace *)s->nxt()->nxt()->sym()->fac();
				}
				//process the next triangle in the ring
				break;
			}
			//move to the next edge
			s = s->nxt();
		}
	}
	//return that a path was found successfully
	return true;
}

//follows a degree-2 ring from one node to another, making sure that each is wide enough for the given radius
bool SeDcdt::FollowLoop(SrArray<SeBase *>& path, SeDcdtFace *startFace, SeDcdtFace *nextFace,
						 SeDcdtFace *goalFace, SeDcdtFace *degree3, float r)
{
	//empties the path first
	path.size(0);
	//and adds the first face to it
	path.push() = startFace->se();
	//goes until it has reached the goal triangle
	while (nextFace != goalFace)
	{
		//gets the elements of the current triangle
		SeBase *s = nextFace->se();
		//goes through its edges
		for (int i = 0; i < 3; i++)
		{
			//when the face across the current edge is the last triangle,
			if (s->sym()->fac() == startFace)
			{
				//check if the triangle across the next edge is the next triangle in the ring
				if (nextFace->link->Adjacent((i + 1) % 3) == degree3)
				{
					//makes sure this triangle is wide enough for radius r
					if (nextFace->link->Width(i) < 2.0f * r)
					{
						//if not, empties the path and returns failure
						path.size(0);
						return false;
					}
					//otherwise, adds the current triangle to the path
					path.push() = nextFace->se();
					//and moves to the next triangle in the ring
					startFace = nextFace;
					nextFace = (SeDcdtFace *)s->nxt()->sym()->fac();
				}
				else
				{
					//similar to above, checks the width
					if (nextFace->link->Width((i + 2) % 3) < 2.0f * r)
					{
						path.size(0);
						return false;
					}
					//and continues along the ring
					path.push() = nextFace->se();
					startFace = nextFace;
					nextFace = (SeDcdtFace *)s->nxt()->nxt()->sym()->fac();
				}
				//process the next triangle in the ring
				break;
			}
			//move to the next edge
			s = s->nxt();
		}
	}
	//return that a path was found successfully
	return true;
}

//determines if the given endpoint has to be checked against the current triangle
bool SeDcdt::ValidEndpoint(SeBase *s, float x, float y, bool left)
{
	//gets the vertices of the current triangle
	float x1, y1, x2, y2, x3, y3, x4, y4;
	TriangleVertices(s, x1, y1, x2, y2, x3, y3);
	//checks if a base angle is obtuse
	if ((abs(AngleBetween(x1, y1, x2, y2, x3, y3)) >= PI / 2.0f)
		|| (abs(AngleBetween(x1, y1, x3, y3, x2, y2)) >= PI / 2.0f))
	{
		//if so, we don't have to worry
		return true;
	}
	//gets the angle of the opposite edge
	float theta = atan2(y3 - y2, x3 - x2);
	//turn 90 degrees clockwise
	theta += (theta <= PI / -2.0f) ? 3.0f * PI / 2.0f : PI / -2.0f;
	//create a point this angle from the "top" vertex
    x4 = x1 + cos(theta);
	y4 = y1 + sin(theta);
	//returns whether the unit doesn't have to pass through the narrowest point in this triangle
	return ((Orientation(x1, y1, x4, y4, x, y) > 0) == (left));
}

//checks whether there is a valid path from the start or goal through their corresponding triangles
bool SeDcdt::ValidEndpoint(SeDcdtFace *first, SeDcdtFace *second, float x, float y, float r)
{
	//goes through the edges of the first triangle
	SeBase *s = first->se();
	for (int i = 0; i < 3; i++)
	{
		//when it finds the one between the first and second triangles,
		if (s->sym()->fac() == second)
		{
			//if it has to cross the line extending from either end vertex, checks their width
			return (((ValidEndpoint(s, x, y, false)) || (first->link->Width((i + 2) % 3) >= 2.0f * r))
				&& ((ValidEndpoint(s->nxt(), x, y, true)) || (first->link->Width(i) >= 2.0f * r)));
		}
		//moves to the next edge
		s = s->nxt();
	}
	//if the second triangle was never found, return failure
	return false;
}

//checks if the start and goal points can validly enter the path
bool SeDcdt::ValidEndpoints(SrArray<SeBase *>& path, float x1, float y1, float x2, float y2, float r)
{
	//if the path is size 1, no problem, otherwise checks that it can go from the first to second triangle,
	//and from the last to second-last triangle
	return ((path.size() < 2) || ((ValidEndpoint((SeDcdtFace *)path[0]->fac(), (SeDcdtFace *)path[1]->fac(), x1, y1, r))
		&& (ValidEndpoint((SeDcdtFace *)path[path.size() - 1]->fac(), (SeDcdtFace *)path[path.size() - 2]->fac(), x2, y2, r))));
}

//checks if a path is valid for a unit of radius r
bool SeDcdt::ValidPath(SrArray<SeBase *>& path, float x1, float y1, float x2, float y2, float r)
{
	//first checks if the endpoints are valid
	if (!ValidEndpoints(path, x1, y1, x2, y2, r))
	{
		return false;
	}
	//then goes through the interior points
	for (int i = 1; i < path.size() - 1; i++)
	{
		//gets the elements of the current triangle in the path
		SeBase *s = path[i]->fac()->se();
		//goes through the edges in this triangle
		for (int j = 0; j < 3; j++)
		{
			//find the edge across which is the next triangle in the path
			if (s->sym()->fac() == path[i + 1]->fac())
			{
				//get the width through this triangle between the last one in the path and the next one
				float width = ((SeDcdtFace *)s->fac())->link->Width(
					(s->nxt()->sym()->fac() == path[i - 1]->fac()) ? j : ((j + 2) % 3));
				//if it's less than the diameter of the unit, the path is invalid
				if (width < 2.0f * r)
				{
					return false;
				}
				break;
			}
			//moves to the next edge in the triangle
			s = s->nxt();
		}
	}
	//if no triangle in the path was invalid, the path is valid
	return true;
}

//determines the triangles that make up the path between the start and goal,
//using the degree-3 nodes speficied in the goal node and it's back pointer chain
//direction is to remove ambiguity when a corridor's endpoints are the same
void SeDcdt::ConstructBasePath(SrArray<SeBase *> &path, SearchNode *goalNode, SeDcdtFace *start, SeDcdtFace *goal, int direction)
{
	//construct the backwards portion of the path first
	SrArray<SeBase *> backwardsPath;
	SeDcdtFace *goal2;
	//if the goal triangle is degree-1,
	if (goal->link->Degree() == 1)
	{
		//go through its edges and find the one across which is the root of the tree
		for (int i = 0; i < 3; i++)
		{
			if (goal->link->Adjacent(i) != NULL)
			{
				//then walk to the root from here
				WalkBetween(backwardsPath, goal, goal->link->Adjacent(i), 0.0f);
				//get rid of the overlapping triangle
				backwardsPath.pop();
				//record the new position in the path
				goal2 = goal->link->Adjacent(i);
				break;
			}
		}
	}
	else
	{
		//otherwise, we start from a degree-2 or 3 node
		goal2 = goal;
	}
	//if we are at a degree-2 node,
	if (goal->link->Degree() < 3)
	{
		//walk from there to the last degree-3 node
		WalkBetween(backwardsPath, goal2, goalNode->Triangle(), 0.0f, direction);
		//get rid of the overlapping triangle
		backwardsPath.pop();
	}
	//follow the chain of back pointers to the start
	while (goalNode->Back() != NULL)
	{
		//create the path between the search node triangles
		SrArray<SeBase *> tempPath;
		WalkBetween(tempPath, goalNode->Back()->Triangle(), goalNode->Triangle(), 0.0f, goalNode->Direction());
		//insert it into the backwards path
		while (!tempPath.empty())
		{
			backwardsPath.push() = tempPath.pop();
		}
		//get rid of the overlapping triangle
		backwardsPath.pop();
		//move back in the chain
		goalNode = goalNode->Back();
	}
	SeDcdtFace *start2;
	//if the start triangle is degree-1,
	if (start->link->Degree() == 1)
	{
		//find the edge across which is the root of the tree
		for (int i = 0; i < 3; i++)
		{
			if (start->link->Adjacent(i) != NULL)
			{
				//walk from the start triangle to the root
				WalkBetween(path, start, start->link->Adjacent(i), 0.0f);
				//get rif of the overlapping triangle
				path.pop();
				//now we work from the root of the tree
				start2 = start->link->Adjacent(i);
				break;
			}
		}
	}
	else
	{
		//otherwise, the start triangle is degree-2 or 3
		start2 = start;
	}
	//if we are at a degree-2 node,
	if (start->link->Degree() < 3)
	{
		//add the path from there to the first degree-3 node
		WalkBetween(path, start2, goalNode->Triangle(), 0.0f, goalNode->Direction());
	}
	//put the path created so far onto this path
	while (!backwardsPath.empty())
	{
		path.push() = backwardsPath.pop();
	}
	//get rid of the extra triangle
	path.pop();
}

//searches the abstract graph for a path between two points
#if defined EXPERIMENT
bool SeDcdt::SearchPathFast(float x1, float y1, float x2, float y2, float r, Data *data, int numData)//, float &length, float &time, int &nodes)
#else
bool SeDcdt::SearchPathFast(float x1, float y1, float x2, float y2, float r)//, float &length, float &time, int &nodes)
#endif
{
#if defined EXPERIMENT
	//start timing the search
	Timer startTime;
	//start counting the nodes searched
	int nodes = 0;
	//also time the total time constructing channel and running the modified funnel algorithm
	float constructionTime = 0.0f;
	//count the number of triangles visited on the low level
	int triangles = 0;
	//count the total number of paths found
	int paths = 0;
	//keep track of the current data point
	int currentData = 0;
#endif
	//set the start and goal points
	startPoint.set(x1, y1);
	goalPoint.set(x2, y2);
	//initialize the path to empty
	currentPath.size(0);
	currentChannel.size(0);
	SeBase *start, *goal;
	//find the triangle containing the start point
	SeTriangulator::LocateResult startResult = LocatePoint(x1, y1, start);
	//if it wasn't found, return a path could not be found
	if (startResult == SeTriangulator::NotFound)
	{
		return false;
	}
	//find the triangle containing the goal point
	SeTriangulator::LocateResult goalResult = LocatePoint(x2, y2, goal);
	//if it couldn't, return a failure
	if (goalResult == SeTriangulator::NotFound)
	{
		return false;
	}
	SeDcdtFace *startFace = (SeDcdtFace *)start->fac();
	SeDcdtFace *goalFace = (SeDcdtFace *)goal->fac();
	//if either triangle isn't abstracted, this search fails
	//also if the triangles aren't in the same connected component, no path connecting them exists
	if ((startFace->link == NULL) || (goalFace->link == NULL)
		|| (startFace->link->Component() != goalFace->link->Component()))
	{
		return false;
	}
	//check that the start and goal points aren't within the radius of any constraints
	if ((!ValidPosition(x1, y1, startFace, r)) || (!ValidPosition(x2, y2, goalFace, r)))
	{
		//if either are, return false
		return false;
	}
	//if the start and goal are in the same triangle,
	if (startFace == goalFace)
	{
		//the path is a straight line
		currentPath.push().set(x1, y1);
		currentPath.push().set(x2, y2);
#if defined EXPERIMENT
		//stop timing
		data[0].TotalTime = startTime.GetDuration();
		//record the length of the path
		data[0].Length = Length(x1, y1, x2, y2);
#endif
		return true;
	}
	//if both start and goal faces are degree-1,
	if ((startFace->link->Degree() == 1) && (goalFace->link->Degree() == 1))
	{
		//checks if the start (and thus the goal as well) is in a tree component
		bool inTree = true;
		//(true if there are no adjacent degree-2 nodes)
		for (int i = 0; i < 3; i++)
		{
			if (startFace->link->Adjacent(i) != NULL)
			{
				inTree = false;
			}
		}
		//if they are in a tree component,
		if (inTree)
		{
            //perform search in it
			SrArray<SeBase *> channel;
#if defined EXPERIMENT
			if (Degree1Path(channel, x1, y1, startFace, x2, y2, goalFace, r, triangles))
#else
			if (Degree1Path(channel, x1, y1, startFace, x2, y2, goalFace, r))
#endif
			{
				//if the search was successful, calculate the shortest path
				currentChannel = channel;
#if defined EXPERIMENT
				//start timing the funnel algorithm
				Timer construction;
				//run the funnel algorithm
				data[0].Length = GetShortestPath(currentPath, channel, x1, y1, x2, y2, r);
				//record the construction time
				data[0].ConstructionTime = construction.GetDuration();
				//and the number of triangles visited
				data[0].ConstructionNodes = triangles;
				//and finally the total time of the algorithm
				data[0].TotalTime = startTime.GetDuration();
#else
				//run the modified funnel algorithm to get the shortest path in that channel
				GetShortestPath(currentPath, channel, x1, y1, x2, y2, r);
#endif
				//and return success
				return true;
			}
			else
			{
				//otherwise return failure
				return false;
			}
		}
	}
	//if the start face is degree-1, moves to the adjacent degree-2 node
	SeDcdtFace *startFace2;
	float startAngle;
	float startChoke;
	if (startFace->link->Degree() == 1)
	{
		//go through the edges
		for (int i = 0; i < 3; i++)
		{
			//determines which one is in the direction of the root of the tree
			SeDcdtFace *adjacent = startFace->link->Adjacent(i);
			if (adjacent != NULL)
			{
				//and records the root and the angle and choke point on the way there
				startFace2 = adjacent;
				startAngle = startFace->link->Angle(i);
				startChoke = startFace->link->Choke(i);
				break;
			}
		}
	}
	else
	{
		//if we are at a degree-2 or 3 node, don't change anything
		startFace2 = startFace;
		startAngle = 0.0f;
		startChoke = INFINITY;
	}
	//if the goal face is degree-1, moves to the adjacent degree-2 node
	SeDcdtFace *goalFace2;
	float goalAngle;
	float goalChoke;
	if (goalFace->link->Degree() == 1)
	{
		//go through the edges
		for (int i = 0; i < 3; i++)
		{
			//determines which one is in the direction of the root of the tree
			SeDcdtFace *adjacent = goalFace->link->Adjacent(i);
			if (adjacent != NULL)
			{
				//and records the root and the angle and choke point on the way there
				goalFace2 = adjacent;
				goalAngle = goalFace->link->Angle(i);
				goalChoke = goalFace->link->Choke(i);
				break;
			}
		}
	}
	else
	{
		//if we are at a degree-2 or 3 node, don't change anything
		goalFace2 = goalFace;
		goalAngle = 0.0f;
		goalChoke = INFINITY;
	}
	//if the start and goal faces are both degree-1 or 2,
	if ((startFace->link->Degree() < 3) && (goalFace->link->Degree() < 3))
	{
		//checks if the start and goal have the same root,
		if (startFace2 == goalFace2)
		{
			//if the start face is at the root of the tree that the goal face is in,
			if (startFace2 == startFace)
			{
#if defined EXPERIMENT
				//start timing the channel construction and modified funnel algorithm
				Timer construction;
#endif
				SrArray<SeBase *> channel;
				//walk from the goal face to the start face and check that the endpoints are valid
				if (WalkBetween(channel, goalFace, startFace, r)
					&& ValidEndpoints(channel, x1, y1, x2, y2, r))
				{
#if defined EXPERIMENT
					//keep track of the number of triangles visited
					triangles += channel.size();
#endif
					//invert the path and get rid of the extra triangle
					channel.revert();
					channel.pop();
#if defined EXPERIMENT
					//record the length of the path found
					data[0].Length = GetShortestPath(currentPath, channel, x1, y1, x2, y2, r);
					//keep track of the current channel
					currentChannel = channel;
					//stop timing and record the construction
					data[0].ConstructionTime = construction.GetDuration();
					//record the number of triangles visited
					data[0].ConstructionNodes = triangles;
					//record the total time of the algorithm
					data[0].TotalTime = startTime.GetDuration();
#else
					//run the modified funnel algorithm
					GetShortestPath(currentPath, channel, x1, y1, x2, y2, r);
#endif
					//and return success
					return true;
				}
				else
				{
					//if the path was not wide enough, return failure
					return false;
				}
			}
			//if the goal face is at the root of the tree that the start face is in,
			else if (goalFace2 == goalFace)
			{
#if defined EXPERIMENT
				//start timing channel construction and the modified funnel algorithm
				Timer construction;
#endif
				SrArray<SeBase *> channel;
				//walk from the start face to the goal face and check that the endpoints are valid
				if (WalkBetween(channel, startFace, goalFace, r)
					&& ValidEndpoints(channel, x1, y1, x2, y2, r))
				{
#if defined EXPERIMENT
					//keep track of the number of triangles visited
					triangles += channel.size();
#endif
					//get rid of the extra triangle
					channel.pop();
					//calculate the shortest path and return success
					currentChannel = channel;
#if defined EXPERIMENT
					//record the length of the path
					data[0].Length = GetShortestPath(currentPath, channel, x1, y1, x2, y2, r);
					//and the construction time
					data[0].ConstructionTime = construction.GetDuration();
					//and the number of triangles visited
					data[0].ConstructionNodes = triangles;
					//and finally the total time of the algorithm
					data[0].TotalTime = startTime.GetDuration();
#else
					//run the modified funnel algorithm on the channel constructed
					GetShortestPath(currentPath, channel, x1, y1, x2, y2, r);
#endif
					//and return success
					return true;
				}
				else
				{
					//if the path was not wide enough, return failure
					return false;
				}
			}
			//if the start and goal are both in the same tree component,
			else
			{
				SrArray<SeBase *> channel;
				//perform search in tree component and check that the endpoints are valid
#if defined EXPERIMENT
				if (Degree1Path(channel, x1, y1, startFace, x2, y2, goalFace, r, triangles)
#else
				if (Degree1Path(channel, x1, y1, startFace, x2, y2, goalFace, r)
#endif
					&& ValidEndpoints(channel, x1, y1, x2, y2, r))
				{
					//record the current channel
					currentChannel = channel;
#if defined EXPERIMENT
					//start timing the construction
					Timer construction;
					//if the search was successful, calculate the shortest path
					data[0].Length = GetShortestPath(currentPath, channel, x1, y1, x2, y2, r);
					//and record the time to find it
					data[0].ConstructionTime = construction.GetDuration();
					//and the triangles visited
					data[0].ConstructionNodes = triangles;
					//and the time of the whole algorithm
					data[0].TotalTime = startTime.GetDuration();
#else
					//calculate the shortest path in the channel
					GetShortestPath(currentPath, channel, x1, y1, x2, y2, r);
#endif
					//and return success
					return true;
				}
				else
				{
					//otherwise, return failure
					return false;
				}
			}
		}
	}
	//if we can't get from the start or goal to the rest of the graph, no path exists
	if ((startChoke < 2.0f * r) || (goalChoke < 2.0f * r))
	{
		return false;
	}
	//if both the start and goal are in degree-1 or 2 nodes, check if they're on a ring
	if ((startFace2->link->Degree() == 2) && (goalFace2->link->Degree() == 2))
	{
		int i;
		//go through the edges of the degree-2 node attached to the start face
		for (i = 0; i < 3; i++)
		{
			//if it's adjacent to a degree-3 node, it's not a ring
			if (startFace2->link->Adjacent(i) != NULL)
			{
				break;
			}
		}
		//if it wasn't, the start and goal are both rooted on a degree-2 ring
		if (i >= 3)
		{
			//create paths between the start and goal triangles and their respective roots,
			//and paths for the left and right sides of the ring
			SrArray<SeBase *> startPath, goalPath, leftPath, rightPath;
			//if the start triangle is the root of its tree, there is no start path
			if (startFace == startFace2)
			{
				startPath.size(0);
				startPath.push() = startFace->se();
			}
			//otherwise, walk from the start to the root of its tree
			else if (!WalkBetween(startPath, startFace, startFace2, r))
			{
				//if the path wasn't wide enough, return failure
				return false;
			}
			//if the goal triangle is the root of its tree, there is no goal path
			if (goalFace == goalFace2)
			{
				goalPath.size(0);
				goalPath.push() = goalFace->se();
			}
			//otherwise, walk from the goal to the root of its tree
			else if (!WalkBetween(goalPath, goalFace, goalFace2, r))
			{
				//if the path wasn't wide enough, return failure
				return false;
			}
			//invert the goal path since it was constructed backward
			goalPath.revert();
			//go through the edges of the root of the start triangle
			SeBase *s = startFace2->se();
			bool leftValid, rightValid;
			for (int i = 0; i < 3; i++)
			{
				//when we find the edge across which there is no degree-3 node,
				if (startFace2->link->Angle(i) == INVALID)
				{
					//constructs the right and left side paths and determines if each is wide enough
					rightValid = Degree2Path(rightPath, startFace2, (SeDcdtFace *)s->nxt()->sym()->fac(), goalFace2, r);
					leftValid = Degree2Path(leftPath, startFace2, (SeDcdtFace *)s->nxt()->nxt()->sym()->fac(), goalFace2, r);
					break;
				}
				//go to the next edge
				s = s->nxt();
			}
			//if neither left or right paths are valid, return failure
			if (!leftValid && !rightValid)
			{
				return false;
			}
			//creates two full paths
			SrArray<SeBase *> fullLeftPath, fullRightPath;
			fullLeftPath.size(0);
			fullRightPath.size(0);
			startPath.pop();
			goalPath.pop();
			//checks if the whole left path is valid, if so, constructs it
			if (leftValid && ((startPath.size() < 1) || (leftPath.size() < 1) || 
				CanCross(startFace2, startPath[startPath.size() - 1], (leftPath.size() < 2) ? goalFace2->se() : leftPath[1], r))
				&& ((goalPath.size() < 2) || (leftPath.size() < 1) ||
				CanCross(goalFace2, (goalPath.size() < 2) ? goalFace->se() : goalPath[1], leftPath[leftPath.size() - 1], r)))
			{
				//adds the start path
				for (int i = 0; i < startPath.size(); i++)
				{
					fullLeftPath.push() = startPath[i];
				}
				//then the left side of the ring
				for (int i = 0; i < leftPath.size(); i++)
				{
					fullLeftPath.push() = leftPath[i];
				}
				//and finally, the goal path
				for (int i = 0; i < goalPath.size(); i++)
				{
					fullLeftPath.push() = goalPath[i];
				}
			}
			//checks if the whole right path is valid, if so, constructs it
			if (rightValid && ((startPath.size() < 1) || (rightPath.size() < 1) || 
				CanCross(startFace2, startPath[startPath.size() - 1], (rightPath.size() < 2) ? goalFace2->se() : rightPath[1], r))
				&& ((goalPath.size() < 1) || (rightPath.size() < 1) ||
				CanCross(goalFace2, (goalPath.size() < 2) ? goalFace->se() : goalPath[1], rightPath[rightPath.size() - 1], r)))
			{
				//adds the start path
				for (int i = 0; i < startPath.size(); i++)
				{
					fullRightPath.push() = startPath[i];
				}
				//then the right side of the ring
				for (int i = 0; i < rightPath.size(); i++)
				{
					fullRightPath.push() = rightPath[i];
				}
				//and finally, the goal path
				for (int i = 0; i < goalPath.size(); i++)
				{
					fullRightPath.push() = goalPath[i];
				}
			}
			//if neither path was valid, return failure
			if ((fullLeftPath.size() == 0) && (fullRightPath.size() == 0))
			{
				return false;
			}
			//if both paths are valid,
			else if ((fullLeftPath.size() > 0) && (fullRightPath.size() > 0))
			{
#if defined EXPERIMENT
				//start timing the construction
				Timer construction;
#endif
				//calculate the lengths of both
				SrPolygon funnelLeft, funnelRight;
				float leftLength = GetShortestPath(funnelLeft, fullLeftPath, x1, y1, x2, y2, r);
				float rightLength = GetShortestPath(funnelRight, fullRightPath, x1, y1, x2, y2, r);
#if defined EXPERIMENT
				//record the construction time
				data[0].ConstructionTime = construction.GetDuration();
#endif
				//set the current path to the shorter of the two and return success
				currentPath = (leftLength < rightLength) ? funnelLeft : funnelRight;
				currentChannel = (leftLength < rightLength) ? fullLeftPath : fullRightPath;
#if defined EXPERIMENT
				//record the length of the shorter path found
				data[0].Length = (leftLength < rightLength) ? leftLength : rightLength;
				//record the triangles visited
				data[0].ConstructionNodes = startPath.size() + goalPath.size() + leftPath.size() + rightPath.size() + 2;
				//stop timing the algorithm
				data[0].TotalTime = startTime.GetDuration();
#endif
				//and return success
				return true;
			}
			//if only one was valid,
			else
			{
#if defined EXPERIMENT
				//start timing construction
				Timer construction;
				//calculate the length of valid path
				data[0].Length = GetShortestPath(currentPath, (fullRightPath.size() == 0) ? fullLeftPath : fullRightPath, x1, y1, x2, y2, r);
				//record the construction time
				data[0].ConstructionTime = construction.GetDuration();
				//total up the triangles used in construction
				data[0].ConstructionNodes = startPath.size() + goalPath.size() + leftPath.size() + rightPath.size() + 2;
				//finish timing the algorithm
				data[0].TotalTime = startTime.GetDuration();
#else
				//calculate the valid path
				GetShortestPath(currentPath, (fullRightPath.size() == 0) ? fullLeftPath : fullRightPath, x1, y1, x2, y2, r);
#endif
				//record the current channel
				currentChannel = (fullRightPath.size() == 0) ? fullLeftPath : fullRightPath;
				//and return success
				return true;
			}
		}
	}
	//the end points of the edge the start triangle is on
	SeDcdtFace *startTriangle1 = NULL, *startTriangle2 = NULL;
	//a priority queue for searching the abstract space
	std::priority_queue<SearchNode *, std::vector<SearchNode *>, check> q;
	//if the start triangle is degree-3,
	if (startFace2->link->Degree() == 3)
	{
		//enqueue a node corresponding to that triangle into the queue
		SrPnt2 p = ClosestPointTo(startFace2, x2, y2, r);
		q.push(new SearchNode(0.0f, Length(x1, y1, x2, y2), p, startFace2, NULL));
	}
	else
	{
		//otherwise, go through the edges of the degree-2 node
		SeBase *s = startFace2->se();
		for (int i = 0; i < 3; i++)
		{
			//get the adjacent triangle across that edge
			SeDcdtFace *face = startFace2->link->Adjacent(i);
			if (face == NULL)
			{
				s = s->nxt();
				continue;
			}
			//if it's not null, assign it to one of the end points
			((startTriangle1 == NULL) ? startTriangle1 : startTriangle2) = face;
			float x_1, y_1, x_2, y_2, x_3, y_3;
			SeBase *s1 = face->se();
			for (int j = 0; j < 3; j++)
			{
				if ((face->link->Adjacent(j) == startFace2->link->Adjacent((i + 1) % 3))
					|| (face->link->Adjacent(j) == startFace2->link->Adjacent((i + 2) % 3)))
				{
					TriangleVertices(s1, x_1, y_1, x_2, y_2, x_3, y_3);
					break;
				}
				s1 = s1->nxt();
			}
			//get the closest point in this triangle to the start and goal,
			SrPnt2 p = ClosestPointOn(x_1, y_1, x_2, y_2, x2, y2, r);
			SrPnt2 p2 = ClosestPointOn(x_1, y_1, x_2, y_2, x1, y1, r);
			//and the closest point to the start from the degree-2 start triangle
			SrPnt2 p3 = ClosestPointTo(startFace2, x1, y1, r);
			//gets the longer of the shortest paths through the triangles from the start to its root
			//and the distance between the start and the closest point in its root triangle
			float distance1 = Maximum(startAngle * r, Length(x1, y1, p3.x, p3.y));
			//gets the shortest path through the triangles between the root and this adjacent degree-3 node
			float distance2 = startFace2->link->Angle(i) * r;
			TriangleVertices(s, x_1, y_1, x_2, y_2, x_3, y_3);
			//gets the angle through the triangle between the adjacent degree-3 node and the tree
			float theta = (startFace2->link->Adjacent((i + 1) % 3) == NULL) ?
				abs(AngleBetween(x_1, y_1, x_2, y_2, x_3, y_3)) : abs(AngleBetween(x_2, y_2, x_1, y_1, x_3, y_3));
			//adds a search node corresponding to this adjacent degree-3 node to the priority queue
			q.push(new SearchNode(Maximum(Length(p2.x, p2.y, x1, y1), distance1 + distance2 + theta * r),
				Length(p.x, p.y, x2, y2), p, face, NULL, i));
			//moves to the next edge
			s = s->nxt();
		}
	}
	//creates an array of goal nodes
	SrArray<SearchNode> goalNodes;
	//if the goal is on a degree-3 node,
	if (goalFace2->link->Degree() == 3)
	{
		//there is only one goal node to consider
		SrPnt2 p = ClosestPointTo(goalFace2, x1, y1, r);
		goalNodes.push() = SearchNode(Length(p.x, p.y, x1, y1), 0.0f, p, goalFace2, NULL);
	}
	else
	{
		//otherwise, go through the edges of the triangle at the root of the goal node
		for (int i = 0; i < 3; i++)
		{
			//get the degree-3 node adjacent across this edge
			SeDcdtFace *face = goalFace2->link->Adjacent(i);
			if (face == NULL)
			{
				continue;
			}
			//get the closest points in the triangle to the start and the goal
			SrPnt2 p = ClosestPointTo(face, x2, y2, r);
			SrPnt2 p2 = ClosestPointTo(face, x1, y1, r);
			//enqueue a search node corresponding with this adjacent degree-3 node
			goalNodes.push(SearchNode(Length(p2.x, p2.y, x1, y1),
				Length(p.x, p.y, x2, y2), p, face, NULL, i));
		}
	}
	//start with no shortest path
	float shortestPathLength = INFINITY;
	SrPolygon shortestPath;
	SrArray<SeBase *> shortestChannel;
	//if both the start and goal are on degree-1 or 2 nodes, check if they have the same endpoints
	if ((q.size() == 2) && (goalNodes.size() == 2) &&
		(((startTriangle1 == goalNodes[0].Triangle()) && (startTriangle2 == goalNodes[1].Triangle())) ||
		((startTriangle1 == goalNodes[1].Triangle()) && (startTriangle2 == goalNodes[0].Triangle()))))
	{
		if (startTriangle1 == startTriangle2)
		{
			//create paths between the start and goal triangles and their respective roots,
			//and paths for the left and right sides of the ring
			SrArray<SeBase *> startPath, goalPath, leftPath, rightPath;
			//if the start triangle is the root of its tree, there is no start path
			if (startFace == startFace2)
			{
				startPath.size(0);
				startPath.push() = startFace->se();
			}
			//otherwise, walk from the start to the root of its tree
			else if (!WalkBetween(startPath, startFace, startFace2, r))
			{
				//if the path wasn't wide enough, return failure
				return false;
			}
			//if the goal triangle is the root of its tree, there is no goal path
			if (goalFace == goalFace2)
			{
				goalPath.size(0);
				goalPath.push() = goalFace->se();
			}
			//otherwise, walk from the goal to the root of its tree
			else if (!WalkBetween(goalPath, goalFace, goalFace2, r))
			{
				//if the path wasn't wide enough, return failure
				return false;
			}
			//invert the goal path since it was constructed backward
			goalPath.revert();
			//go through the edges of the root of the start triangle
			SeBase *s = startFace2->se();
			bool leftValid, rightValid;
			for (int i = 0; i < 3; i++)
			{
				//when we find the edge across which there is no degree-3 node,
				if (startFace2->link->Angle(i) == INVALID)
				{
					//constructs the right and left side paths and determines if each is wide enough
					rightValid = FollowLoop(rightPath, startFace2, (SeDcdtFace *)s->nxt()->sym()->fac(), goalFace2, startTriangle1, r);
					leftValid = FollowLoop(leftPath, startFace2, (SeDcdtFace *)s->nxt()->nxt()->sym()->fac(), goalFace2, startTriangle1, r);
					break;
				}
				//go to the next edge
				s = s->nxt();
			}
			//if neither left or right paths are valid, return failure
			if (!leftValid && !rightValid)
			{
				return false;
			}
			//creates two full paths
			SrArray<SeBase *> fullLeftPath, fullRightPath;
			fullLeftPath.size(0);
			fullRightPath.size(0);
			startPath.pop();
			goalPath.pop();
			//checks if the whole left path is valid, if so, constructs it
			if (leftValid && ((startPath.size() < 1) || (leftPath.size() < 1) || 
				CanCross(startFace2, startPath[startPath.size() - 1], (leftPath.size() < 2) ? goalFace2->se() : leftPath[1], r))
				&& ((goalPath.size() < 2) || (leftPath.size() < 1) ||
				CanCross(goalFace2, (goalPath.size() < 2) ? goalFace->se() : goalPath[1], leftPath[leftPath.size() - 1], r)))
			{
				//adds the start path
				for (int i = 0; i < startPath.size(); i++)
				{
					fullLeftPath.push() = startPath[i];
				}
				//then the left side of the ring
				for (int i = 0; i < leftPath.size(); i++)
				{
					fullLeftPath.push() = leftPath[i];
				}
				//and finally, the goal path
				for (int i = 0; i < goalPath.size(); i++)
				{
					fullLeftPath.push() = goalPath[i];
				}
			}
			//checks if the whole right path is valid, if so, constructs it
			if (rightValid && ((startPath.size() < 1) || (rightPath.size() < 1) || 
				CanCross(startFace2, startPath[startPath.size() - 1], (rightPath.size() < 2) ? goalFace2->se() : rightPath[1], r))
				&& ((goalPath.size() < 1) || (rightPath.size() < 1) ||
				CanCross(goalFace2, (goalPath.size() < 2) ? goalFace->se() : goalPath[1], rightPath[rightPath.size() - 1], r)))
			{
				//adds the start path
				for (int i = 0; i < startPath.size(); i++)
				{
					fullRightPath.push() = startPath[i];
				}
				//then the right side of the ring
				for (int i = 0; i < rightPath.size(); i++)
				{
					fullRightPath.push() = rightPath[i];
				}
				//and finally, the goal path
				for (int i = 0; i < goalPath.size(); i++)
				{
					fullRightPath.push() = goalPath[i];
				}
			}
			//if neither path was valid, return failure
			if ((fullLeftPath.size() == 0) && (fullRightPath.size() == 0))
			{
				return false;
			}
			//if both paths are valid,
			else if ((fullLeftPath.size() > 0) && (fullRightPath.size() > 0))
			{
#if defined EXPERIMENT
				//start timing construction
				Timer construction;
#endif
				//calculate the lengths of both
				SrPolygon funnelLeft, funnelRight;
				float leftLength = GetShortestPath(funnelLeft, fullLeftPath, x1, y1, x2, y2, r);
				float rightLength = GetShortestPath(funnelRight, fullRightPath, x1, y1, x2, y2, r);
#if defined EXPERIMENT
				//record the construction time
				data[0].ConstructionTime = construction.GetDuration();
#endif
				//set the current path and channel to the shorter of the two
				currentPath = (leftLength < rightLength) ? funnelLeft : funnelRight;
				currentChannel = (leftLength < rightLength) ? fullLeftPath : fullRightPath;
#if defined EXPERIMENT
				//record the length of the chosen path
				data[0].Length = (leftLength < rightLength) ? leftLength : rightLength;
				//and the triangles visited
				data[0].ConstructionNodes = startPath.size() + goalPath.size() + leftPath.size() + rightPath.size() + 2;
				//and finish timing the algorithm
				data[0].TotalTime = startTime.GetDuration();
#endif
				//and return success
				return true;
			}
			else
			{
#if defined EXPERIMENT
				//start timing construction
				Timer construction;
				//if only one was valid, calculate the valid path
				data[0].Length = GetShortestPath(currentPath, (fullRightPath.size() == 0) ? fullLeftPath : fullRightPath, x1, y1, x2, y2, r);\
				//record the construction time
				data[0].ConstructionTime = construction.GetDuration();
				//and the triangles visited
				data[0].ConstructionNodes = startPath.size() + goalPath.size() + leftPath.size() + rightPath.size() + 2;
				//and stop timing the algorithm
				data[0].TotalTime = startTime.GetDuration();
#else
				//calculate the shortest path in the valid channel
				GetShortestPath(currentPath, (fullRightPath.size() == 0) ? fullLeftPath : fullRightPath, x1, y1, x2, y2, r);
#endif
				//record the current channel
				currentChannel = (fullRightPath.size() == 0) ? fullLeftPath : fullRightPath;
				//and return success
				return true;
			}
		}
		//get the root of the start triangle
		SeDcdtFace *face = startFace2;
		SeDcdtFace *destinationFace;
		float destinationAngle;
		//go through that triangle's edges
		for (int i = 0; i < 3; i++)
		{
			//if the degree-3 node adjacent across that edge is the first goal node,
			if (face->link->Adjacent(i) == goalNodes[0].Triangle())
			{
				//record the angle (distance) to that node
				destinationAngle = face->link->Angle(i);
				break;
			}
		}
		//next go through the edges of the triangle at the root of the goal
		for (int i = 0; i < 3; i++)
		{
			//when we find the edge across which the first goal node is adjacent,
			if (goalFace2->link->Adjacent(i) == goalNodes[0].Triangle())
			{
				//we check whether this node or the other is closer to it,
				//and set the destination face to be the degree-3 node adjacent to the degree-2
				//start node such that we expect to cross the degree-2 goal node when moving to it
				destinationFace = goalNodes[(goalFace2->link->Angle(i) < destinationAngle) ? 0 : 1].Triangle();
				break;
			}
		}
		//go from the degree-2 start node to the destination face, building a channel as we go
		SrArray<SeBase *> channel;
		channel.size(0);
		//if we reach the destination face, the start and goal are on different edges with the same endpoints
		while (face != destinationFace)
		{
			//if we reach the degree-2 goal face, they're on the same edge,
			if (face == goalFace2)
			{
#if defined EXPERIMENT
				//start timing construction
				Timer construction;
#endif
				//create the start path from the start face to its root
				SrArray<SeBase *> startPath;
				WalkBetween(startPath, startFace, startFace2, 0.0f);
				//discard the overlapping triangle
				startPath.pop();
				//create the goal path from the goal face to its root
				SrArray<SeBase *> goalPath;
				WalkBetween(goalPath, goalFace, goalFace2, 0.0f);
#if defined EXPERIMENT
				//keep track of the number of triangles visited
				triangles += startPath.size() + goalPath.size() + 1;
#endif
				//invert the (backwards) goal path
				channel.revert();
				//add the channel we built to the end of the start path
				while (!channel.empty())
				{
					startPath.push() = channel.pop();
				}
				//then add the goal path
				while (!goalPath.empty())
				{
					startPath.push() = goalPath.pop();
				}
				//discard the extra triangle at the end
				startPath.pop();
				//if the path is valid,
				if (ValidPath(startPath, x1, y1, x2, y2, r))
				{
					//calculates the shortest path in that channel and saves the length of it
					shortestPathLength = GetShortestPath(shortestPath, startPath, x1, y1, x2, y2, r);
					shortestChannel = startPath;
#if defined EXPERIMENT
					//keeps track of the number of paths found
					paths++;
					//records the length
					data[0].Length = shortestPathLength;
					//and the construction time
					data[0].ConstructionTime = construction.GetDuration();
					//and the triangles visited
					data[0].ConstructionNodes = triangles;
					//and the time so far
					data[0].TotalTime = startTime.GetDuration();
					//move to the next data point
					currentData++;
#endif
				}
#if defined EXPERIMENT
				//stop timing construction
				constructionTime += construction.GetDuration();
#endif
				//remove from the goal nodes the one that is closest to the start
				goalNodes.remove((goalNodes[0].Triangle() == destinationFace) ? 1 : 0);
				//and remove the other one from the queue
				if (q.top()->Triangle() == destinationFace)
				{
					q.pop();
				}
				else
				{
					SearchNode *temp = q.top();
					q.pop();
					q.pop();
					q.push(temp);
				}
				//stop building the channel
				break;
			}
			//go through the edges of the current triangle
			SeBase *s = face->se();
			for (int i = 0; i < 3; i++)
			{
				//when we find the edge across which the destination node is adjacent,
				if (face->link->Adjacent(i) == destinationFace)
				{
					//add this triangle to the channel
					channel.push() = s;
					//and move to the triangle across that edge
					face = (SeDcdtFace *)s->sym()->fac();
					break;
				}
				//go to the next edge
				s = s->nxt();
			}
		}
	}
	//record the Euclidean distance between the start and goal
	float startGoalDistance = Length(x1, y1, x2, y2);
#if defined FIRST_PATH_FAST
	//create a second queue if running to the first path
	std::priority_queue<SearchNode *, std::vector<SearchNode *>, check> q2;
	//and begin marking the triangle mesh
	_mesh->begin_marking();
#endif
	//actual abstract degree-3 node search finally begins!
	//continue until all nodes have been exhausted
	while (!q.empty())
	{
#if defined EXPERIMENT
		//if it is time to collect more data,
		if ((currentData > 0) && (startTime.GetDuration() >= data[currentData].TotalTime * data[0].TotalTime))
		{
			//if this is the last data point,
			if (currentData == numData - 1)
			{
				//empty the main queue of nodes
				while (!q.empty())
				{
					//closing and deleting all nodes in there
					q.top()->Close();
					delete q.top();
					q.pop();
				}
#if defined FIRST_PATH_FAST
				//empty the secondary queue similarly
				while (!q2.empty())
				{
					//closing and deleting all nodes in there
					q2.top()->Close();
					delete q2.top();
					q2.pop();
				}
#endif
				//and stop searching
				break;
			}
			//record the total time for the algorithm,
			data[currentData].TotalTime = startTime.GetDuration();
			//the construction time,
			data[currentData].ConstructionTime = constructionTime;
			//the number of nodes searched
			data[currentData].SearchNodes = nodes;
			//the shortest path found,
			data[currentData].Length = shortestPathLength;
			//the number of paths found,
			data[currentData].Paths = paths;
			//and the number of triangles used in construction
			data[currentData].ConstructionNodes = triangles;
			//and move to the next data item
			currentData++;
		}
#endif
		//get the most promising search node off of the queue
		SearchNode *current = q.top();
		q.pop();
#if defined FIRST_PATH_FAST
		//if we want the first path fast and we don't have it yet but we have searched this triangle,
		if ((shortestPathLength == INFINITY) && (_mesh->marked(current->Triangle())))
		{
			//put it on the secondary queue for now and move to the next node
			q2.push(current);
			continue;
		}
		//mark the current triangle as searched
		_mesh->mark(current->Triangle());
#endif
#if defined EXPERIMENT
		//keep track of the number of nodes searched
		nodes++;
#endif
		//if the shortest path through the current node is longer than
		//the best path found so far, we are done searching
		if (current->f() >= shortestPathLength)
		{
			//empty the queue,
			while (!q.empty())
			{
				//closing and deleting all nodes in there
				q.top()->Close();
				delete q.top();
				q.pop();
			}
#if defined FIRST_PATH_FAST
			while (!q2.empty())
			{
				//closing and deleting all nodes in there
				q2.top()->Close();
				delete q2.top();
				q2.pop();
			}
#endif
			//and the current node
			current->Close();
			delete current;
			//stop searching
			break;
		}
		//record if we found the goal
		bool found = false;
		//go through the goal nodes
		for (int i = 0; i < goalNodes.size(); i++)
		{
			//if the current triangle is a goal node,
			if (current->Triangle() == goalNodes[i].Triangle())
			{
#if defined EXPERIMENT
				//start timing construction
				Timer construction;
#endif
				//construct the channel corresponding to the path so far
				SrArray<SeBase *> channel;
				ConstructBasePath(channel, current, startFace, goalFace, goalNodes[i].Direction());
#if defined EXPERIMENT
				//keep track of the number of triangles visited
				triangles += channel.size();
#endif
				//only check the channel is it is wide enough
				if (ValidEndpoints(channel, x1, y1, x2, y2, r))
				{
#if defined FIRST_PATH_FAST
					//if we found the first path, put all the nodes in the secondary queue
					while (!q2.empty())
					{
						//back in the main queue
						q.push(q2.top());
						q2.pop();
					}
#endif
					//get the shortest path and its length
					SrPolygon currentShortestPath;
					float currentPathLength = GetShortestPath(currentShortestPath, channel, x1, y1, x2, y2, r);
					//if it's the best path found so far, record it
					if (currentPathLength < shortestPathLength)
					{
						shortestPathLength = currentPathLength;
						shortestPath.size(0);
						shortestPath = currentShortestPath;
						shortestChannel.size(0);
						shortestChannel = channel;
					}
#if defined EXPERIMENT
					//keep track of the number of paths found
					paths++;
#endif
				}
#if defined EXPERIMENT
				//record the construction time
				constructionTime += construction.GetDuration();
#endif
				//we have found a solution
				found = true;
			}
		}
		//if we found the goal, don't generate any children
		if (found)
		{
			//close and delete the goal node
			current->Close();
			delete current;
#if defined EXPERIMENT
			//if this is the first path,
			if (currentData < 1)
			{
				//if it's also the final data point, stop searching
				if (currentData == numData - 1)
				{
					break;
				}
				//record the total time so far
				data[currentData].TotalTime = startTime.GetDuration();
				//and the construction time
				data[currentData].ConstructionTime = constructionTime;
				//and the total number of nodes searched
				data[currentData].SearchNodes = nodes;
				//and the shortest path found so far
				data[currentData].Length = shortestPathLength;
				//and the number of paths found so far
				data[currentData].Paths = paths;
				//and the number of triangles visited
				data[currentData].ConstructionNodes = triangles;
				//and move to the next data point
				currentData++;
			}
#endif
			//get the next node from the queue
			continue;
		}
		//go through the edges of this triangle
		SeBase *s = current->Triangle()->se();
		for (int i = 0; i < 3; i++)
		{
			//make sure not to cross the starting edge back the way we came
			if (((current->Triangle() == startTriangle1) && (current->Triangle()->link->Adjacent(i) == startTriangle2))
				|| ((current->Triangle() == startTriangle2) && (current->Triangle()->link->Adjacent(i) == startTriangle1)))
			{
				continue;
			}
			//get the degree-3 node adjacent across that edge
			SeDcdtFace *childFace = current->Triangle()->link->Adjacent(i);
			//get the width through this triangle and through the corridor to the next triangle
			float width = (current->Back() == NULL) ? INFINITY :
				(current->Triangle()->link->Adjacent((i + 1) % 3) == current->Back()->Triangle()) ?
				current->Triangle()->link->Width(i) : current->Triangle()->link->Width((i + 2) % 3);
			width = Minimum(current->Triangle()->link->Choke(i), width);
			//only consider this triangle if it's wide enough to get there and we haven't
			//searched it before (and it's not the way we just came)
			if ((!current->Searched(childFace)) && (width >= 2.0f * r)
				&& ((current->Back() == NULL) || (childFace != current->Back()->Triangle())))
			{
				//get the angle we cross the current triangle to get to the child
				float x_1, y_1, x_2, y_2, x_3, y_3;
				SeBase *s1 = childFace->se();
				for (int j = 0; j < 3; j++)
				{
					if (childFace->link->Adjacent(j) == current->Triangle())
					{
						TriangleVertices(s1, x_1, y_1, x_2, y_2, x_3, y_3);
						break;
					}
					s1 = s1->nxt();
				}
				//get the closest point on the entry edge to the goal
				SrPnt2 p = ClosestPointOn(x_1, y_1, x_2, y_2, x2, y2, r);
				//and the start
				SrPnt2 p2 = ClosestPointOn(x_1, y_1, x_2, y_2, x1, y1, r);
				float x_4 = x_1;
				float y_4 = y_1;
				float x_5 = x_2;
				float y_5 = y_2;
				TriangleVertices(s, x_1, y_1, x_2, y_2, x_3, y_3);
				//get the angle through the last triangle to this one
				float theta = (current->Back() == NULL) ? 0.0f : (current->Triangle()->link->Adjacent((i + 1) % 3) == current->Back()->Triangle()) ?
					abs(AngleBetween(x_1, y_1, x_2, y_2, x_3, y_3)) : abs(AngleBetween(x_3, y_3, x_1, y_1, x_2, y_2));
				//the heuristic is the closest (Euclidean) distance to the goal
				float h = Length(p.x, p.y, x2, y2);
				//the distance travelled so far is the distance for the parent plus the max of the total angle through
				//the parent triangle and the corridor from it to this triangle times the object's radius, and the difference
				//between the parent nodes heuristic and this node's
				float minDistance = current->g() + Maximum((current->Triangle()->link->Angle(i) + theta) * r, current->h() - h);
				//maxxed with the distance between the start and the closest point to it on the entry edge
				minDistance = Maximum(Length(x1, y1, p2.x, p2.y), minDistance);
				//maxxed with the Euclidean distance between the start and goal minus this node's heuristic
				minDistance = Maximum(minDistance, startGoalDistance - h);
				//maxxed with the parent's distance travelled plus the minimum distance between its entry edge and
				//that of this node, plus the angle through the parent node times the unit's radius
				minDistance = Maximum(minDistance, current->g() + SegmentDistance(x_1, y_1, x_2, y_2, x_5, y_5, x_4, y_4, r) + theta * r);
				//if this child might yield a shorter path,
				if ((minDistance + h) < shortestPathLength)
				{
					//push a node with these values onto the priority queue
					q.push(new SearchNode(minDistance, h, p, childFace, current, i));
					//tell the parent node it has another child
					current->OpenChild();
				}
			}
			//move to the next edge of the parent node's triangle
			s = s->nxt();
		}
		//if no children were opened,
		if (current->OpenChildren() <= 0)
		{
			//we can close and delete the current node now
			current->Close();
			delete current;
		}
	}
#if defined FIRST_PATH_FAST
	//stop marking searched triangles
	_mesh->end_marking();
	//go through the secondary queue
	while (!q2.empty())
	{
		//closing and deleting all nodes in there
		q2.top()->Close();
		delete q2.top();
		q2.pop();
	}
#endif
	//save the shortest path found and its channel
	currentPath = shortestPath;
	currentChannel = shortestChannel;
#if defined EXPERIMENT
	//record the total time of the algorithm
	data[currentData].TotalTime = startTime.GetDuration();
	//and the construction time
	data[currentData].ConstructionTime = constructionTime;
	//and the number of nodes searched
	data[currentData].SearchNodes = nodes;
	//and the length of the shortest path found
	data[currentData].Length = shortestPathLength;
	//and the total number of paths
	data[currentData].Paths = paths;
	//and finally the number of triangles used for construction
	data[currentData].ConstructionNodes = triangles;
#endif
	//return whether or not one was found
	return (shortestPathLength < INFINITY);
}

//searches the base graph for a path between two points
#if defined EXPERIMENT
bool SeDcdt::SearchPathBaseFast(float x1, float y1, float x2, float y2, float r, Data *data, int numData)//, float &length, float &time, int &nodes)
#else
bool SeDcdt::SearchPathBaseFast(float x1, float y1, float x2, float y2, float r)//, float &length, float &time, int &nodes)
#endif
{
#if defined EXPERIMENT
	//start timing the algorithm
	Timer startTime;
	//keep track of the total tome spent constructing paths
	float constructionTime = 0.0f;
	//and the current data point
	int currentData = 0;
	//and the number of triangles searched
	int nodes = 0;
	//and the number of paths found
	int paths = 0;
#endif
	//initialize the path to empty
	currentPath.size(0);
	SeBase *start;
	//find the triangle containing the start point
	SeTriangulator::LocateResult startResult = LocatePoint(x1, y1, start);
	//if it wasn't found, return a path could not be found
	if (startResult == SeTriangulator::NotFound)
	{
		return false;
	}
	//get the starting triangle
	SeDcdtFace *startFace = (SeDcdtFace *)start->fac();
	//create a priority queue for the search nodes
	std::priority_queue<SearchNode *, std::vector<SearchNode *>, check> q;
#if defined FIRST_PATH_FAST
	//if we are finding the first path quickly, initialize a secondary queue
	std::priority_queue<SearchNode *, std::vector<SearchNode *>, check> q2;
#endif
	//calculate the closest point to the goal in the first triangle
	SrPnt2 p = ClosestPointTo(startFace, x2, y2, r);
	//use the distance between that and the goal as the heuristic in the first search node
	q.push(new SearchNode(0.0f, Length(p.x, p.y, x2, y2), p, startFace, NULL));
	//the shortest path so far and its length and channel
	float shortestPathLength = INFINITY;
	SrPolygon shortestPath;
	SrArray<SeBase *> shortestChannel;
#if defined FIRST_PATH_FAST
	//start marking which triangles have been explored
	_mesh->begin_marking();
#endif
	//we start with 1 unexplored triangle on the queue
//	int unexplored = 1;
	//calculate the Euclidean distance between the start and goal
	float startGoalDistance = Length(x1, y1, x2, y2);
	//continue until all nodes have been exhausted
	while (!q.empty())
	{
#if defined EXPERIMENT
		//if it's time to record another data point,
		if ((currentData > 0) && (startTime.GetDuration() >= data[currentData].TotalTime * data[0].TotalTime))
		{
			//if it's the last data point,
			if (currentData == numData - 1)
			{
				//empty the primary queue, close and delete all remaining nodes
				while (!q.empty())
				{
					q.top()->Close();
					delete q.top();
					q.pop();
				}
#if defined FIRST_PATH_FAST
				//empty the secondary queue, close and delete all remaining nodes
				while (!q2.empty())
				{
					q2.top()->Close();
					delete q2.top();
					q2.pop();
				}
#endif
				//and stop searching
				break;
			}
			//get the total time spent so far
			data[currentData].TotalTime = startTime.GetDuration();
			//the time spent on construction
			data[currentData].ConstructionTime = constructionTime;
			//the number of nodes searched
			data[currentData].SearchNodes = nodes;
			//the shortest path found
			data[currentData].Length = shortestPathLength;
			//and the number of paths found
			data[currentData].Paths = paths;
			//and move to the next data point
			currentData++;
		}
#endif
		//gets the best search node so far
		SearchNode *current = q.top();
		q.pop();
#if defined FIRST_PATH_FAST
		//if we want the first path quickly and we haven't found one yet but have searched this triangle before,
		if ((shortestPathLength == INFINITY) && (_mesh->marked(current->Triangle())))
		{
			//push it onto the secondary queue for now
			q2.push(current);
			//and get the next most promising node
			continue;
		}
		//otherwise, mark this triangle as visited
		_mesh->mark(current->Triangle());
#endif
#if defined EXPERIMENT
		//keep track of the number of nodes searched
		nodes++;
#endif
		//if the shortest path through this node is longer than the best path we have found so far,
		//or if we have no unexplored nodes on the queue and we haven't found a path yet
		if ((current->f() >= shortestPathLength))// || ((unexplored <= 0) && (shortestPathLength == INFINITY)))
		{
			//empty the primary queue, close and delete all remaining nodes
			while (!q.empty())
			{
				q.top()->Close();
				delete q.top();
				q.pop();
			}
#if defined FIRST_PATH_FAST
			//empty the secondary queue, close and delete all remaining nodes
			while (!q2.empty())
			{
				q2.top()->Close();
				delete q2.top();
				q2.pop();
			}
#endif
			//close and delete the current node
			current->Close();
			delete current;
			//exit the loop
			break;
		}
		//if we haven't explored the current node yet
//		if (!_mesh->marked(current->Triangle()))
//		{
			//mark it as explored now
//			_mesh->mark(current->Triangle());
			//we are taking an unexplored node off of the queue
//			unexplored--;
//		}
		//if the goal is in the current triangle,
		if (InTriangle(current->Triangle(), x2, y2))
		{
#if defined EXPERIMENT
			//time the construction
			Timer construct;
#endif
			//construct the channel formed by our search
			SrArray<SeBase *> channel = ConstructBaseChannel(current);
			//close and delete the current search node
			current->Close();
			delete current;
			//check if the endpoints are valid
			if (ValidEndpoints(channel, x1, y1, x2, y2, r))
			{
#if defined FIRST_PATH_FAST
				//once we find a path put all the nodes from the secondary queue on to the primary one
				while (!q2.empty())
				{
					q.push(q2.top());
					q2.pop();
				}
#endif
				//calculate the path and its length
				SrPolygon currentShortestPath;
				float currentPathLength = GetShortestPath(currentShortestPath, channel, x1, y1, x2, y2, r);
				//if this is the best path so far,
				if (currentPathLength < shortestPathLength)
				{
					//set this as the best path
					shortestPathLength = currentPathLength;
					shortestPath.size(0);
					shortestPath = currentShortestPath;
					shortestChannel.size(0);
					shortestChannel = channel;
				}
			}
#if defined EXPERIMENT
			//keep track of the total time spent doing construction
			constructionTime += construct.GetDuration();
			//keep track of the number of paths found
			paths++;
			//if we are on the first data point,
			if (currentData < 1)
			{
				//also if we're on the last data point,
				if (currentData == numData - 1)
				{
					//empty the primary queue, close and delete all remaining nodes
					while (!q.empty())
					{
						q.top()->Close();
						delete q.top();
						q.pop();
					}
					//and stop searching
					break;
				}
				//record the time spent so far on the algorithm
				data[currentData].TotalTime = startTime.GetDuration();
				//and the time spent on construction
				data[currentData].ConstructionTime = constructionTime;
				//and the number of triangles searched
				data[currentData].SearchNodes = nodes;
				//and the length of the shortest path
				data[currentData].Length = shortestPathLength;
				//and the number of paths found
				data[currentData].Paths = paths;
				//and move to the next data point
				currentData++;
			}
#endif
			//move to the next triangle
			continue;
		}
		//go through the edges of the current triangle
		SeBase *s = current->Triangle()->se();
		for (int i = 0; i < 3; i++)
		{
			//only consider crossing edges that aren't blocked
			if (!Blocked(s))
			{
				//get the triangle associated with the potential child node
				SeDcdtFace *childFace = (SeDcdtFace *)s->sym()->fac();
				//calculate the width through this triangle between the last triangle and the next
				float width = (current->Back() == NULL) ? INFINITY : (s->nxt()->sym()->fac() == current->Back()->Triangle()) ?
					current->Triangle()->link->Width(i) : current->Triangle()->link->Width((i + 2) % 3);
				//only generate the child node if we haven't explored it yet in this path, the width through
				//the triangle is enough, and it's not the last face we explored
				if ((!current->Searched(childFace)) && (width >= 2.0f * r)
					&& ((current->Back() == NULL) || (childFace != current->Back()->Triangle())))
				{
					//get a component of the child face
					SeBase *s1 = childFace->se();
					float x_1, y_1, x_2, y_2, x_3, y_3;
					//go through the edges to determine which is the entry edge
					for (int j = 0; j < 3; j++)
					{
						//that is the edge across which is the parent triangle
						if (s1->sym()->fac() == s->fac())
						{
							//and get the vertices starting from that edge
							TriangleVertices(s1, x_1, y_1, x_2, y_2, x_3, y_3);
							break;
						}
						//move to the next edge to check
						s1 = s1->nxt();
					}
					//get the closest point on the entry edge to the goal
					SrPnt2 p = ClosestPointOn(x_1, y_1, x_2, y_2, x2, y2, r);
					//and the start
					SrPnt2 p2 = ClosestPointOn(x_1, y_1, x_2, y_2, x1, y1, r);
					//get the parent triangle's vertices
					TriangleVertices(s, x_1, y_1, x_2, y_2, x_3, y_3);
					//and calculate the angle through that triangle to this one
					float theta = (current->Back() == NULL) ? 0.0f : (s->nxt()->sym()->fac() == current->Back()->Triangle()) ?
						abs(AngleBetween(x_1, y_1, x_2, y_2, x_3, y_3)) : abs(AngleBetween(x_3, y_3, x_1, y_1, x_2, y_2));
					//the heuristic is the Euclidean distance between the goal and the closest point
					//to it in this triangle's entry edge
					float h = Length(p.x, p.y, x2, y2);
					//the distance travelled is the maximum of that for the parent triangle
					//plus the maximum of the angle through the parent triangle times the
					//unit's radius, and the parent triangle's heuristic minus that of this one
					float minDistance = current->g() + Maximum(theta * r, current->h() - h);
					//maxxed with the minimum distance between this triangle's entry edge and the start
					minDistance = Maximum(Length(x1, y1, p2.x, p2.y), minDistance);
					//maxxed with the Euclidean distance between the start and goal minus this
					//triangle's heuristic value
					minDistance = Maximum(minDistance, startGoalDistance - h);
					//if this child could yield a shorter path than the best found so far,
					if ((minDistance + h) < shortestPathLength)
					{
						//put the child search node on the queue
						q.push(new SearchNode(minDistance, h, p, childFace, current));
						//tell the current node that it has an open child
						current->OpenChild();
						//if we haven't explored this triangle before,
//						if (!_mesh->marked(childFace))
//						{
//							//we are putting an unexplored node on the queue
//							unexplored++;
//						}
					}
				}
			}
			//move to the next edge of the parent triangle
			s = s->nxt();
		}
		//if no children were opened, close the current node and delete it
		if (current->OpenChildren() <= 0)
		{
			current->Close();
			delete current;
		}
	}
#if defined FIRST_PATH_FAST
	//empty the queue, close and delete all remaining nodes
	while (!q2.empty())
	{
		q2.top()->Close();
		delete q2.top();
		q2.pop();
	}
	//stop marking the triangle mesh
	_mesh->end_marking();
#endif
	//set the current path and channel to the best one found
	currentPath = shortestPath;
	currentChannel = shortestChannel;
	//set the start and goal points
	startPoint.set(x1, y1);
	goalPoint.set(x2, y2);
//	_mesh->end_marking();
#if defined EXPERIMENT
	//stop timing the algorithm and record the total time spent
	data[currentData].TotalTime = startTime.GetDuration();
	//record the total time spent on construction
	data[currentData].ConstructionTime = constructionTime;
	//record the number of nodes searched
	data[currentData].SearchNodes = nodes;
	//record the length of the shortest path
	data[currentData].Length = shortestPathLength;
	//and finally the total number of paths found
	data[currentData].Paths = paths;
#endif
	//return whether a path was found or not
	return (shortestPathLength < INFINITY);
}

//construct the channel given the last SearchNode in the solution
SrArray<SeBase *> SeDcdt::ConstructBaseChannel(SearchNode *goal)
{
	SrArray<SeBase *> channel;
	//if there is only one node, return an empty path
	if (goal->Back() == NULL)
	{
		return channel;
	}
	//otherwise, move back in the chain
	SearchNode *current = goal->Back();
	//until we are at the second-to-first node
	while (current->Back() != NULL)
	{
		//go through the edges of the triangle associated with this node
		SeBase *s = current->Triangle()->se();
		for (int i = 0; i < 3; i++)
		{
			//if the previous triangle is across this edge,
			if (s->sym()->fac() == current->Back()->Triangle())
			{
				//add this edge/face to the channel
				channel.push() = s;
				//and move to the next node
				break;
			}
			//go to the next edge in the triangle
			s = s->nxt();
		}
		//move to the previous node in the chain
		current = current->Back();
	}
	//add the last triangle to the channel
	channel.push() = current->Triangle()->se();
	//the channel was constructed backward, so flip it, then return it
	channel.revert();
	return channel;
}

//uses the modified funnel algorithm to get the shortest path for nonzero-radius units
//and return the length of this path
float SeDcdt::GetShortestPath(SrPolygon &path, SrArray<SeBase *> Channel, float x1, float y1, float x2, float y2, float r)
{
	//initializes the path
	path.open(true);
	path.size(0);
	//if there are no edges in the channel, the path is a straight line
	if (Channel.size() < 1)
	{
		path.push().set(x1, y1);
		path.push().set(x2, y2);
		return Length(x1, y1, x2, y2);
	}
	//otherwise, initialize a funnel deque
	//with the starting point
	FunnelDeque fd(x1, y1, r);
	//get the first edge in the channel
	SeBase *s = Channel[0];
	//get the reference whose sym edge
	//belongs to the next triangle in the channel
	for (int i = 0; i < 3; i++)
	{
		if (((Channel.size() > 1) && (s->sym()->fac() == Channel[1]->fac()))
			|| (InTriangle((SeDcdtFace *)s->sym()->fac(), x2, y2)))
		{
			break;
		}
		s = s->nxt();
	}
	//get the point on the right side of that edge
	SrPnt2 p;
	p.set(((SeDcdtVertex *)s->vtx())->p);
	//insert it into the deque
	fd.Add(p, FunnelDeque::LeftTangent, path);
	SeVertex *right = s->vtx();
	//get the point on the left side of that edge
	s = s->nxt();
	SeVertex *left = s->vtx();
	p.set(((SeDcdtVertex *)s->vtx())->p);
	//insert it into the deque
	fd.Add(p, FunnelDeque::RightTangent, path);
	//go through the other edges in the channel
	for (int i = 1; i < Channel.size() - 1; i++)
	{
		s = Channel[i];
		//get the reference whose sym edge
		//belongs to the next triangle in the channel
		for (int j = 0; j < 3; j++)
		{
			if (s->sym()->fac() == Channel[i + 1]->fac())
			{
				break;
			}
			s = s->nxt();
		}
		//if that edge shares a vertex with the right side
		//of the funnel, the opposite point should be
		//added to the left side of the funnel
		if (s->vtx() == right)
		{
			p.set(((SeDcdtVertex *)s->nxt()->vtx())->p);
			fd.Add(p, FunnelDeque::RightTangent, path);
			left = s->nxt()->vtx();
		}
		//otherwise, add it to the right side of the funnel
		else //(s->nxt()->vtx() == left)
		{
			p.set(((SeDcdtVertex *)s->vtx())->p);
			fd.Add(p, FunnelDeque::LeftTangent, path);
			right = s->vtx();
		}
	}
	//go to the last edge in the channel
	s = Channel[Channel.size() - 1];
	//find the reference across which is
	//the triangle with the goal in it
	for (int j = 0; j < 3; j++)
	{
		if (InTriangle((SeDcdtFace *)s->sym()->fac(), x2, y2))
		{
			break;
		}
		s = s->nxt();
	}
	//like above, determines whether to add
	//the point to the left or right side of the funnel
	if (s->vtx() == right)
	{
		p.set(((SeDcdtVertex *)s->nxt()->vtx())->p);
		fd.Add(p, FunnelDeque::RightTangent, path);
	}
	else //(s->nxt()->vtx() == left)
	{
		p.set(((SeDcdtVertex *)s->vtx())->p);
		fd.Add(p, FunnelDeque::LeftTangent, path);
	}
	//finally, add the goal point to the funnel
	p.set(x2, y2);
	fd.Add(p, FunnelDeque::Point, path);
	//calculate the length of the path sand return it
	return fd.Length(path);
}

//returns a polygon with the boundary of the current channel
SrPolygon SeDcdt::GetChannelBoundary()
{
	//start the boundary with the starting point, go clockwise
	SrPolygon boundary;
	//if there is no channel, return an empty boundary
	if (currentChannel.size() == 0)
	{
		boundary.size(0);
		return boundary;
	}
	boundary.push().set(startPoint.x, startPoint.y);
	SeBase *s;
	//go through each of the triangles in the channel (except the last one)
	for (int i = 0; i < currentChannel.size() - 1; i++)
	{
		//go through the edges of the current triangle
		s = currentChannel[i];
		for (int j = 0; j < 3; j++)
		{
			//if this edge borders the next triangle,
			if (s->sym()->fac() == currentChannel[i + 1]->fac())
			{
				//add the vertex on the other side of this edge to the boundary
				boundary.push().set(((SeDcdtVertex *)s->nxt()->vtx())->p.x, ((SeDcdtVertex *)s->nxt()->vtx())->p.y);
				break;
			}
			//go to the next edge
			s = s->nxt();
		}
	}
	//now deal with the last triangle in the channel
	s = currentChannel[currentChannel.size() - 1];
	//go through its edges
	for (int i = 0; i < 3; i++)
	{
		//find the edge shed with the triangle containing the goal point
		if (InTriangle((SeDcdtFace *)s->sym()->fac(), goalPoint.x, goalPoint.y))
		{
			//add the vertex on the other end of this edge, the goal point,
			//and the vertex at this end of the edge to the boundary
			boundary.push().set(((SeDcdtVertex *)s->nxt()->vtx())->p.x, ((SeDcdtVertex *)s->nxt()->vtx())->p.y);
			boundary.push().set(goalPoint.x, goalPoint.y);
			boundary.push().set(((SeDcdtVertex *)s->vtx())->p.x, ((SeDcdtVertex *)s->vtx())->p.y);
			break;
		}
		//go to the next edge
		s = s->nxt();
	}
	//now go backwards through the triangles in the channel (starting with the second-last one)
	for (int i = currentChannel.size() - 2; i >= 0; i--)
	{
		//go through the edges of the current triangle
		s = currentChannel[i];
		for (int j = 0; j < 3; j++)
		{
			//if this edge borders the next triangle,
			if (s->sym()->fac() == currentChannel[i + 1]->fac())
			{
				//add the vertex on this side of the edge to the boundary
				boundary.push().set(((SeDcdtVertex *)s->vtx())->p.x, ((SeDcdtVertex *)s->vtx())->p.y);
				break;
			}
			s = s->nxt();
		}
	}
	//return the boundary
	return boundary;
}

//returns the current path
SrPolygon &SeDcdt::GetPath()
{
	return currentPath;
}

//Abstract space searching function definitions }
