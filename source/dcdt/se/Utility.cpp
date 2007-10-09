//Utility.cpp

//DJD: Helper function definitions {

#include "precompiled.h"

#include <math.h>

#include "se_dcdt.h"

//degree value for an unabstracted triangle
#define UNABSTRACTED -1

//the list of unprocessed triangles
template <class T>
SrArray<SeLinkFace<T> *> SeLinkFace<T>::processing;

//gets the vertex coordinates and whether or not the edges are blocked, for a face
void SeDcdt::TriangleInfo(SeDcdtFace *face, float &x1, float &y1, bool &e1,
						  float &x2, float &y2, bool &e2, float &x3, float &y3, bool &e3)
{
	TriangleInfo(face->se(), x1, y1, e1, x2, y2, e2, x3, y3, e3);
}

//gets the vertex coordinates and whether or not the edges are blocked, in a certain order
void SeDcdt::TriangleInfo(SeBase *s, float &x1, float &y1, bool &e1,
						  float &x2, float &y2, bool &e2, float &x3, float &y3, bool &e3)
{
	TriangleVertices(s, x1, y1, x2, y2, x3, y3);
	TriangleEdges(s, e1, e2, e3);
}

//gets the vertex coordinates of a triangle, for a face
void SeDcdt::TriangleVertices(SeDcdtFace *face, float &x1, float &y1, 
							  float &x2, float &y2, float &x3, float &y3)
{
	TriangleVertices(face->se(), x1, y1, x2, y2, x3, y3);
}

//gets the vertex coordinates of a triangle, in a certain order
void SeDcdt::TriangleVertices(SeBase *s, float &x1, float &y1, 
							  float &x2, float &y2, float &x3, float &y3)
{
	x1 = ((SeDcdtVertex *)s->vtx())->p.x;
	y1 = ((SeDcdtVertex *)s->vtx())->p.y;
	s = s->nxt();
	x2 = ((SeDcdtVertex *)s->vtx())->p.x;
	y2 = ((SeDcdtVertex *)s->vtx())->p.y;
	s = s->nxt();
	x3 = ((SeDcdtVertex *)s->vtx())->p.x;
	y3 = ((SeDcdtVertex *)s->vtx())->p.y;
}

//gets whether or not the edges of a triangle are blocked, for a face
void SeDcdt::TriangleEdges(SeDcdtFace *face, bool &e1, bool &e2, bool &e3)
{
	TriangleEdges(face->se(), e1, e2, e3);
}

//gets whether or not the edges of a triangle are blocked, in a certain order
void SeDcdt::TriangleEdges(SeBase *s, bool &e1, bool &e2, bool &e3)
{
	e1 = Blocked(s);
	s = s->nxt();
	e2 = Blocked(s);
	s = s->nxt();
	e3 = Blocked(s);
}

//gets the midpoint of a triangle
void SeDcdt::TriangleMidpoint(SeDcdtFace *face, float &x, float &y)
{
	float x1, y1, x2, y2, x3, y3;
	TriangleVertices(face, x1, y1, x2, y2, x3, y3);
	x = (x1 + x2 + x3) / 3.0f;
	y = (y1 + y2 + y3) / 3.0f;
}

//gets the midpoint of a triangle
SrPnt2 SeDcdt::TriangleMidpoint(SeDcdtFace *face)
{
	float x1, y1, x2, y2, x3, y3;
	TriangleVertices(face, x1, y1, x2, y2, x3, y3);
	SrPnt2 p;
	p.x = (x1 + x2 + x3) / 3.0f;
	p.y = (y1 + y2 + y3) / 3.0f;
	return p;
}

//gets whether or not an edge is constrained or bordering the outside face
bool SeDcdt::Blocked(SeBase *s)
{
	return (((SeDcdtEdge *)s->edg())->is_constrained() || (s->sym()->fac() == outside));
}

//gets the degree of node a face has been abstracted to
int SeDcdt::Degree(SeBase *s)
{
	return Degree(s->fac());
}

//gets the degree of node a face has been abstracted to
int SeDcdt::Degree(SeFace *face)
{
	if (((SeDcdtFace *)face)->link == NULL)
	{
		return UNABSTRACTED;
	}
	return ((SeDcdtFace *)face)->link->Degree();
}

//gets the angle between the given vertex coordinates
float SeDcdt::AngleBetween(float x1, float y1, float x2, float y2, float x3, float y3)
{
	float angle1 = (float)atan2(y1 - y2, x1 - x2);
	float angle2 = (float)atan2(y3 - y2, x3 - x2);
	float diff = angle1 - angle2;
	diff += (diff <= -PI) ? 2.0f * PI : (diff > PI) ? -2.0f * PI : 0.0f;
	return diff;
}

//gets the orientation of the vertices with coordinates given
//(0 for collinear, <0 for clockwise, >0 for counterclockwise)
float SeDcdt::Orientation(float x1, float y1, float x2, float y2, float x3, float y3)
{
	return ((x1 * y2) + (x2 * y3) + (x3 * y1) - (x1 * y3) - (x2 * y1) - (x3 * y2));
}

//determines if an angle is accute
bool SeDcdt::IsAccute(float theta)
{
	return (abs(theta) < (PI / 2.0f));
}

//determines if an angle is obtuse
bool SeDcdt::IsObtuse(float theta)
{
	return (abs(theta) > (PI / 2.0f));
}

//calculates the distance between two points
float SeDcdt::Length(float x1, float y1, float x2, float y2)
{
	float xDiff = x1 - x2;
	float yDiff = y1 - y2;
	return (sqrt(xDiff * xDiff + yDiff * yDiff));
}

//returns the smaller of the two arguments
float SeDcdt::Minimum(float a, float b)
{
	return ((a < b) ? a : b);
}

//returns the larger of the two arguments
float SeDcdt::Maximum(float a, float b)
{
	return ((a > b) ? a : b);
}

//returns the minimum distance between the two segments given
float SeDcdt::SegmentDistance(float A1x, float A1y, float A2x, float A2y,
							  float B1x, float B1y, float B2x, float B2y)
{
	float d1 = PointSegmentDistance(A1x, A1y, B1x, B1y, B2x, B2y);
	float d2 = PointSegmentDistance(A2x, A2y, B1x, B1y, B2x, B2y);
	float d3 = PointSegmentDistance(B1x, B1y, A1x, A1y, A2x, A2y);
	float d4 = PointSegmentDistance(B2x, B2y, A1x, A1y, A2x, A2y);
	float min = Minimum(Minimum(d1, d2), Minimum(d3, d4));
	return min;
}

//returns the minimum distance between the two segments given, at least r from the endpoints
float SeDcdt::SegmentDistance(float ls1x, float ls1y, float ls2x, float ls2y,
							  float ll1x, float ll1y, float ll2x, float ll2y, float r)
{
	float theta = atan2(ls2y - ls1y, ls2x - ls1x);
	float A1x = ls1x + cos(theta) * r;
	float A1y = ls1y + sin(theta) * r;
	float A2x = ls2x - cos(theta) * r;
	float A2y = ls2y - sin(theta) * r;
	theta = atan2(ll2y - ll1y, ll2x - ll1x);
	float B1x = ll1x + cos(theta) * r;
	float B1y = ll1y + sin(theta) * r;
	float B2x = ll2x - cos(theta) * r;
	float B2y = ll2y - sin(theta) * r;
	return SegmentDistance(A1x, A1y, A2x, A2y, B1x, B1y, B2x, B2y);
}

//returns the minimum distance between the point and the line segment given
float SeDcdt::PointSegmentDistance(float x, float y, float x1, float y1, float x2, float y2)
{
	if (!IsAccute(AngleBetween(x1, y1, x2, y2, x, y)))
	{
		return Length(x2, y2, x, y);
	}
	else if (!IsAccute(AngleBetween(x2, y2, x1, y1, x, y)))
	{
		return Length(x1, y1, x, y);
	}
	else
	{
		return PointLineDistance(x, y, x1, y1, x2, y2);
	}
}

//returns the closest point on the segment given, at least r in from the endpoints, to the point given
SrPnt2 SeDcdt::ClosestPointOn(float x1, float y1, float x2, float y2, float x, float y, float r)
{
	SrPnt2 point;
	float distance;
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
	return point;
}

//Helper function definitions }
