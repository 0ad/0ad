//Abstraction.h

//DJD: definition of Abstraction class {

#ifndef ABSTRACTION_H
#define ABSTRACTION_H

#include <float.h>
#include "FunnelDeque.h"

//defines an invalid entry, and "infinity", for our purposes
#define INVALID -1
#define INFINITY FLT_MAX

//prototypes for classes defined later
template <class T> class SeLinkFace;
class Abstraction;
typedef SeLinkFace<Abstraction> SeDcdtFace;

//class which contains information about our topological abstraction
class Abstraction
{
protected:
	//list of adjacent nodes
	//if this node is degree-1 in a tree, all will be NULL
	//if it is degree-1 otherwise, one will be the root of the tree, the others will be NULL
	//if it is degree-2 in a ring, all will be NULL
	//if it is degree-2 otherwise, two will be degree-3 nodes on the ends of the corridor, the other will be NULL
	//if it is degree-3, all will be degree-3 nodes on the end of the corridors
	SeDcdtFace *m_CAdjacent[3];
	//sum of interior angles of triangles between this one and the corresponding adjacent one
	//INVALID if the corresponding adjacent node is NULL
	float m_dAngle[3];
	//width between the corresponding edge and the one counterclockwise to it, through the triangle
	float m_dWidth[3];
	//smallest width through the triangles between this one and the corresponding adjacent one
	//INVALID if the corresponding adjacent node is NULL
	float m_dChoke[3];
	//degree of this node
	int m_nDegree;
	//connected component to which this triangle belongs
	int m_nComponent;

	//makes sure the index is in the range [0, 2]
	bool CheckBounds(int n)
	{
		return ((n >= 0) && (n < 3));
	}
public:
	//creates the abstraction with given degree
	Abstraction(int nDegree)
	{
		//starts all adjacents as NULL and other information as INVALID
		for (int i = 0; i < 3; i++)
		{
			m_CAdjacent[i] = NULL;
			m_dAngle[i] = INVALID;
			m_dWidth[i] = INVALID;
			m_dChoke[i] = INVALID;
		}
		m_nDegree = nDegree;
		m_nComponent = INVALID;
	}

	//default destructor
	~Abstraction()
	{
	}

	//accessor for the degree of the node
	int Degree()
	{
		return m_nDegree;
	}

	//mutator for the degree of the node
	void Degree(int nDegree)
	{
		m_nDegree = nDegree;
	}

	//accessor for the connected component of the node
	int Component()
	{
		return m_nComponent;
	}

	//mutator for the connected component of the node
	void Component(int nComponent)
	{
		m_nComponent = nComponent;
	}

	//accessor for the adjacent node with index given
	SeDcdtFace *Adjacent(int n)
	{
		return (CheckBounds(n)) ? m_CAdjacent[n] : NULL;
	}

	//mutator for the adjacent node with index given
	void Adjacent(int n, SeDcdtFace *CAdjacent)
	{
		if (CheckBounds(n))
		{
			m_CAdjacent[n] = CAdjacent;
		}
	}

	//accessor for the angle with index given
	float Angle(int n)
	{
		return (CheckBounds(n)) ? m_dAngle[n] : INVALID;
	}

	//mutator for the angle with index given
	void Angle(int n, float dAngle)
	{
		if (CheckBounds(n))
		{
			m_dAngle[n] = dAngle;
		}
	}

	//accessor for the width with index given
	float Width(int n)
	{
		return (CheckBounds(n)) ? m_dWidth[n] : INVALID;
	}

	//mutator for the width with index given
	void Width(int n, float dWidth)
	{
		if (CheckBounds(n))
		{
			m_dWidth[n] = dWidth;
		}
	}

	//accessor for the choke point width with index given
	float Choke(int n)
	{
		return (CheckBounds(n)) ? m_dChoke[n] : INVALID;
	}

	//mutator for the choke point width with index given
	void Choke(int n, float dChoke)
	{
		if (CheckBounds(n))
		{
			m_dChoke[n] = dChoke;
		}
	}
};

#endif

//definition of Abstraction class }
