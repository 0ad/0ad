#include "BoundingObjects.h"

bool CBoundingObject::intersects( CBoundingObject* obj )
{
	CVector2D delta = pos - obj->pos;
	if( !delta.within( trivialRejectionRadius + obj->trivialRejectionRadius ) )
		return( false );

	if( obj->type > type )
	{
		return( obj->_intersects( this, &delta ) );
	}
	else
		return( _intersects( obj, &delta ) );
}

void CBoundingCircle::setPosition( float _x, float _y )
{
	pos.x = _x; pos.y = _y;
}

void CBoundingCircle::setRadius( float _radius )
{
	r = _radius;
	trivialRejectionRadius = _radius * _radius;
}

bool CBoundingCircle::_intersects( CBoundingObject* obj, CVector2D* delta )
{
	// Easy enough. Trivial rejection is sufficient.
	return( true );
}

CBoundingBox::CBoundingBox( float _x, float _y, float _orientation, float _width, float _height )
{
	setPosition( _x, _y );
	setDimensions( _width, _height );
	setOrientation( _orientation );
}

void CBoundingBox::setPosition( float _x, float _y )
{
	pos.x = _x; pos.y = _y;
}

void CBoundingBox::setDimensions( float _width, float _height )
{
	w = _width / 2.0f;
	h = _height / 2.0f;
	trivialRejectionRadius = ( w * w ) + ( h * h );
}

void CBoundingBox::setOrientation( float _orientation )
{
	u.x = sin( _orientation );
	u.y = cos( _orientation );
	v.x = u.y;
	v.y = -u.x;
}

bool CBoundingBox::_intersects( CBoundingObject* obj, CVector2D* delta )
{
	if( obj->type == BOUND_CIRCLE )
	{
		// Imperfect but quick...

		CBoundingCircle* c = (CBoundingCircle*)obj;
		float deltah = fabs( delta->dot( u ) );
		if( deltah > ( h + c->r ) ) return( false );
		float deltaw = fabs( delta->dot( v ) );
		if( deltaw > ( w + c->r ) ) return( false );
		return( true );
	}
	else
	{
		// Another OABB:
	
		// Seperable axis theorem. (Optimizations: Algorithmic done, really low-level stuff not.)
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
		uv = u.dot( b->v );
		vu = v.dot( b->u );
		uu = u.dot( b->u );
		vv = v.dot( b->v );

		// Project box 2 onto v-axis of box 1

		prj1 = fabs( vu * b->h + vv * b->w );
		prj2 = fabs( vu * b->h - vv * b->h );
		dm = delta->dot( v );

		if( prj1 > prj2 )
		{
			if( ( dm - prj1 ) > w ) return( false );
		}
		else
			if( ( dm - prj2 ) > w ) return( false );
		
		// Project box 2 onto u-axis of box 1

		prj1 = fabs( uu * b->h + uv * b->w );
		prj2 = fabs( uu * b->h - uv * b->w );
		dm = delta->dot( u );

		if( prj1 > prj2 ) 
		{
			if( ( dm - prj1 ) > h ) return( false );
		}
		else
			if( ( dm - prj2 ) > h ) return( false );

		// Project box 1 onto v-axis of box 2

		prj1 = fabs( uv * h + vv * w );
		prj2 = fabs( uv * h - vv * w );

		if( prj1 > prj2 )
		{
			if( ( dm - prj1 ) > b->w ) return( false );
		}
		else
			if( ( dm - prj2 ) > b->w ) return( false );

		// Project box 1 onto u-axis of box 2

		prj1 = fabs( uu * h + vu * w );
		prj2 = fabs( uu * h - vu * w );

		if( prj1 > prj2 )
		{
			if( ( dm - prj1 ) > b->h ) return( false );
		}
		else
			if( ( dm - prj2 ) > b->h ) return( false );

		// And a partridge in a pear tree...
		
		return( true );

	}
}

		

