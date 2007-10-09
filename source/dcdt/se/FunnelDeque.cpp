//FunnelDeque.cpp

//DJD: definition of FunnelNode functions {

#include "precompiled.h"

#include "FunnelDeque.h"
#include <float.h>
#include <math.h>

//default constructor - initializes all
//member variables to default values
FunnelNode::FunnelNode()
{
	m_CLeft = NULL;
	m_CRight = NULL;
	m_CPoint.set(FLT_MIN, FLT_MIN);
}

//constructor - initializes the point member
//variable to the coordinates passed
FunnelNode::FunnelNode(float x, float y)
{
	m_CLeft = NULL;
	m_CRight = NULL;
	m_CPoint.set(x, y);
}

//constructor - initializes the point member
//variable to the one passed
FunnelNode::FunnelNode(const SrPnt2& point)
{
	m_CLeft = NULL;
	m_CRight = NULL;
	m_CPoint.set(point.x, point.y);
}

//mutator for the left pointer
void FunnelNode::Left(FunnelNode *left)
{
	m_CLeft = left;
}

//mutator for the right pointer
void FunnelNode::Right(FunnelNode *right)
{
	m_CRight = right;
}

//accessor for the left pointer
FunnelNode *FunnelNode::Left()
{
	return m_CLeft;
}

//accessor for the right pointer
FunnelNode *FunnelNode::Right()
{
	return m_CRight;
}

//mutator for the point
void FunnelNode::Point(const SrPnt2& point)
{
	m_CPoint.set(point.x, point.y);
}

//accessor for the point
SrPnt2 FunnelNode::Point()
{
	SrPnt2 copy;
	copy.set(m_CPoint.x, m_CPoint.y);
	return copy;
}

//mutator for the point's coordinates
void FunnelNode::Point(float x, float y)
{
	m_CPoint.set(x, y);
}

//accessor for the point's X coordinate
float FunnelNode::X()
{
	return m_CPoint.x;
}

//accessor for the point's Y coordinate
float FunnelNode::Y()
{
	return m_CPoint.y;
}

//definition of FunnelNode functions }

//DJD: definition of FunnelDeque functions {

//constructor - initializes the apex of the funnel
//to the point passed, and the unit radius
FunnelDeque::FunnelDeque(float x, float y, float r)
{
	m_CApex = new FunnelNode(x, y);
	m_CLeft = m_CApex;
	m_CRight = m_CApex;
	m_tApexType = Point;
	m_dRadius = r;
}

//constructor - initializes the apex of the funnel
//to the coordinates passed, and the unit radius
FunnelDeque::FunnelDeque(const SrPnt2& point, float r)
{
	m_CApex = new FunnelNode(point);
	m_CLeft = m_CApex;
	m_CRight = m_CApex;
	m_tApexType = Point;
	m_dRadius = r;
}

//destructor - deletes any funnel nodes in the deque
FunnelDeque::~FunnelDeque()
{
	FunnelNode *current = m_CLeft;
	while (current != NULL)
	{
		FunnelNode *temp = current;
		current = current->Right();
		delete temp;
	}
}

//Adds a new point to the funnel, adds to the existing path if the apex moves
//type = RightTangent => point is being added on left side
//type = LeftTangent => point is being added on right side
//type = Point => point is the end of the channel
void FunnelDeque::Add(const SrPnt2& p, CornerType type, SrPolygon& path)
{
	//the new funnel node containing the new point
	FunnelNode *next = new FunnelNode(p);
	//if it's being added on the left side
	//(operations are just reversed for the other side)
	if (type == RightTangent)
	{
		//loop until the point is added to the funnel
		while (true)
		{
			//if the apex is the only point in the funnel,
			//simply add the point to the appropriate side
			if (m_CLeft == m_CRight)
			{
				next->Right(m_CApex);
				m_CApex->Left(next);
				m_CLeft = next;
				break;
			}
			float x_1, y_1, x_2, y_2;
			CornerType type_1, type_2;
			//if there are no points on the left side of the funnel,
			//get the wedge going to the opposite side of the apex
			if (m_CLeft == m_CApex)
			{
				x_1 = m_CLeft->X();
				y_1 = m_CLeft->Y();
				x_2 = m_CLeft->Right()->X();
				y_2 = m_CLeft->Right()->Y();
				type_1 = m_tApexType;
				type_2 = LeftTangent;
			}
			//otherwise get the wedge on this side of the apex
			else
			{
				x_2 = m_CLeft->X();
				y_2 = m_CLeft->Y();
				x_1 = m_CLeft->Right()->X();
				y_1 = m_CLeft->Right()->Y();
				type_2 = RightTangent;
				if (m_CLeft->Right() == m_CApex)
				{
					type_1 = m_tApexType;
				}
				else
				{
					type_1 = RightTangent;
				}
			}
			//calculate the angle associated with this wedge
			float wedge = Angle(x_1, y_1, type_1, x_2, y_2, type_2);
			//also calculate the distance between these two points
			float length1 = Distance(x_1, y_1, x_2, y_2);
			//and that to the new point (special case for the modofied funnel algorithm)
			float length2 = Distance(x_1, y_1, p.x, p.y);
			//now get the wedge leading to the new point
			x_1 = m_CLeft->X();
			y_1 = m_CLeft->Y();
			if (m_CApex == m_CLeft)
			{
				type_1 = m_tApexType;
			}
			else
			{
				type_1 = RightTangent;
			}
			x_2 = p.x;
			y_2 = p.y;
			type_2 = type;
			//and the angle associated with it
			float toPoint = Angle(x_1, y_1, type_1, x_2, y_2, type_2);
			//compare the two angles
			float diff = wedge - toPoint;
			diff += (diff <= -PI) ? 2.0f * PI : (diff > PI) ? -2.0f * PI : 0.0f;
			//if the existing wedge is counterclockwise of that to the point
			if (diff < 0.0f)
			{
				//add the new point on this end of the funnel
				m_CLeft->Left(next);
				next->Right(m_CLeft);
				m_CLeft = next;
				break;
			}
			//if it is clockwise,
			else
			{
				//check if we are popping the apex
				if (m_CLeft == m_CApex)
				{
					//if the point being added is closer than the one being popped,
					if (length2 < length1)
					{
						//make the new point the apex instead of the one on the opposite side
						//(special case for the modified funnel algorithm)
						float angle = Angle(m_CApex->X(), m_CApex->Y(), m_tApexType, p.x, p.y, type);
						AddPoint(m_CApex->X(), m_CApex->Y(), m_tApexType, angle, path);
						AddPoint(p.x, p.y, type, angle, path);
						next->Right(m_CApex->Right());
						m_CApex->Right()->Left(next);
						delete m_CApex;
						m_CApex = next;
						m_CLeft = next;
						m_tApexType = type;
						break;
					}
					//otherwise,
					else
					{
						//move the apex to the right and add the segment between
						//the old and new apexes to the path so far
						float angle = Angle(m_CApex->X(), m_CApex->Y(), m_tApexType,
							m_CApex->Right()->X(), m_CApex->Right()->Y(), CornerType::LeftTangent);
						AddPoint(m_CApex->X(), m_CApex->Y(), m_tApexType, angle, path);
						AddPoint(m_CApex->Right()->X(), m_CApex->Right()->Y(), CornerType::LeftTangent, angle, path);
						m_CApex = m_CApex->Right();
						m_tApexType = LeftTangent;
					}
				}
				//pop the leftmost point off of the funnel
				FunnelNode *temp = m_CLeft;
				m_CLeft = m_CLeft->Right();
				m_CLeft->Left(NULL);
				delete temp;
			}
		}
	}
	//otherwise, if adding a point on the right side,
	//continue as before, with sides reversed
	//adding a point also ends up here - 
	//we do this to pop off all the necessary points off of
	//the right side of the funnel, so at the end we can just
	//add to the path the points between the apex and the
	//right side of the funnel (done at the end)
	else
	{
		while (true)
		{
			if (m_CLeft == m_CRight)
			{
				next->Left(m_CApex);
				m_CApex->Right(next);
				m_CRight = next;
				break;
			}
			float x_1, y_1, x_2, y_2;
			CornerType type_1, type_2;
			if (m_CRight == m_CApex)
			{
				x_1 = m_CRight->X();
				y_1 = m_CRight->Y();
				x_2 = m_CRight->Left()->X();
				y_2 = m_CRight->Left()->Y();
				type_1 = m_tApexType;
				type_2 = RightTangent;
			}
			else
			{
				x_2 = m_CRight->X();
				y_2 = m_CRight->Y();
				x_1 = m_CRight->Left()->X();
				y_1 = m_CRight->Left()->Y();
				type_2 = LeftTangent;
				if (m_CRight->Left() == m_CApex)
				{
					type_1 = m_tApexType;
				}
				else
				{
					type_1 = LeftTangent;
				}
			}
			float wedge = Angle(x_1, y_1, type_1, x_2, y_2, type_2);
			float length1 = Distance(x_1, y_1, x_2, y_2);
			float length2 = Distance(x_1, y_1, p.x, p.y);
			x_1 = m_CRight->X();
			y_1 = m_CRight->Y();
			if (m_CApex == m_CRight)
			{
				type_1 = m_tApexType;
			}
			else
			{
				type_1 = LeftTangent;
			}
			x_2 = p.x;
			y_2 = p.y;
			type_2 = type;
			float toPoint = Angle(x_1, y_1, type_1, x_2, y_2, type_2);
			float diff = wedge - toPoint;
			diff += (diff <= -PI) ? 2.0f * PI : (diff > PI) ? -2.0f * PI : 0.0f;
			if (diff < 0.0f)
			{
				if (m_CRight == m_CApex)
				{
					if (length2 < length1)
					{
						float angle = Angle(m_CApex->X(), m_CApex->Y(), m_tApexType, p.x, p.y, type);
						AddPoint(m_CApex->X(), m_CApex->Y(), m_tApexType, angle, path);
						AddPoint(p.x, p.y, type, angle, path);
						next->Left(m_CApex->Left());
						m_CApex->Left()->Right(next);
						delete m_CApex;
						m_CApex = next;
						m_CRight = next;
						m_tApexType = type;
						break;
					}
					else
					{
						float angle = Angle(m_CApex->X(), m_CApex->Y(), m_tApexType,
							m_CApex->Left()->X(), m_CApex->Left()->Y(), CornerType::RightTangent);
						AddPoint(m_CApex->X(), m_CApex->Y(), m_tApexType, angle, path);
						AddPoint(m_CApex->Left()->X(), m_CApex->Left()->Y(), CornerType::RightTangent, angle, path);
						m_CApex = m_CApex->Left();
						m_tApexType = RightTangent;
					}
				}
				FunnelNode *temp = m_CRight;
				m_CRight = m_CRight->Left();
				m_CRight->Right(NULL);
				delete temp;
			}
			else
			{
				m_CRight->Right(next);
				next->Left(m_CRight);
				m_CRight = next;
				break;
			}
		}
	}
	//this is where the points between the apex
	//and the right side o fthe funnel are added to the path
	//when adding the final point to the channel
	if (type == Point)
	{
		FunnelNode *Current = m_CApex;
		while (Current != m_CRight)
		{
			float x_1, y_1, x_2, y_2, toPoint;
			CornerType type_1, type_2;
			type_1 = (Current == m_CApex) ? m_tApexType : CornerType::LeftTangent;
			type_2 = (Current->Right() == m_CRight) ? CornerType::Point : CornerType::LeftTangent;
			x_1 = Current->X();
			y_1 = Current->Y();
			Current = Current->Right();
			x_2 = Current->X();
			y_2 = Current->Y();
			toPoint = Angle(x_1, y_1, type_1, x_2, y_2, type_2);
			AddPoint(x_1, y_1, type_1, toPoint, path);
			AddPoint(x_2, y_2, type_2, toPoint, path);
		}
	}
}

//returns the length of a path found by this funnel algorithm with the current radius
float FunnelDeque::Length(SrPolygon path)
{
	//only paths with an even number of points are valid
	if ((path.size() % 2) != 0)
	{
		return 0.0f;
	}
	float length = 0.0f;
	//go through the straight segments and total their lengths
	for (int i = 0; i < path.size(); i += 2)
	{
		float xDiff = path[i + 1].x - path[i].x;
		float yDiff = path[i + 1].y - path[i].y;
		length += sqrt(xDiff * xDiff + yDiff * yDiff);
	}
	//go through the curved segments and add their length
	//(the difference of the angles of the
	// surrounding segments, times the unit radius)
	for (int i = 1; i < path.size() - 3; i += 2)
	{
		float angle1 = atan2(path[i + 1].y - path[i].y, path[i + 1].x - path[i].x);
		float angle2 = atan2(path[i + 3].y - path[i + 2].y, path[i + 3].x - path[i + 2].x);
		float diff = angle1 - angle2;
		diff += (diff <= -PI) ? 2.0f * PI : (diff > PI) ? -2.0f * PI : 0.0f;
		length += m_dRadius * abs(diff);
	}
	return length;
}

//returns the angle from x_1, y_1 to x_2, y_2, given the types
float FunnelDeque::Angle(float x_1, float y_1, CornerType type_1, float x_2, float y_2, CornerType type_2)
{
	//simply calls the appropriate function based
	//on the types of the two points
	if (type_1 == Point)
	{
		return PointToTangent(x_1, y_1, x_2, y_2, (type_2 == RightTangent));
	}
	else if (type_2 == Point)
	{
		float angle = PointToTangent(x_2, y_2, x_1, y_1, (type_1 == LeftTangent));
		return AngleRange(angle + PI);
	}
	else if (type_1 != type_2)
	{
		return ToTangentAlt(x_1, y_1, x_2, y_2, (type_1 == LeftTangent));
	}
	else
	{
		return ToTangent(x_1, y_1, x_2, y_2);
	}
}

//adds a point to the path given a coordinate, its type,
//and the angle of the segment attached
void FunnelDeque::AddPoint(float x, float y, CornerType type, float angle, SrPolygon &path)
{
	//if it's a point type, just add it unchanged
	if (type == Point)
	{
		path.push().set(x, y);
	}
	//otherwise,
	else
	{
		//move the unit radius from the point to one side or the other
		float theta = AngleRange(angle + PI / ((type == LeftTangent) ? 2.0f : -2.0f));
		path.push().set(x + cos(theta) * m_dRadius, y + sin(theta) * m_dRadius);
	}
}

//takes an angle value and restricts it to the range (-PI, PI]
float FunnelDeque::AngleRange(float theta)
{
	return ((theta > PI) ? (theta - 2.0f * PI) :
		(theta <= -PI) ? (theta + 2.0f * PI) : theta);
}

//gets the distance between the points (x_1, y_1) and (x_2, y_2)
float FunnelDeque::Distance(float x_1, float y_1, float x_2, float y_2)
{
	float x = x_2 - x_1;
	float y = y_2 - y_1;
	return sqrt(x * x + y * y);
}

//gets the angle of the line tangent to two points of the same type
float FunnelDeque::ToTangent(float x_1, float y_1, float x_2, float y_2)
{
	return atan2(y_2 - y_1, x_2 - x_1);
}

//gets the angle of the line tangent to a point
//of type LeftTangent and one of type RightTangent
float FunnelDeque::ToTangentAlt(float x_1, float y_1, float x_2, float y_2, bool LeftRight)
{
	float h = Distance(x_1, y_1, x_2, y_2) / 2.0f;
	float l = sqrt(h * h - m_dRadius * m_dRadius);
	float interior = atan(m_dRadius / l);
	float absolute = atan2(y_2 - y_1, x_2 - x_1);
	if (LeftRight)
	{
		interior = -interior;
	}
	return AngleRange(absolute + interior);
}

//gets the angle of the line going through one point
//and running tangent to another
float FunnelDeque::PointToTangent(float x_1, float y_1, float x_2, float y_2, bool Right)
{
	float h = Distance(x_1, y_1, x_2, y_2);
	float l = sqrt(h * h - m_dRadius * m_dRadius);
	float interior = atan(m_dRadius / l);
	float absolute = atan2(y_2 - y_1, x_2 - x_1);
	if (Right)
	{
		interior = -interior;
	}
	return AngleRange(absolute + interior);
}

//prints the funnel (used for debugging)
void FunnelDeque::Print()
{
	FunnelNode *Current = m_CLeft;
	while (Current != NULL)
	{
		sr_out << "\t(" << Current->X() << ", " << Current->Y() << ")";
		if (Current == m_CLeft)
		{
			sr_out << " <-- Left";
		}
		if (Current == m_CApex)
		{
			sr_out << " <-- Apex";
		}
		if (Current == m_CRight)
		{
			sr_out << " <-- Right";
		}
		sr_out << srnl;
		Current = Current->Right();
	}
}

//prints the path and then the funnel (used for debugging)
void FunnelDeque::Print(SrPolygon path)
{
	sr_out << "Path = {";
	for (int i = 0; i < path.size() - 1; i++)
	{
		SrPnt2 p = path[i];
		sr_out << "(" << p.x << ", " << p.y << "), ";
	}
	if (path.size() > 0)
	{
		SrPnt2 p = path[path.size() - 1];
		sr_out << "(" << p.x << ", " << p.y << ")";
	}
	sr_out << "}\n";
	Print();
}

//definition of FunnelDeque functions }
