
/** \file sr_triangle.h 
 * A triangle in 3d space */

# ifndef SR_TRIANGLE_H
# define SR_TRIANGLE_H

# include "sr_vec.h" 

/*! \class SrTriangle sr_triangle.h
    \brief A triangle in 3d space. 

 */
class SrTriangle
 { public :
    SrPnt a, b, c;

   public :

    /*! Initializes SrTriangle as the triangle (i,j,k). Implemented inline. */
    SrTriangle () : a(SrPnt::i), b(SrPnt::j), c(SrPnt::k) {}

    /*! Constructor from three points. */
    SrTriangle ( const SrPnt& p1, const SrPnt& p2, const SrPnt& p3 ) : a(p1), b(p2), c(p3) {}

    /*! Sets vertices coordinates. */
    void set ( const SrPnt& p1, const SrPnt& p2, const SrPnt& p3 ) { a=p1; b=p2; c=p3; }

    /*! Returns k, such that a*k.x + b*k.y + c*k.z == p. */
    SrVec barycentric ( const SrPnt &p ) const;

    /*! Update the position of triangle [a,b,c] in order to keep the baricentric
        coordinates of an interior point that moved. Let p=a*k.x + b*k.y + c*k.z.
        When p is displaced to p+v, the triangle vertices [a,b,c] are updated so
        to achieve a*k.x + b*k.y + c*k.z == p+v. */
    void translate ( const SrVec &k, const SrVec& v );

    /*! Apply the translation v to each triangle vertex. */
    void translate ( const SrVec& v );

    /*! Returns the normalized triangle normal. */
    SrVec normal () const;

    /*! Outputs in format: "a,b,c". */
    friend SrOutput& operator<< ( SrOutput& o, const SrTriangle& t );

    /*! Inputs from format: "a,b,c". */
    friend SrInput& operator>> ( SrInput& in, SrTriangle& t );
 };

//============================== end of file ===============================

# endif // SR_TRIANGLE_H
