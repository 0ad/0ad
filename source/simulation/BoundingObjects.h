// BoundingObjects.h
//
// Last modified: 26 May 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Bounding circle and object-aligned bounding box. 2D, for simulation code.
//
// Note: object-aligned bounding boxes are often referred to as oriented bounding boxes (OBBs)
// At present, this stuff compiles; no guarantee is made of it actually working yet, however.

#ifndef BOUNDING_OBJECTS_INCLUDED
#define BOUNDING_OBJECTS_INCLUDED

#include "Prometheus.h"

class CVector2D
{
public:
	float x;
	float y;
	CVector2D() {}
	CVector2D( float _x, float _y )
	{
		x = _x; y = _y;
	}
	static inline float dot( CVector2D& u, CVector2D& v )
	{
		return( u.x * v.x + u.y * v.y );
	}
	inline float dot( CVector2D& u )
	{
		return( dot( *this, u ) );
	}
	inline CVector2D operator-( CVector2D& u )
	{
		return( CVector2D( x - u.x, y - u.y ) );
	}
	inline bool within( float dist )
	{
		return( ( x * x + y * y ) <= ( dist * dist ) );
	}
};

class CBoundingBox;
class CBoundingCircle;

class CBoundingObject
{
public:
	CBoundingObject() {}
	enum CBoundingType
	{
		BOUND_CIRCLE,
		BOUND_OABB
	};
	CBoundingType type;
	CVector2D pos;
	float trivialRejectionRadius;
	bool intersects( CBoundingObject* obj );
	virtual bool _intersects( CBoundingObject* obj, CVector2D* delta ) = 0;

};

class CBoundingCircle : public CBoundingObject
{
public:
	float r;
	CBoundingCircle( float _x, float _y, float _radius );
	void setPosition( float _x, float _y );
	void setRadius( float _radius );
	bool _intersects( CBoundingObject* obj, CVector2D* delta );
};

class CBoundingBox : public CBoundingObject
{
public:
	CVector2D u; // Unit vector along the direction of this box's height.
	CVector2D v; // Unit vector along the direction of this box's width.
	float h; // Half this box's height.
	float w; // Half this box's width.
	CBoundingBox( float _x, float _y, float _orientation, float _width, float _height );
	void setPosition( float _x, float _y );
	void setDimensions( float _width, float _height );
	void setOrientation( float _orientation );
	bool _intersects( CBoundingObject* obj, CVector2D* delta );
};

#endif