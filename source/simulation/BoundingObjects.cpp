#include "precompiled.h"

#include "BoundingObjects.h"
#include "ogl.h"
#include "MathUtil.h"
#include "stdio.h"
#include "assert.h"

bool CBoundingObject::intersects( CBoundingObject* obj )
{
	CVector2D delta = m_pos - obj->m_pos;

	if( !delta.within( m_radius + obj->m_radius ) )
		return( false );

	if( obj->m_type > m_type )	// More complex types get the burden of processing.
	{
		return( obj->_intersects( this, delta ) );
	}
	else
		return( _intersects( obj, delta ) );
}

bool CBoundingObject::contains( const CVector2D& point )
{
	CVector2D delta = m_pos - point;

	if( !delta.within( m_radius ) )
		return( false );

	return( _contains( point, delta ) );
}
CBoundingCircle::CBoundingCircle( float x, float y, float radius )
{
	m_type = BOUND_CIRCLE;
	setPosition( x, y );
	setRadius( radius );
}

CBoundingCircle::CBoundingCircle( float x, float y, CBoundingCircle* copy )
{
	m_type = BOUND_CIRCLE;
	m_offset = copy->m_offset;
 	setPosition( x, y );
	setRadius( copy->m_radius );
}

void CBoundingObject::setPosition( float x, float y )
{
	m_pos.x = x; m_pos.y = y;
	m_pos += m_offset;
}

void CBoundingCircle::setRadius( float radius )
{
	m_radius = radius;
}

bool CBoundingCircle::_intersects( CBoundingObject* obj, const CVector2D& delta )
{
	assert( obj->m_type == BOUND_CIRCLE );
	// Easy enough. The only time this gets called is a circle-circle collision,
	// but we know the circles collide (they passed the trivial rejection step)
	return( true );
}

bool CBoundingCircle::_contains( const CVector2D& point, const CVector2D& delta )
{
	return( true );
}

void CBoundingCircle::render( float height )
{
	glBegin( GL_LINE_LOOP );

	for( int i = 0; i < 10; i++ )
	{
		float ang = i * 2 * PI / 10.0f;
		float x = m_pos.x + m_radius * sin( ang );
		float y = m_pos.y + m_radius * cos( ang );
		glVertex3f( x, height, y );
	}

	glEnd();
}

CBoundingBox::CBoundingBox( float x, float y, const CVector2D& u, float width, float height )
{
	m_type = BOUND_OABB;
	setPosition( x, y );
	setDimensions( width, height );
	setOrientation( u );
}

CBoundingBox::CBoundingBox( float x, float y, const CVector2D& u, CBoundingBox* copy )
{
	m_type = BOUND_OABB;
	m_offset = copy->m_offset;
	setPosition( x, y );
	setDimensions( copy->getWidth(), copy->getHeight() );
	setOrientation( u ); 
}

CBoundingBox::CBoundingBox( float x, float y, float orientation, float width, float height )
{
	m_type = BOUND_OABB;
	setPosition( x, y );
	setDimensions( width, height );
	setOrientation( orientation );
}

CBoundingBox::CBoundingBox( float x, float y, float orientation, CBoundingBox* copy )
{
	m_type = BOUND_OABB;
	m_offset = copy->m_offset;
	setPosition( x, y );
	setDimensions( copy->getWidth(), copy->getHeight() );
	setOrientation( orientation ); 
}

void CBoundingBox::setDimensions( float width, float height )
{
	m_w = width / 2.0f;
	m_h = height / 2.0f;
	m_radius = sqrt( ( m_w * m_w ) + ( m_h * m_h ) );
}

void CBoundingBox::setOrientation( float orientation )
{
	m_u.x = sin( orientation );
	m_u.y = cos( orientation );
	m_v.x = m_u.y;
	m_v.y = -m_u.x;
}

void CBoundingBox::setOrientation( const CVector2D& u )
{
	m_u = u;
	m_v.x = m_u.y;
	m_v.y = -m_u.x;
}

bool CBoundingBox::_intersects( CBoundingObject* obj, const CVector2D& delta )
{
	if( obj->m_type == BOUND_CIRCLE )
	{
		// Imperfect but quick...

		CBoundingCircle* c = (CBoundingCircle*)obj;
		float deltah = fabs( delta.dot( m_u ) );
		if( deltah > ( m_h + c->m_radius ) ) return( false );
		float deltaw = fabs( delta.dot( m_v ) );
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
		uv = m_u.dot( b->m_v );
		vu = m_v.dot( b->m_u );
		uu = m_u.dot( b->m_u );
		vv = m_v.dot( b->m_v );

		// Project box 2 onto v-axis of box 1

		prj1 = fabs( vu * b->m_h + vv * b->m_w );
		prj2 = fabs( vu * b->m_h - vv * b->m_h );
		dm = delta.dot( m_v );

		if( prj1 > prj2 )
		{
			if( ( dm - prj1 ) > m_w ) return( false );
		}
		else
			if( ( dm - prj2 ) > m_w ) return( false );
		
		// Project box 2 onto u-axis of box 1

		prj1 = fabs( uu * b->m_h + uv * b->m_w );
		prj2 = fabs( uu * b->m_h - uv * b->m_w );
		dm = delta.dot( m_u );

		if( prj1 > prj2 ) 
		{
			if( ( dm - prj1 ) > m_h ) return( false );
		}
		else
			if( ( dm - prj2 ) > m_h ) return( false );

		// Project box 1 onto v-axis of box 2

		prj1 = fabs( uv * m_h + vv * m_w );
		prj2 = fabs( uv * m_h - vv * m_w );
		dm = delta.dot( b->m_v );

		if( prj1 > prj2 )
		{
			if( ( dm - prj1 ) > b->m_w ) return( false );
		}
		else
			if( ( dm - prj2 ) > b->m_w ) return( false );

		// Project box 1 onto u-axis of box 2

		prj1 = fabs( uu * m_h + vu * m_w );
		prj2 = fabs( uu * m_h - vu * m_w );
		dm = delta.dot( b->m_u );

		if( prj1 > prj2 )
		{
			if( ( dm - prj1 ) > b->m_h ) return( false );
		}
		else
			if( ( dm - prj2 ) > b->m_h ) return( false );

		return( true );

	}
}

bool CBoundingBox::_contains( const CVector2D& point, const CVector2D& delta )
{
	float deltah = fabs( delta.dot( m_u ) );
	if( deltah > m_h ) return( false );
	float deltaw = fabs( delta.dot( m_v ) );
	if( deltaw > m_w ) return( false );
	return( true );
}

void CBoundingBox::render( float height )
{
	glBegin( GL_LINE_LOOP );

	CVector2D p;

	p = m_pos + m_u * m_h + m_v * m_w;
	glVertex3f( p.x, height, p.y );

	p = m_pos + m_u * m_h - m_v * m_w;
	glVertex3f( p.x, height, p.y );

	p = m_pos - m_u * m_h - m_v * m_w;
	glVertex3f( p.x, height, p.y );

	p = m_pos - m_u * m_h + m_v * m_w;
	glVertex3f( p.x, height, p.y );

	glEnd();

}
		

