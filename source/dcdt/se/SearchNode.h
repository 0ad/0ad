//SearchNode.h

//DJD: definition of the SearchNode class {

#ifndef SEARCHNODE_H
#define SEARCHNODE_H

#include "se_dcdt.h"

//class used to store a state in the search functions
class SearchNode
{
protected:
	//the (minimum) distance travelled since the start node
	float m_dMinDistance;
	//the (admissible and consistent) estimate of the distance to the goal
	float m_dHeuristic;
	//the closest point in this triangle to the goal
	SrPnt2 m_CClosestPoint;
	//the triangle (face) associated with this node
	SeDcdtFace *m_CTriangle;
	//the parent search node of this one
	SearchNode *m_CBack;
	//the number of open children
	int m_nOpenChildren;
	//the direction taken from the last node to this one
	int m_nDirection;
public:
	//constructor - initializes the member variables
	SearchNode(float dMinDistance, float dHeuristic, SrPnt2 CClosestPoint, SeDcdtFace *CTriangle, SearchNode *CBack, int nDirection = INVALID)
	{
		m_dMinDistance = dMinDistance;
		m_dHeuristic = dHeuristic;
		m_CClosestPoint.x = CClosestPoint.x;
		m_CClosestPoint.y = CClosestPoint.y;
		m_CTriangle = CTriangle;
		m_CBack = CBack;
		m_nOpenChildren = 0;
		m_nDirection = nDirection;
	}

	//returns the estimated minimum cost of a path from the start to the goal through this node
	float f()
	{
		return (m_dMinDistance + m_dHeuristic);
	}

	//returns the estimated minimum cost of a path from the start to this node
	float g()
	{
		return m_dMinDistance;
	}

	//returns the estimated minimum cost of a path from this node to the goal
	float h()
	{
		return m_dHeuristic;
	}

	//returns (a copy of) the closest point in this triangle to the goal
	SrPnt2 Point()
	{
		SrPnt2 temp;
		temp.x = m_CClosestPoint.x;
		temp.y = m_CClosestPoint.y;
		return temp;
	}

	//returns the triangle associated with this node
	SeDcdtFace *Triangle()
	{
		return m_CTriangle;
	}

	//returns the parent search node of this one
	SearchNode *Back()
	{
		return m_CBack;
	}

	//returns the direction taken from the last node to this one
	int Direction()
	{
		return m_nDirection;
	}

	//sets the number of open children
	void OpenChild()
	{
		m_nOpenChildren++;
	}

	//closes a child
	bool CloseChild()
	{
		return ((--m_nOpenChildren) <= 0);
	}

	//closes the current search node
	//(deleting its parent if it has no other open children)
	void Close()
	{
		if ((m_CBack != NULL) && m_CBack->CloseChild())
		{
			m_CBack->Close();
			delete m_CBack;
			m_CBack = NULL;
		}
	}

	//returns the number of open children this node has
	int OpenChildren()
	{
		return m_nOpenChildren;
	}

	//checks the ancestors of this node to see if a given triangle was already searched
	bool Searched(SeDcdtFace *triangle)
	{
		SearchNode *current = m_CBack;
		while (current != NULL)
		{
			if (current->Triangle() == triangle)
			{
				return true;
			}
			current = current->Back();
		}
		return false;
	}
};

#endif

//definition of the SearchNode class }
