//Abstract.cpp

//DJD: Abstract function definitions {
#include "precompiled.h"
#include "0ad_warning_disable.h"

#include <math.h>
#include <fstream>

#include "se_dcdt.h"

//the degree for an unabstracted triangle
#define UNABSTRACTED -1

template <class T>
SrArray<SeLinkFace<T> *> SeLinkFace<T>::processing;

void SeDcdt::Abstract()
{
#if defined EXPERIMENT
	//count the number of degree-3 nodes in the abstract graph
	int num = 0;
#endif
//Degree-1 and Degree-0 faces
	//keep track of the connected component
	int component = 0;
	//possible degree-1 triangles
	SrArray<SeDcdtFace *> degree1;
	//possible degree-2 or 3 triangles
	SrArray<SeDcdtFace *> processing;
	//get first face and record it
	SeDcdtFace *currentFace = _mesh->first()->fac();
	SeDcdtFace *firstFace = currentFace;
	if (outside == NULL)
	{
		//first determine the "outside" face
//		for (int n = 0; n < SeDcdtFace::Faces(); n++)
		do
		{
//			SeDcdtFace *currentFace = SeDcdtFace::Face(n);
			if (currentFace == NULL)
			{
				continue;
			}
			float x1, y1, x2, y2, x3, y3;
			TriangleVertices(currentFace, x1, y1, x2, y2, x3, y3);
			//outside face is any whose vertices aren't in counterclockwise order
			if (Orientation(x1, y1, x2, y2, x3, y3) <= 0.0f)
			{
				//record this face, reset, and break
				outside = currentFace;
				currentFace = firstFace;
				break;
			}
			//if it wasn't found, try the next face
			currentFace = currentFace->nxt();
		}
		while (currentFace != firstFace);
	}
	//go through all faces in the triangulation
//	for (int n = 0; n < SeDcdtFace::Faces(); n++)
	do
	{
//		SeDcdtFace *currentFace = SeDcdtFace::Face(n);
		//don't process the outside face
		if ((currentFace == NULL) || (currentFace == outside))
		{
			currentFace = currentFace->nxt();
			continue;
		}
		//keep track of how many edges are constrained
		int numConstrained = 0;
		//this is the adjacent face to a degree-1 triangle
		SeDcdtFace *tempFace = NULL;
		//access the parts of the face
		SeBase *s = currentFace->se();
		//count the number of constrained edges around this face
		for (int i = 0; i < 3; i++)
		{
			//consider a constrained edge or one bordering the outside face to be blocked
			if (Blocked(s))
			{
				numConstrained++;
			}
			//if there's an unconstrained edge, record the face across it
			else
			{
				tempFace = (SeDcdtFace *)(s->sym()->fac());
			}
			//go to the next element of the triangle
			s = s->nxt();
		}
		//if there are two constrained edges, this is a degree-1 triangle
		if (numConstrained == 2)
		{
			//abstract the face and calculate its widths
			currentFace->link = new Abstraction(1);
			CalculateWidths(currentFace);
			//if the face across the unconstrained edge is already degree-1,
			if (Degree(tempFace) == 1)
			{
				//this is a tree component
				TreeAbstract(tempFace, component++);
			}
			//if it's not abstracted, it might now be a degree-1 node
			else if (Degree(tempFace) == UNABSTRACTED)
			{
				degree1.push() = tempFace;
			}
		}
		//if all edges are constrained, this is a degree-0 triangle
		else if (numConstrained == 3)
		{
			//abstract it and set its component
			currentFace->link = new Abstraction(0);
			currentFace->link->Component(component++);
		}
		//if it's not degree-0 or 1, it might be degree-2 or 3
		else
		{
			//enqueue it for later processing
			processing.push() = currentFace;
		}
		//move to the next face in the mesh
		currentFace = currentFace->nxt();
	}
	//continue until reaching the first face again
	while (currentFace != firstFace);
	//keep track of which faces we have considered for degree-1 nodes
	_mesh->begin_marking();
	//go through the possible degree-1 nodes
	while (!degree1.empty())
	{
		//get the next possible degree-1 node
		SeDcdtFace *currentFace = degree1.pop();
		//if this face has already been dealt with, skip it
		if ((_mesh->marked(currentFace)) || (Degree(currentFace) != UNABSTRACTED))
		{
			continue;
		}
		//keep track of the number of adjacent triangles
		int numAdjacent = 0;
		//the face across an unconstrained edge
		SeDcdtFace *tempFace = NULL;
		//access the elements of the triangle
		SeBase *s = currentFace->se();
		//calculate the number of unabstracted faces across unconstrained edges
		for (int i = 0; i < 3; i++)
		{
			//only consider adjacent, unabstracted edges
			if (!Blocked(s) && (Degree(s->sym()) == UNABSTRACTED))
			{
				numAdjacent++;
				tempFace = (SeDcdtFace *)(s->sym()->fac());
			}
			//move to the next edge of the triangle
			s = s->nxt();
		}
		//if there is only one, this is a degree-1 node
		if (numAdjacent == 1)
		{
			//abstract this node and calculate its widths
			currentFace->link = new Abstraction(1);
			CalculateWidths(currentFace);
			//the adjacent triangle is now possibly degree-1
			degree1.push() = tempFace;
			//mark this face as dealt with
			_mesh->mark(currentFace);
		}
		//if there aren't any, this is a tree component
		else if (numAdjacent == 0)
		{
			TreeAbstract(currentFace, component++);
			//mark this face as dealt with
			_mesh->mark(currentFace);
		}
	}
	//done with degree-1 nodes
	_mesh->end_marking();

//Degree-2 and Degree-3 faces
	//go through all possible degree-3 triangles
	for (int n = 0; n < processing.size(); n++)
	{
		//gets the current face to check
		SeDcdtFace *currentFace = processing[n];
		//we only care about unabstracted nodes at this point
		if (Degree(currentFace) != UNABSTRACTED)
		{
			continue;
		}
		//the number of non-degree-1 triangles across unconstrained edges
		int numAdjacent = 0;
		//access the elements of the triangle
		SeBase *s = currentFace->se();
		//go through the edges of the triangle
		for (int i = 0; i < 3; i++)
		{
			//get the degree of the triangle across that edge
			int degree = Degree(s->sym());
			//keep track of non-degree-1 edges across unconstrained edges
			if (!Blocked(s) && (degree != 1))
			{
				numAdjacent++;
			}
			//move to the next edge
			s = s->nxt();
		}
		//if this is not a degree-3 triangle, skip it
		if (numAdjacent < 3)
		{
			continue;
		}
		//stack of degree-3 nodes in this connected component
		SrArray<SeDcdtFace *> degree3;
		//put the current triangle on the stack
		degree3.push() = currentFace;
		//continue through all degree-3 nodes in this component
		while (!degree3.empty())
		{
			//get one of the degree-3 nodes from the stack
			SeDcdtFace *stackFace = degree3.pop();
			//abstract it and calculate its widths and set its connected component
			stackFace->link = new Abstraction(3);
#if defined EXPERIMENT
			//keep track of the extra degree-3 node
			num++;
#endif
			CalculateWidths(stackFace);
			stackFace->link->Component(component);
			//access the elements of the triangle
			s = stackFace->se();
			//go in all directions from this node
			for (int i = 0; i < 3; i++)
			{
				//get the face across this particular edge
				SeDcdtFace *tempFace = (SeDcdtFace *)(s->sym()->fac());
				//mark the first face and the previous face
				SeDcdtFace *firstFace = stackFace;
				SeDcdtFace *lastFace = stackFace;
				//keep track of the cumulative angle since the last degree-3 node
				float angle = 0;
				//follow this chain until another degree-3 node is encountered
				while (true)
				{
					//the number of adjacent faces across unconstrained edges not abstracted as degree-1
					numAdjacent = 0;
					//access the elements of this triangle
					SeBase *s1 = tempFace->se();
					//go through the edges of this triangle
					for (int j = 0; j < 3; j++)
					{
						//get the degree of the face across this edge
						int degree = Degree(s1->sym());
						//if it's not blocked or degree-1, it's adjacent
						if (!Blocked(s1) && (degree != 1))
						{
							numAdjacent++;
						}
						//move to the next edge
						s1 = s1->nxt();
					}
					//if the current triangle is degree-3,
					if (numAdjacent == 3)
					{
						//if it hasn't been abstracted yet, put it on the stack to deal with
						if (Degree(tempFace) == UNABSTRACTED)
						{
							degree3.push() = tempFace;
						}
						//sets the original degree-3 node adjacent to this one
						stackFace->link->Adjacent(i, tempFace);
						//and sets the total angle between them
						stackFace->link->Angle(i, angle);
						//if there were no degree-2 nodes in between these,
						if (lastFace == stackFace)
						{
							//get the elements of this triangle
							SeBase *s1 = tempFace->se();
							//go through the edges
							for (int j = 0; j < 3; j++)
							{
								//find the edge between this triangle and the last one
								if (s1->sym()->fac() == lastFace)
								{
									//the choke point is the length of the edge between these faces
									float x1, y1, x2, y2, x3, y3;
									TriangleVertices(s1, x1, y1, x2, y2, x3, y3);
									stackFace->link->Choke(i, Length(x1, y1, x2, y2));
									break;
								}
								//move to the next edge
								s1 = s1->nxt();
							}
						}
						//if there were degree-2 nodes between them,
						else
						{
							//access the elements of the previous triangle
							SeBase *s1 = lastFace->se();
							float choke, width;
							//go through its edges
							for (int j = 0; j < 3; j++)
							{
								//find the direction of the original degree-3 node
								if (lastFace->link->Adjacent(j) == stackFace)
								{
									//get the choke point to this node
									choke = lastFace->link->Choke(j);
									//get the width through this triangle between the two degree-3 nodes
									width = lastFace->link->Width((s1->nxt()->sym()->fac() == tempFace) ? j : (j + 2) % 3);
									//the choke point of the original degree-3 node is the lesser of the two
									stackFace->link->Choke(i, Minimum(choke, width));
									break;
								}
								//move to the next edge
								s1 = s1->nxt();
							}
						}
						//we've reached the other degree-3 node - stop following this chain
						break;
					}
					//if the triangle is degree-2
					else if (numAdjacent == 2)
					{
						//if the triangle wasn't abstracted before,
						if (Degree(tempFace) == UNABSTRACTED)
						{
							//abstract it and set its widths and connected component
							tempFace->link = new Abstraction(2);
							CalculateWidths(tempFace);
							tempFace->link->Component(component);
						}
						//access the elements of this triangle
						SeBase *s1 = tempFace->se();
						//go through the edges of the triangle
						for (int j = 0; j < 3; j++)
						{
							//find the edge across which is the last triangle
							if (s1->sym()->fac() == lastFace)
							{
								//set the angle back towards the original degree-3 node
								tempFace->link->Angle(j, angle);
								//and set the adjacent degree-3 node in that direction
								tempFace->link->Adjacent(j, firstFace);
								//if there are no nodes between this one and the degree-3 node,
								if (lastFace == firstFace)
								{
									//the choke point is simply the length of the edge between them
									float x1, y1, x2, y2, x3, y3;
									TriangleVertices(s1, x1, y1, x2, y2, x3, y3);
									tempFace->link->Choke(j, Length(x1, y1, x2, y2));
								}
								else
								{
									float choke, width;
									//otherwise, access the elements of the last triangle
									SeBase *s2 = lastFace->se();
									//and go through the edges
									for (int k = 0; k < 3; k++)
									{
										//we want the edge which leads back to the original degree-3 node
										if ((s2->sym()->fac() == tempFace) || (lastFace->link->Adjacent(k) == NULL))
										{
											s2 = s2->nxt();
											continue;
										}
										//get the associated choke point width
										choke = lastFace->link->Choke(k);
										//get the width between the two degree-3 nodes
										if (s2->nxt()->sym()->fac() == tempFace)
										{
											width = lastFace->link->Width(k);
										}
										else
										{
											width = lastFace->link->Width((k + 2) % 3);
										}
										break;
									}
									//the choke point value is the lesser of these
									tempFace->link->Choke(j, Minimum(choke, width));
								}
								break;
							}
							//move to the next edge
							s1 = s1->nxt();
						}
						//calculate the angle between edges leading to degree-3 nodes
						float currentAngle = 0;
						//access the elements of this triangle again
						s1 = tempFace->se();
						//record the next triangle to move to
						SeDcdtFace *nextFace = NULL;
						//go through the edges of the triangle
						for (int j = 0; j < 3; j++)
						{
							//get the degree of the adjacent triangle across this edge
							int degree = Degree(s1->sym());
							//if there is a tree component off this node, collapse it
							if (!Blocked(s1) && (degree == 1)
								&& (((SeDcdtFace *)s1->sym()->fac())->link->Component() == INVALID))
							{
								TreeCollapse(tempFace, (SeDcdtFace *)(s1->sym()->fac()), component);
							}
							//when finding the next face, record it
							if (!Blocked(s1) && (s1->sym()->fac() != lastFace) && (degree != 1))
							{
								nextFace = (SeDcdtFace *)s1->sym()->fac();
							}
							//determine the degree of the triangle across the next edge
							int degree2 = Degree(s1->nxt()->sym());
							//if this edge and the next are the connecting edges, record the angle between them
							if (!((degree == 0) || (degree == 1) || (degree2 == 0) || (degree2 == 1))
								&& !Blocked(s1) && !Blocked(s1->nxt()))
							{
								float x1, y1, x2, y2, x3, y3;
								TriangleVertices(s1, x1, y1, x2, y2, x3, y3);
								currentAngle = abs(AngleBetween(x1, y1, x2, y2, x3, y3));
							}
							//move to the next edge
							s1 = s1->nxt();
						}
						//increment the cumulative angle so far
						angle += currentAngle;
						//move to the next edge
						lastFace = tempFace;
						tempFace = nextFace;
					}
					//in other cases, report an error
					else
					{
						sr_out.warning("ERROR: should only encounter degree-2 and degree-3 faces this way\n");
					}
				}
				//follow another chain
				s = s->nxt();
			}
		}
		//get the next component
		component++;
	}

//Degree-2 faces (in a ring)
	//go through the processing queue again
	for (int i = 0; i < processing.size(); i++)
	{
		//get the current triangle
		SeDcdtFace *currentFace = processing[i];
		//only continue if it hasn't been abstracted yet
		if (Degree(currentFace) != UNABSTRACTED)
		{
			continue;
		}
		//mark it as the first triangle in the ring
		SeDcdtFace *firstFace = currentFace;
		//the next triangle to visit
		SeDcdtFace *nextFace = NULL;
		//follow the ring of unabstracted faces
		while (true)
		{
			//access the elements of the current triangle
			SeBase *s = currentFace->se();
			//go through its edges
			for (int i = 0; i < 3; i++)
			{
				//record the degree of the triangle across this edge
				int degree = Degree(s->sym());
				//if there's a tree component off this face, collapse it
				if (!Blocked(s) && (degree == 1))
				{
					TreeCollapse(currentFace, (SeDcdtFace *)s->sym()->fac(), component);
				}
				//when the next face is found in the ring, record it
				if (!Blocked(s) && (degree == UNABSTRACTED))
				{
					nextFace = (SeDcdtFace *)s->sym()->fac();
				}
				//move to the next edge
				s = s->nxt();
			}
			//abstract the current face, calculate its widths, and set its connected component
			currentFace->link = new Abstraction(2);
			CalculateWidths(currentFace);
			currentFace->link->Component(component);
			//access the triangle's elements again
			s = currentFace->se();
			//go through the edges again
			for (int i = 0; i < 3; i++)
			{
				//if the adjacent triangle is part of the ring
				if (!Blocked(s) && Degree(s->sym()) != 1)
				{
					//set the angle and choke values accordingly
                    currentFace->link->Angle(i, INFINITY);
					currentFace->link->Choke(i, INFINITY);
				}
				//move to the next edge
				s = s->nxt();
			}
			//if the start of the ring has been reencountered, stop following it
			if (currentFace == nextFace)
			{
				break;
			}
			//move to the next face in the ring
			currentFace = nextFace;
		}
		//move to the next ring (connected component)
		component++;
	}
#if defined EXPERIMENT
	//output the number of degree-3 nodes to the output file
	std::ofstream outFile("Data.txt", std::ios_base::app);
	outFile << "Degree3: " << num << "\t";
	outFile.close();
#endif
}

//Abstract into a tree
void SeDcdt::TreeAbstract(SeDcdtFace *first, int component)
{
	//create a stack of the nodes in the tree
	SrArray<SeDcdtFace *> tree;
	//put the initial node on the tree
	tree.push() = first;
	//continue until the tree can't be expanded anymore
	while (!tree.empty())
	{
		//get the next face off of the stack
		SeDcdtFace *currentFace = tree.pop();
		//get the elements of this triangle
		SeBase *s = currentFace->se();
		//go through its edges
		for (int i = 0; i < 3; i++)
		{
			//ignore triangles across contrained edges
			if (Blocked(s))
			{
				s = s->nxt();
				continue;
			}
			//gets the face across that edge
			SeDcdtFace *tempFace = (SeDcdtFace *)s->sym()->fac();
			//if the adjacent triangle hasn't been dealt with, put it on the stack
			if ((tempFace->link->Angle(0) == INVALID) && (tempFace->link->Angle(1) == INVALID)
				&& (tempFace->link->Angle(2) == INVALID))
			{
				tree.push() = tempFace;
			}
			//move to the next edge
			s = s->nxt();
		}
		//abstract the triangle if it hasn't already been
		if (Degree(currentFace) == UNABSTRACTED)
		{
			//if the triangle hasn't been abstracted yet, abstract it and calculate its widths
			currentFace->link = new Abstraction(1);
			CalculateWidths(currentFace);
		}
		//access the triangle's elements again
		s = currentFace->se();
		//move through its edges
		for (int i = 0; i < 3; i++)
		{
			//for each unconstrained edge,
			if (!Blocked(s))
			{
				//set the angle and choke values accordingly
                currentFace->link->Angle(i, INFINITY);
				currentFace->link->Choke(i, INFINITY);
			}
			//move to the next edge
			s = s->nxt();
		}
		//set the triangle's connected component
		currentFace->link->Component(component);
	}
}

//Collapse a tree onto a degree-2 node
void SeDcdt::TreeCollapse(SeDcdtFace *root, SeDcdtFace *currentFace, int component)
{
	//mark edges we have dealt with
	_mesh->begin_marking();
	//starting with the root of the tree
	_mesh->mark(root);
	//create a stack of triangles to be processed
	SrArray<SeDcdtFace *> tree;
	//and a parallel one for angles of those triangles
	SrArray<float> angles;
	//put the current triangle on the stack to be processed
	tree.push() = currentFace;
	//and record that it is at angle 0 from the root
	angles.push() = 0.0f;
	//continue until all faces in the tree have been dealt with
	while (!tree.empty())
	{
		//get the top face off the stack
		currentFace = tree.pop();
		//set its connected component
		currentFace->link->Component(component);
		//mark this face as dealt with
		_mesh->mark(currentFace);
		//access this face's elements
		SeBase *s = currentFace->se();
		//the angles to the other adjacent triangles
		float angle[2];

		int i;
		//go through the edges
		for (i = 0; i < 3; i++)
		{
			//the face across that edge
			SeDcdtFace *tempFace = (SeDcdtFace *)s->sym()->fac();
			//if this is the entry edge,
			if (!Blocked(s) && _mesh->marked(tempFace))
			{
				//set the angle
				currentFace->link->Angle(i, angles.pop());
				//calculate the angle to the other adjacent triangles
				float x1, y1, x2, y2, x3, y3;
				TriangleVertices(s, x1, y1, x2, y2, x3, y3);
				angle[0] = abs(AngleBetween(x1, y1, x2, y2, x3, y3)) + currentFace->link->Angle(i);
				angle[1] = abs(AngleBetween(x2, y2, x3, y3, x1, y1)) + currentFace->link->Angle(i);
				//set the adjacent triangle in that direction to the root
				currentFace->link->Adjacent(i, root);
				//if this is the triangle next to the root,
				if (tempFace == root)
				{
					//the choke value is the length of the edge between them
					currentFace->link->Choke(i, Length(x1, y1, x2, y2));
				}
				//if there are triangles between this and the root,
				else
				{
					//get the elements of the current triangle
					SeBase *s1 = tempFace->se();
					float width, choke;
					//go through its edges
					for (int j = 0; j < 3; j++)
					{
						//get the choke value in that direction
						float temp = tempFace->link->Choke(j);
						//if it's a valid value,
						if ((temp != INVALID) && (temp != INFINITY))
						{
							//record it
							choke = temp;
							//gets the width through that triangle between the root and current triangles
							if (s1->nxt()->sym()->fac() == currentFace)
							{
								width = tempFace->link->Width(j);
							}
							else
							{
								width = tempFace->link->Width((j + 2) % 3);
							}
							break;
						}
						//move to the next edge
						s1 = s1->nxt();
					}
					//the choke value of this triangle is the lesser of these values
					currentFace->link->Choke(i, Minimum(width, choke));
				}
				break;
			}
			//move to the next edge
			s = s->nxt();
		}
		//record where we left off
		int base = i + 1;
		//visit the other two adjacent triangles
		for (int i = 0; i < 2; i++)
		{
			//move to the next edge
			s = s->nxt();
			//only deal with ones not across a constrained edge
			if (Blocked(s))
			{
				continue;
			}
			//set the angle and choke values acoordingly
			currentFace->link->Angle((i + base) % 3, INFINITY);
			currentFace->link->Choke((i + base) % 3, INFINITY);
			//put them on the tree for processing
			tree.push() = (SeDcdtFace *)s->sym()->fac();
			//also put the appropriate angle on the other tree
			angles.push() = angle[i];
		}
	}
	//finished collapsing the tree
	_mesh->end_marking();
}

//deletes the information from the abstraction
void SeDcdt::DeleteAbstraction()
{
	//get the first face in the mesh
	SeDcdtFace *currentFace = _mesh->first()->fac();
	//and record it
	SeDcdtFace *firstFace = currentFace;
	//go through the faces
	do
	{
		//if the triangle is abstracted,
		if (currentFace->link != NULL)
		{
			//free the abstraction information
			delete currentFace->link;
			currentFace->link = NULL;
		}
		//move to the next triangle
		currentFace = currentFace->nxt();
	}
	//until reaching the first face again
	while (currentFace != firstFace);
	//mark the outside face as being invalid
	outside = NULL;
}

//Abstract function definitions }
