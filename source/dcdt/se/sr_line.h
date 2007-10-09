/** \file sr_line.h 
 * Three dimensional line */

# ifndef SR_LINE_H
# define SR_LINE_H

# include "sr_vec.h"

class SrBox;
class SrInput;
class SrOutput;

/*! \class SrLine sr_line.h
    \brief Three dimensional line

    SrLine defines a line by keeping two points in the three dimensional
    space. These two points are p1 and p2 of type SrVec(==SrPnt). When the line
    is considered as a segement, p1 and p2 will delimit the segment, and
    when the line is considered as a ray, the source of the ray is p1 and
    the ray direction is defined as p2-p1. */
class SrLine
 { public :
    SrPnt p1, p2;
   public :
    static const SrLine x;        //!< (0,0,0)--(1,0,0) line
    static const SrLine y;        //!< (0,0,0)--(0,1,0) line
    static const SrLine z;        //!< (0,0,0)--(0,0,1) line
   public :

    /*! Initializes SrLine as the x axis (0,0,0)--(1,0,0) line. */
    SrLine () : p1(SrPnt::null), p2(SrPnt::i) {}

    /*! Copy constructor. */
    SrLine ( const SrLine &l ) : p1(l.p1), p2(l.p2) {}

    /*! Initializes with the given endpoints. */
    SrLine ( const SrPnt &v1, const SrPnt &v2 ) : p1(v1), p2(v2) {}

    /*! Set endpoints. */
    void set ( const SrPnt &v1, const SrPnt &v2 ) { p1=v1; p2=v2; }

    /*! Same as copy operator. */
    void set ( const SrLine &l ) { p1=l.p1; p2=l.p2; }

    /*! Copy operator from another SrLine. */
    void operator = ( const SrLine &l ) { p1=l.p1; p2=l.p2; }

    /*! Calculates the intersection of SrLine with the triangle [v0,v1,v2].
        If the line intercepts the triangle, true is returned, otherwise
        false is returned. The triangle can be given in any orientation.
        When true is returned the return values t,u,v will satisfy:  
        (1-u-v)v0 + uv1 + vv2 == (1-t)p1 + (t)p2 == interception point.
        In this way, u and v indicate a parametric distance from the vertices
        and t is a parametric distance that can be used to determine if only
        the segment [p1,p2] intersects in fact the triangle. */
    bool intersects_triangle ( const SrPnt &v0, const SrPnt &v1, const SrPnt &v2,
                               float &t, float &u, float &v ) const;

    /*! Returns the number of intersections between the line and the square (v1,v2,v3,v4).
        In case true is returned, the intersection point is
        defined by (1-t)p1 + (t)p2, so that if t is between 0 and 1, the point
        is also inside the segment determined by SrLine. */
    bool intersects_square ( const SrPnt& v1, const SrPnt& v2,
                             const SrPnt& v3, const SrPnt& v4, float& t ) const; 

    /*! Returns 1 or 2 if the line intersects the box, otherwise 0 is returned.
        In case 2 is returned, there are two intersection points defined by
        (1-t1)p1 + (t1)p2, and (1-t2)p1 + (t2)p2 (t1<t2).
        In case 1 is returned, there is one intersection point (1-t1)p1 + (t1)p2,
        and t1 is equal to t2. Parameter vp is a pointer to SrPnt[4], and will contain
        the corners of the traversed side of the box, if vp is not null. */
    int intersects_box ( const SrBox& box, float& t1, float& t2, SrPnt* vp=0 ) const;

    /*! Returns 1 or 2 if the line intersects the sphere (center,radius), otherwise
        0 is returned. In case 1 or 2 is returned, there are one or two intersection
        points, which are put in the vp array, if the vp pointer is not null.
        If two intersection points exist they are ordered according to the proximity
        to SrLine::p1 */
    int intersects_sphere ( const SrPnt& center, float radius, SrPnt* vp=0 ) const;

    /*! Returns the closest point in the line to p. Parameter k, if given,
        will be such that: closestpt == p1+k*(p2-p1) */
    SrPnt closestpt ( SrPnt p, float* k=0 ) const;

    /*! Outputs in format: "p1 p2". */
    friend SrOutput& operator<< ( SrOutput& o, const SrLine& l );

    /*! Inputs from format: "p1 p2". */
    friend SrInput& operator>> ( SrInput& in, SrLine& l );

 };

//============================== end of file ===============================

# endif // SR_LINE_H
