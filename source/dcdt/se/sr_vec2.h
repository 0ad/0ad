/** \file sr_vec2.h 
 * Two dimensional vec2tor */

# ifndef SR_VEC2_H
# define SR_VEC2_H

# include "sr_input.h"
# include "sr_output.h"

class SrVec2;

/*! This "point typedef" is used only to coherently describe variables,
    making possible to distiguish points from vectors parameters when
    interpreting the meaning of parameters. */
typedef SrVec2 SrPnt2;

/*! \class SrVec2 sr_vec2.h
    \brief Two dimensional vector. 

    Its two coordinates are of type float. */
class SrVec2
 { public :
    float x, y;
   public :
    static const SrVec2 null;     //!< (0,0) null vector
    static const SrVec2 one;      //!< (1,1) vector
    static const SrVec2 minusone; //!< (-1,-1) vector
    static const SrVec2 i;        //!< (1,0) vector
    static const SrVec2 j;        //!< (0,1) vector
   public :

    /*! Initializes SrVec2 as a null vector. Implemented inline. */
    SrVec2 () : x(0), y(0) {} 

    /*! Copy constructor. Implemented inline. */
    SrVec2 ( const SrVec2& v ) : x(v.x), y(v.y) {}

    /*! Initializes with the two given float coordinates. Implemented inline. */
    SrVec2 ( float a, float b ) : x(a), y(b) {}

    /*! Initializes from a float pointer. Implemented inline. */
    SrVec2 ( float p[2] ) : x(p[0]), y(p[1]) {}

    /*! Sets coordinates from the given vector. Implemented inline. */
    void set ( const SrVec2& v ) { x=v.x; y=v.y; }

    /*! Sets coordinates from the two given float values. Implemented inline. */
    void set ( float a, float b ) { x=a; y=b; }

    /*! Apply a rotation to SrVec2 by angle a, where sa=sin(a), and ca=cos(a). */
    void rot ( float sa, float ca );

    /*! Apply a rotation around cent. */
    void rot ( const SrVec2& cent, float sa, float ca );

    /*! Apply a rotation to SrVec2 with the angle in radians. */
    void rot ( float radians );

    /*! Apply a rotation around cent, with the angle in radians. */
    void rot ( const SrVec2& cent, float radians );

    /*! Makes each coordinate to have its absolute value. */
    void abs ();

    /*! Normalize the vector. If its norm is 0 or 1, the vector is not changed.
        Note that the norm is checked to be 1 or 0 using norm2(), an so without 
        evaluating the square root. */
    void normalize ();

    /*! Sets the length to be n and returns the previous length */
    float len ( float n );

    /*! Returns the length of the vector. Equivalent to norm() */
    float len () { return norm(); }

    /*! Returns the norm (distance to zero) of the vector. */
    float norm () const;

    /*! Returns the square of the norm (x*x + y*y). Implemented inline. */
    float norm2 () const { return x*x + y*y; }

    /*! Returns the norm, but using the max function of distance, which does not require sqrt() */
    float norm_max () const;

    /*! Returns the angle from the vector (0,0) to SrVec2. The angle will be a
        value in radians inside the interval [0,pi], and uses atan2. */
    float angle () const; // [0,pi]

    /*! Returns the angle from the vector (0,0) to SrVec2, but using the max norm. The angle 
        will then be a value inside the interval [0,8). */
    float angle_max () const; // [0,8)

    /*! Returns the orthogonal vector (-y,x). */
    SrVec2 ortho () const { return SrVec2(-y,x); }

    /*! Returns true if all coordinates are zero; false otherwise. */
    bool iszero () const { return x==0.0 && y==0.0? true:false; }

    /*! Returns true if the vector is inside the closed neighboorhood of point 0,0 and radius ds. */
    bool nextzero ( float ds ) const { return norm2()<=ds*ds? true:false; }

    /*! Allows member access like a vector */
    float& operator[] ( int i ) { return *((&x)+i); }

    /*! Convertion to a float pointer. */
    operator float* () const { return (float*)&x; }

    /*! Assignment operator from another SrVec2. Implemented inline. */
    void operator = ( const SrVec2& v ) { set(v); }

    /*! Assignment operator from a const float *. Implemented inline. */
    void operator = ( const float *v ) { x=(float)v[0]; y=(float)v[1]; }

    /*! -= operator with another SrVec2. Implemented inline. */
    void operator -= ( const SrVec2& v ) { x-=v.x; y-=v.y; }

    /*! += operator with another SrVec2. Implemented inline. */
    void operator += ( const SrVec2& v ) { x+=v.x; y+=v.y; }

    /*! *= operator with another SrVec2. Implemented inline. */
    void operator *= ( float r ) { x*=r; y*=r; }

    /*! /= operator with another SrVec2. Implemented inline. */
    void operator /= ( float r ) { x/=r; y/=r; }

    /*! Unary minus. Implemented inline. */
    friend SrVec2 operator - ( const SrVec2& v )
           { return SrVec2 ( -v.x, -v.y ); }

    /*! Adds two vectors. Implemented inline. */
    friend SrVec2 operator + ( const SrVec2& v1, const SrVec2& v2 ) 
           { return SrVec2 (v1.x+v2.x, v1.y+v2.y); }

    /*! Subtracts two vectors. Implemented inline. */
    friend SrVec2 operator - ( const SrVec2& v1, const SrVec2& v2 ) 
           { return SrVec2 (v1.x-v2.x, v1.y-v2.y); }

    /*! Comparison operator. Implemented inline. */
    friend bool operator == ( const SrVec2& v1, const SrVec2& v2 )
           { return v1.x==v2.x && v1.y==v2.y? true:false; }

    /*! Comparison operator. Implemented inline. */
    friend bool operator != ( const SrVec2& v1, const SrVec2& v2 )
           { return v1.x!=v2.x || v1.y!=v2.y? true:false; }

    /*! Multiplication by scalar. Implemented inline. */
    friend SrVec2 operator * ( const SrVec2& v, float r )
           { return SrVec2 (v.x*r, v.y*r); }

    /*! Division by scalar. Implemented inline. */
    friend SrVec2 operator / ( const SrVec2& v, float r )
           { return SrVec2 (v.x/r, v.y/r); }

    /*! Returns true if dist(v1,v2)<=ds, otherwise returns false. Implemented inline. */
    friend bool next ( const SrVec2& v1, const SrVec2& v2, float ds )
           { return dist2(v1,v2)<=ds*ds? true:false; }

    /*! Swaps the contents of v1 with v2. */
    friend void swap ( SrVec2& v1, SrVec2& v2 );

    /*! Returns the distance between v1 and v2 using norm_max(). */
    friend float dist_max ( const SrVec2& v1, const SrVec2& v2 );

    /*! Returns the distance between p1 and p2. */
    friend float dist ( const SrPnt2& p1, const SrPnt2& p2 );

    /*! Returns the square of the distance between p1 and p2. */
    friend float dist2 ( const SrPnt2& p1, const SrPnt2& p2 );

    /*! Returns the angle in radians between v1 and v2,
        which is inside the interval [0,pi] */
    friend float angle ( const SrVec2& v1, const SrVec2& v2 );

    friend float angle_fornormvecs ( const SrVec2 &v1, const SrVec2 &v2 );

    /*! Returns the oriented angle in radians between v1 and v2,
        which is inside the interval (-pi,pi] */
    friend float angle_ori ( const SrVec2& v1, const SrVec2& v2 );

    /*! Returns the angle between v1 and v2, but using the max norm.
        The angle will be a value inside the interval [0,4]. */
    friend float angle_max ( const SrVec2& v1, const SrVec2& v2 ); // [0,4]

    /*! Returns the oriented angle between v1 and v2, but using the max norm.
        The angle will be a value inside the interval (-4,4]. */
    friend float angle_max_ori ( const SrVec2 &v1, const SrVec2 &v2 ); // (-4,4]

    /*! Returns the cross product z coordinate of v1 and v2: a zero value means
        that the vectors are colinear, otherwise, a positive number means that
        v2 is on the left side of v1, and a negative number means that v2 is on
        the right side of v1. */
    friend float cross ( const SrVec2& v1, const SrVec2& v2 );

    /*! Returns the dot product of v1 and v2 (v1.x*v2.x + v1.y*v2.y). */
    friend float dot ( const SrVec2& v1, const SrVec2& v2 );

    /*! Returns the linear interpolation of v1 v2 in parameter t ( v1(1-t)+v2(t) ). */
    friend SrVec2 lerp ( const SrVec2& v1, const SrVec2& v2, float t );

    /*! Returns -1,0,1 depending if v1 is less, equal or greater than v2, 
        using a comparison by coordinates. */
    friend int compare ( const SrVec2& v1, const SrVec2& v2 );

    /*! Pointer version of the compare function. */
    friend int compare ( const SrVec2* v1, const SrVec2* v2 );

    /*! Returns -1,0,1 depending if v1 is less, equal or greater than v2, 
        using a 2d polar comparison with în relation to (1,0) (using angle_max()). */
    friend int compare_polar ( const SrVec2& v1, const SrVec2& v2 );

    /*! Pointer version of the compare_polar function. */
    friend int compare_polar ( const SrVec2* v1, const SrVec2* v2 );

    /*! Returns u,v,w==1-u-v, u+v+w==1, such that p1*u + p2*v + p3*w == p */
    friend void barycentric ( const SrPnt2& p1, const SrPnt2& p2, const SrPnt2& p3, const SrPnt2& p,
                              float& u, float& v, float& w );

    /*! Wrapper for the sr_ccw() function in sr_geo2.h */
    friend float ccw ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3 );

    /*! Wrapper for the equivalent sr_segments_intersect function in sr_geo2.h */
    friend bool segments_intersect ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p4 );

    /*! Wrapper for the equivalent sr_segments_intersect function in sr_geo2.h */
    friend bool segments_intersect ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p4, SrVec2& p );

    /*! Wrapper for the equivalent sr_lines_intersect function in sr_geo2.h */
    friend bool lines_intersect ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p4 );

    /*! Wrapper for the equivalent sr_lines_intersect function in sr_geo2.h */
    friend bool lines_intersect ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p4, SrVec2& p );

    /*! Wrapper for the equivalent sr_line_projection function in sr_geo2.h */
    friend void line_projection ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p, SrVec2& q );

    /*! Wrapper for the equivalent sr_segment_projection function in sr_geo2.h */
    friend bool segment_projection ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p, SrVec2& q, float epsilon );

    /*! Wrapper for the equivalent sr_in_segment function in sr_geo2.h */
    friend bool in_segment ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p, float epsilon );

    /*! Wrapper for the equivalent sr_in_segment function in sr_geo2.h */
    friend bool in_segment ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p, float epsilon, float& dist2 );

    /*! Wrapper for the equivalent sr_in_triangle function in sr_geo2.h */
    friend bool in_triangle ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p );

    /*! Wrapper for the equivalent sr_in_circle function in sr_geo2.h */
    bool in_circle ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p );

    /*! Outputs in format: "x y". */
    friend SrOutput& operator<< ( SrOutput& o, const SrVec2& v );

    /*! Inputs from format: "x y". */
    friend SrInput& operator>> ( SrInput& in, SrVec2& v );
 };

//============================== end of file ===============================

# endif // SR_VEC2_H
