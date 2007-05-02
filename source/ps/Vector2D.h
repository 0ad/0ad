// Vector2D.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// 2-dimensional vector class, primarily for use by simulation code.

#ifndef VECTOR_2D_INCLUDED
#define VECTOR_2D_INCLUDED

#include <math.h>
#include "maths/Vector3D.h"

class CVector2D
{
public:
	float x;
	float y;
	inline CVector2D() { x = 0.0f; y = 0.0f; }
	inline CVector2D( float _x, float _y )
	{
		x = _x; y = _y;
	}
	inline CVector2D( const CVector3D& v3 ) // This is done an awful lot.
	{
		x = v3.X; y = v3.Z;
	}
	inline operator CVector3D() const
	{
		return( CVector3D( x, 0, y ) );
	}
	inline bool operator==( const CVector2D& rhs ) const
	{
		return( x == rhs.x && y == rhs.y );
	}
	static inline float Dot( const CVector2D& u, const CVector2D& v )
	{
		return( u.x * v.x + u.y * v.y );
	}
	static inline float betadot( const CVector2D& u, const CVector2D& v )
	{
		// Beta-dot product. I have no idea if that's its correct name
		// but use of it tends to simplify collision formulae.
		// At the moment I think all of my code uses separate vectors
		// and dots them together, though.
		return( u.x * v.y - u.y * v.x );
	}
	inline CVector2D beta() const
	{
		return( CVector2D( y, -x ) );
	}
	inline float Dot( const CVector2D& u ) const
	{
		return( Dot( *this, u ) );
	}
	inline float betadot( const CVector2D& u ) const
	{	
		return( betadot( *this, u ) );
	}
	inline CVector2D operator+( const CVector2D& u ) const
	{
		return( CVector2D( x + u.x, y + u.y ) );
	}
	inline CVector2D operator-( const CVector2D& u ) const
	{
		return( CVector2D( x - u.x, y - u.y ) );
	}
	inline CVector2D& operator+=( const CVector2D& u )
	{
		x += u.x; y += u.y;
		return( *this );
	}
	inline CVector2D& operator-=( const CVector2D& u )
	{
		x -= u.x; y -= u.y;
		return( *this );
	}
	inline CVector2D operator*( const float scale ) const
	{
		return( CVector2D( x * scale, y * scale ) );
	}
	inline CVector2D operator/( const float scale ) const
	{
		return( CVector2D( x / scale, y / scale ) );
	}
	inline CVector2D& operator*=( const float scale )
	{
		x *= scale; y *= scale;
		return( *this );
	} 
	inline CVector2D& operator/=( const float scale )
	{
		x /= scale; y /= scale;
		return( *this );
	}
	inline float Length() const
	{
		return( sqrt( x * x + y * y ) );
	}
	inline float length2() const
	{
		return( x * x + y * y );
	}
	CVector2D Normalize() const
	{
		float l = Length();
		if( l < 0.00001 ) return( CVector2D( 1.0f, 0.0f ) );
		l = 1 / l;
		return( CVector2D( x * l, y * l ) );
	}
	inline bool within( const float dist ) const
	{
		return( ( x * x + y * y ) <= ( dist * dist ) );
	}
};

#endif
