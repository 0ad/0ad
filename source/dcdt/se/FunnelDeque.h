//FunnelDeque.h

//DJD: definition of FunnelNode class {

#ifndef FUNNELDEQUE_H
#define FUNNELDEQUE_H

#include "sr_vec2.h"
#include "sr_polygon.h"

//define a floating point value for pi
#ifndef PI
#define PI 3.1415926535897932384626433832795f
#endif

//a node in the funnel deque
class FunnelNode
{
protected:
	//left and right pointers (doubly-connected)
	FunnelNode *m_CLeft;
	FunnelNode *m_CRight;
	//the vertex represented by this funnel node
	SrPnt2 m_CPoint;
public:
	//default constructor
	FunnelNode();
	//constructor - initializes the vertex coordinates
	FunnelNode(float x, float y);
	//constructor - initializes the vertex
	FunnelNode(const SrPnt2& point);
	//mutator for the left pointer
	void Left(FunnelNode *left);
	//mutator for the right pointer
	void Right(FunnelNode *right);
	//accessor for the left pointer
	FunnelNode *Left();
	//accessor for the right pointer
	FunnelNode *Right();
	//mutator for the vertex
	void Point(const SrPnt2& point);
	//accessor for the vertex
	SrPnt2 Point();
	//mutator for the vertex coordinates
	void Point(float x, float y);
	//accessor for the vertex x coordinate
	float X();
	//accessor for the vertex y coordinate
	float Y();
};

//definition of FunnelNode class }

//DJD: definition of FunnelDeque class {

//the funnel deque used in the funnel algorithm
class FunnelDeque
{
public:
	//different kinds of corners - point, left and right tangent
	//(tangent to a circle around the corner, running along the side specified)
	enum CornerType {Point, LeftTangent, RightTangent};
protected:
	//left end, right end, and apex pointers for the deque
	FunnelNode *m_CLeft;
	FunnelNode *m_CRight;
	FunnelNode *m_CApex;
	//the corner type of the node at the apex
	//(ones on the left side are all right tangent, ones on the right are all left tangent)
	CornerType m_tApexType;
	//radius of the unit for which we are finding the path
	float m_dRadius;
	//angle between the first and second points given their corner types
	float Angle(float x_1, float y_1, CornerType type_1, float x_2, float y_2, CornerType type_2);
	//constrains an angfle to the range (-PI, PI]
	float AngleRange(float theta);
	//returns the (Euclidean) distance between two points
	float Distance(float x_1, float y_1, float x_2, float y_2);
	//returns the angle between the given points
	//(same as the angle to their tangents if they're the same type)
	float ToTangent(float x_1, float y_1, float x_2, float y_2);
	//returns the angle between tangents of the given points
	//given whether the first is left tangent type, assuming the second is the opposite
	float ToTangentAlt(float x_1, float y_1, float x_2, float y_2, bool LeftRight);
	//returns the angle between the first point and a tangent to the second
	//(the type of the second is specified)
	float PointToTangent(float x_1, float y_1, float x_2, float y_2, bool Right);
	//adds a given point to the path
	void AddPoint(float x, float y, CornerType type, float angle, SrPolygon &path);
public:
	//creates the funnel deque based on a starting point and radius
	FunnelDeque(float x, float y, float r);
	//copy constructor
	FunnelDeque(const SrPnt2& point, float r);
	//destructor
	~FunnelDeque();
	//adds a given point of a given type to the funnel deque, adding to the path if necessary
	void Add(const SrPnt2& p, CornerType type, SrPolygon& path);
	//prints the contents of the funnel deque
	void Print();
	//prints the contents of the path
	void Print(SrPolygon path);
	//calculates the length of the path for a unit of given radius
	float Length(SrPolygon path);
};

#endif

//definition of FunnelDeque class }
