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

#include "Vector2D.h"

class CBoundingBox;
class CBoundingCircle;

class CBoundingObject
{
public:
	CBoundingObject() { m_offset.x = 0; m_offset.y = 0; }
	enum EBoundingType
	{
		BOUND_CIRCLE,
		BOUND_OABB
	};
	EBoundingType m_type;
	CVector2D m_pos;
	CVector2D m_offset;
	float m_radius;
	void setPosition( float x, float y );
	bool intersects( CBoundingObject* obj );
	bool contains( const CVector2D& point );
	virtual bool _intersects( CBoundingObject* obj, const CVector2D& delta ) = 0;
	virtual bool _contains( const CVector2D& point, const CVector2D& delta ) = 0;
	virtual void render( float height ) = 0;	// Temporary
};

class CBoundingCircle : public CBoundingObject
{
public:
	CBoundingCircle() { m_type = BOUND_OABB; }
	CBoundingCircle( float x, float y, float radius );
	CBoundingCircle( float x, float y, CBoundingCircle* copy );
	void setRadius( float radius );
	bool _intersects( CBoundingObject* obj, const CVector2D& delta );
	bool _contains( const CVector2D& point, const CVector2D& delta );
	void render( float height );	// Temporary
};

class CBoundingBox : public CBoundingObject
{
public:
	CBoundingBox() { m_type = BOUND_OABB; }
	CVector2D m_u; // Unit vector along the direction of this box's height.
	CVector2D m_v; // Unit vector along the direction of this box's width.
	float m_h; // Half this box's height.
	float m_w; // Half this box's width.
	CBoundingBox( float x, float y, float orientation, float width, float height );
	CBoundingBox( float x, float y, const CVector2D& orientation, float width, float height );
	CBoundingBox( float x, float y, float orientation, CBoundingBox* copy );
	CBoundingBox( float x, float y, const CVector2D& orientation, CBoundingBox* copy );
	void setDimensions( float width, float height );
	void setOrientation( float orientation );
	void setOrientation( const CVector2D& orientation );
	float getWidth() const { return( 2.0f * m_w ); };
	float getHeight() const { return( 2.0f * m_h ); };
	bool _intersects( CBoundingObject* obj, const CVector2D& delta );
	bool _contains( const CVector2D& point, const CVector2D& delta );
	void render( float height );	// Temporary
};

#endif
