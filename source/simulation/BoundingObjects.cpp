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

#include "precompiled.h"

#include "BoundingObjects.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"

bool CBoundingObject::Intersects( CBoundingObject* obj )
{
	CVector2D delta = m_pos - obj->m_pos;

	if( !delta.within( m_radius + obj->m_radius ) )
		return( false );

	if( obj->m_type > m_type )	// More complex types get the burden of processing.
	{
		return( obj->LooselyIntersects( this, delta ) );
	}
	else
		return( LooselyIntersects( obj, delta ) );
}

bool CBoundingObject::Contains( const CVector2D& point )
{
	CVector2D delta = m_pos - point;

	if( !delta.within( m_radius ) )
		return( false );

	return( LooselyContains( point, delta ) );
}

void CBoundingObject::SetPosition( float x, float y )
{
	m_pos.x = x; m_pos.y = y;
}

void CBoundingObject::SetHeight( float height )
{
	m_height = height;
}

CBoundingCircle::CBoundingCircle( float x, float y, float radius, float height )
{
	m_type = BOUND_CIRCLE;
	SetPosition( x, y );
	SetRadius( radius );
	SetHeight( height );
}

CBoundingCircle::CBoundingCircle( float x, float y, CBoundingCircle* copy )
{
	m_type = BOUND_CIRCLE;
 	SetPosition( x, y );
	SetRadius( copy->m_radius );
	SetHeight( copy->m_height );
}

void CBoundingCircle::SetRadius( float radius )
{
	m_radius = radius;
}

bool CBoundingCircle::LooselyIntersects( CBoundingObject* obj, const CVector2D& UNUSED(delta) )
{
	debug_assert( obj->m_type == BOUND_CIRCLE );
	// Easy enough. The only time this gets called is a circle-circle collision,
	// but we know the circles collide (they passed the trivial rejection step)
	return( true );
}

bool CBoundingCircle::LooselyContains( const CVector2D& UNUSED(point), const CVector2D& UNUSED(delta) )
{
	return( true );
}

void CBoundingCircle::Render( float height )
{
	glBegin( GL_LINE_LOOP );

	for( int i = 0; i < 10; i++ )
	{
		float ang = i * 2 * (float)M_PI / 10.0f;
		float x = m_pos.x + m_radius * sin( ang );
		float y = m_pos.y + m_radius * cos( ang );
		glVertex3f( x, height, y );
	}

	glEnd();
}

CBoundingBox::CBoundingBox( float x, float y, const CVector2D& u, float width, float depth, float height )
{
	m_type = BOUND_OABB;
	SetPosition( x, y );
	SetDimensions( width, depth );
	SetHeight( height );
	SetOrientation( u );
}

CBoundingBox::CBoundingBox( float x, float y, const CVector2D& u, CBoundingBox* copy )
{
	m_type = BOUND_OABB;
	SetPosition( x, y );
	SetDimensions( copy->GetWidth(), copy->GetDepth() );
	SetHeight( copy->m_height );
	SetOrientation( u ); 
}

CBoundingBox::CBoundingBox( float x, float y, float orientation, float width, float depth, float height )
{
	m_type = BOUND_OABB;
	SetPosition( x, y );
	SetDimensions( width, depth );
	SetHeight( height );
	SetOrientation( orientation );
}

CBoundingBox::CBoundingBox( float x, float y, float orientation, CBoundingBox* copy )
{
	m_type = BOUND_OABB;
	SetPosition( x, y );
	SetDimensions( copy->GetWidth(), copy->GetDepth() );
	SetHeight( copy->m_height );
	SetOrientation( orientation ); 
}

void CBoundingBox::SetDimensions( float width, float depth )
{
	m_w = width / 2.0f;
	m_d = depth / 2.0f;
	m_radius = sqrt( ( m_w * m_w ) + ( m_d * m_d ) );
}

void CBoundingBox::SetOrientation( float orientation )
{
	m_u.x = sin( orientation );
	m_u.y = cos( orientation );
	m_v.x = m_u.y;
	m_v.y = -m_u.x;
}

void CBoundingBox::SetOrientation( const CVector2D& u )
{
	m_u = u;
	m_v.x = m_u.y;
	m_v.y = -m_u.x;
}

bool CBoundingBox::LooselyIntersects( CBoundingObject* obj, const CVector2D& delta )
{
	if( obj->m_type == BOUND_CIRCLE )
	{
		// Imperfect but quick...

		CBoundingCircle* c = (CBoundingCircle*)obj;
		float deltad = fabs( delta.Dot( m_u ) );
		if( deltad > ( m_d + c->m_radius ) ) return( false );
		float deltaw = fabs( delta.Dot( m_v ) );
		if( deltaw > ( m_w + c->m_radius ) ) return( false );
		return( true );
	}
	else
	{
		// Another OABB:
	
		// Seperable axis theorem. (Optimizations: Algorithmic done, low-level stuff not.)
		// Debugging this can often be quite entertaining.
	
		CBoundingBox* b = (CBoundingBox*)obj;

		// SAT in a nutshell: If two boxes are disjoint, there's an axis where their projections don't overlap
		// That axis (in 2D) will run parallel to a side of one of the boxes

		// This code computes projections of boxes onto each axis in turn - hopefully quickly
		// then does simple max/min checks to determine overlap

		// uv, vu, uu, vv are dot-products of our u- and v- axes of each box.
		// prj1, prj2 are two coordinates locating the projections of two corners of a box onto an axis.
		// dm is the distance between the boxes, projected onto the axis.
	
		// Lots of nice symmetries used to help speed things up

		// Note that a trivial-rejection test has already taken place by this point.

		float uv, vu, uu, vv, prj1, prj2, dm;
		uv = m_u.Dot( b->m_v );
		vu = m_v.Dot( b->m_u );
		uu = m_u.Dot( b->m_u );
		vv = m_v.Dot( b->m_v );

		// Project box 2 onto v-axis of box 1

		prj1 = fabs( vu * b->m_d + vv * b->m_w );
		prj2 = fabs( vu * b->m_d - vv * b->m_w );
		dm = delta.Dot( m_v );

		if( prj1 > prj2 )
		{
			if( ( dm - prj1 ) > m_w ) return( false );
		}
		else
			if( ( dm - prj2 ) > m_w ) return( false );
		
		// Project box 2 onto u-axis of box 1

		prj1 = fabs( uu * b->m_d + uv * b->m_w );
		prj2 = fabs( uu * b->m_d - uv * b->m_w );
		dm = delta.Dot( m_u );

		if( prj1 > prj2 ) 
		{
			if( ( dm - prj1 ) > m_d ) return( false );
		}
		else
			if( ( dm - prj2 ) > m_d ) return( false );

		// Project box 1 onto v-axis of box 2

		prj1 = fabs( uv * m_d + vv * m_w );
		prj2 = fabs( uv * m_d - vv * m_w );
		dm = delta.Dot( b->m_v );

		if( prj1 > prj2 )
		{
			if( ( dm - prj1 ) > b->m_w ) return( false );
		}
		else
			if( ( dm - prj2 ) > b->m_w ) return( false );

		// Project box 1 onto u-axis of box 2

		prj1 = fabs( uu * m_d + vu * m_w );
		prj2 = fabs( uu * m_d - vu * m_w );
		dm = delta.Dot( b->m_u );

		if( prj1 > prj2 )
		{
			if( ( dm - prj1 ) > b->m_d ) return( false );
		}
		else
			if( ( dm - prj2 ) > b->m_d ) return( false );

		return( true );

	}
}

bool CBoundingBox::LooselyContains( const CVector2D& UNUSED(point), const CVector2D& delta )
{
	float deltad = fabs( delta.Dot( m_u ) );
	if( deltad > m_d ) return( false );
	float deltaw = fabs( delta.Dot( m_v ) );
	if( deltaw > m_w ) return( false );
	return( true );
}

void CBoundingBox::Render( float height )
{
	glBegin( GL_LINE_LOOP );

	CVector2D p;

	p = m_pos + m_u * m_d + m_v * m_w;
	glVertex3f( p.x, height, p.y );

	p = m_pos + m_u * m_d - m_v * m_w;
	glVertex3f( p.x, height, p.y );

	p = m_pos - m_u * m_d - m_v * m_w;
	glVertex3f( p.x, height, p.y );

	p = m_pos - m_u * m_d + m_v * m_w;
	glVertex3f( p.x, height, p.y );

	glEnd();

}
		

