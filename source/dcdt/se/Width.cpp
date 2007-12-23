//Width.cpp

//DJD: Width function definitions {
#include "precompiled.h"
#include "0ad_warning_disable.h"

#include "se_dcdt.h"
#include <math.h>

//list of unprocessed triangles
template <class T>
SrArray<SeLinkFace<T> *> SeLinkFace<T>::processing;

//determines the distance between a point and the closest point to it on a line
float SeDcdt::PointLineDistance(float x, float y, float x1, float y1, float x2, float y2)
{
	if (x1 == x2)
	{
		return abs(x - x1);
	}
	float rise = y2 - y1;
	float run = x2 - x1;
	float intercept = y1 - (rise / run) * x1;
	float a = rise;
	float b = -run;
	float c = run * intercept;
	return (abs(a * x + b * y + c) / sqrt(a * a + b * b));
}

//determines if an edge should be considered when calculating the width
//(if the base angles of the triangle formed by this edge joined with the "origin" point are both accute)
bool SeDcdt::Consider(float x, float y, float x1, float y1, float x2, float y2)
{
	return ((IsAccute(AngleBetween(x, y, x1, y1, x2, y2)))
		&& (IsAccute(AngleBetween(x, y, x2, y2, x1, y1))));
}

//Calculates the meaningful widths of a triangle
void SeDcdt::CalculateWidths(SeDcdtFace *face)
{
	//if the triangle hasn't been abstracted yet, return
	if (face->link == NULL)
	{
		return;
	}
	//goes through the vertices of the triangle
	SeBase *s = face->se();
	for (int i = 0; i < 3; i++)
	{
		//calculate and set the width between those edges
		face->link->Width(i, TriangleWidth(s->nxt()));
		s = s->nxt();
	}
	float x1, y1, x2, y2, x3, y3;
	TriangleVertices(face, x1, y1, x2, y2, x3, y3);
}

//determine one width through a triangle
float SeDcdt::TriangleWidth(SeBase *s)
{
	//get the coordinates of the triangle
	float x, y, x1, y1, x2, y2;
	TriangleVertices(s, x, y, x1, y1, x2, y2);
	//if either base angle isn't accute, the width of that
	//triangle is the length of the shorter edge
	if (!IsAccute(AngleBetween(x, y, x1, y1, x2, y2)))
	{
		return Length(x, y, x1, y1);
	}
	else if (!IsAccute(AngleBetween(x, y, x2, y2, x1, y1)))
	{
		return Length(x, y, x2, y2);
	}
	else
	{
		//otherwise, calculate the upper bound on the width of the triangle
		float CurrentWidth = Minimum(Length(x, y, x1, y1), Length(x, y, x2, y2));
		//and calculate the actual value of the width
		return SearchWidth(x, y, s->nxt()->sym(), CurrentWidth);
	}
}

//checks the current triangle for bounds on the triangle width
float SeDcdt::SearchWidth(float x, float y, SeBase *s, float CurrentWidth)
{
	//get the coordinates of the triangle
	float x1, y1, x2, y2, x3, y3;
	TriangleVertices(s, x1, y1, x2, y2, x3, y3);
	//checks if the entrance edge should be considered
	if (!Consider(x, y, x1, y1, x2, y2))
	{
		//if not, returns the width unmodified
		return CurrentWidth;
	}
	//calculates the distance between this edge and the "origin" point
	float Distance = PointLineDistance(x, y, x1, y1, x2, y2);
	//if it's farther than the upper bound
	if (Distance >= CurrentWidth)
	{
		return CurrentWidth;
	}
	//if the edge is constrained,
	else if (Blocked(s->sym()))
	{
		//return this distance as the new upper bound
		return Distance;
	}
	//gets the distance to the vertex opposite the entrance edge
	Distance = Length(x, y, x3, y3);
	//if this distance is less than the current upper bound,
	if (Distance <= CurrentWidth)
	{
		//return this distance as the new upper bound
		return Distance;
	}
	//otherwise, searches across the other two edges for bounds
	CurrentWidth = SearchWidth(x, y, s->nxt()->sym(), CurrentWidth);
	return SearchWidth(x, y, s->nxt()->nxt()->sym(), CurrentWidth);
}

//determines if a certain point (x, y) in face is at least distance r from any constraints
bool SeDcdt::ValidPosition(float x, float y, SeDcdtFace *face, float r)
{
	//get the triangle's vertices
	float x1, y1, x2, y2, x3, y3;
	TriangleVertices(face, x1, y1, x2, y2, x3, y3);
	//check if any are closer than r to (x, y)
	if ((Length(x, y, x1, y1) < r) || (Length(x, y, x2, y2) < r) || (Length(x, y, x3, y3) < r))
	{
		return false;
	}
	//get the edges of the triangle
	SeBase *s = face->se();
	//go through them one-by-one
	for (int i = 0; i < 3; i++)
	{
		//if there are any constraints within r of (x, y) across the current edge
		if (SearchWidth(x, y, s->sym(), r) < r)
		{
			//return that this in an invalid position
			return false;
		}
		//move to the next edge
		s = s->nxt();
	}
	//if no constraints were found within r of (x, y), it is a valid position
	return true;
}

//Width function definitions }
