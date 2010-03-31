/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

// BoundingObjects.h
// 
// Bounding circle and object-aligned bounding box. 2D, for simulation code.
//
// Note: object-aligned bounding boxes are often referred to as oriented bounding boxes (OBBs)

#ifndef INCLUDED_BOUNDINGOBJECTS
#define INCLUDED_BOUNDINGOBJECTS

#include "ps/Vector2D.h"

class CBoundingBox;
class CBoundingCircle;

class CBoundingObject
{
public:
	CBoundingObject() {}
	virtual ~CBoundingObject() {}
	enum EBoundingType
	{
		BOUND_NONE,
		BOUND_CIRCLE,
		BOUND_OABB
	};
	EBoundingType m_type;
	CVector2D m_pos;
	float m_radius;
	float m_height;

	void SetPosition( float x, float y );
	void SetHeight( float height );
	bool Intersects( CBoundingObject* obj );
	bool Contains( const CVector2D& point );
	virtual bool LooselyIntersects( CBoundingObject* obj, const CVector2D& delta ) = 0;
	virtual bool LooselyContains( const CVector2D& point, const CVector2D& delta ) = 0;
	virtual void Render( float height ) = 0;	// Temporary
};

class CBoundingCircle : public CBoundingObject
{
public:
	CBoundingCircle() { m_type = BOUND_OABB; }
	CBoundingCircle( float x, float y, float radius, float height );
	CBoundingCircle( float x, float y, CBoundingCircle* copy );
	void SetRadius( float radius );
	bool LooselyIntersects( CBoundingObject* obj, const CVector2D& delta );
	bool LooselyContains( const CVector2D& point, const CVector2D& delta );
	void Render( float height );	// Temporary
};

class CBoundingBox : public CBoundingObject
{
public:
	CBoundingBox() { m_type = BOUND_OABB; }
	CVector2D m_u; // Unit vector along the direction of this box's depth.
	CVector2D m_v; // Unit vector along the direction of this box's width.
	float m_d; // Half this box's depth.
	float m_w; // Half this box's width.
	CBoundingBox( float x, float y, float orientation, float width, float depth, float height );
	CBoundingBox( float x, float y, const CVector2D& orientation, float width, float depth, float height );
	CBoundingBox( float x, float y, float orientation, CBoundingBox* copy );
	CBoundingBox( float x, float y, const CVector2D& orientation, CBoundingBox* copy );
	void SetDimensions( float width, float depth );
	void SetOrientation( float orientation );
	void SetOrientation( const CVector2D& orientation );
	float GetWidth() const { return( 2.0f * m_w ); };
	float GetDepth() const { return( 2.0f * m_d ); };
	bool LooselyIntersects( CBoundingObject* obj, const CVector2D& delta );
	bool LooselyContains( const CVector2D& point, const CVector2D& delta );
	void Render( float height );	// Temporary
};

#endif
