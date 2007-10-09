
# ifndef SR_PLANE_H
# define SR_PLANE_H

# include "sr_vec.h"

class SrPlane
 { public :
    SrVec coords;
    float coordsw;
    static const SrPlane XY, //<! The plane of ( SrVec::null, SrVec::i, SrVec::j )
                         XZ, //<! The plane of ( SrVec::null, SrVec::i, SrVec::k )
                         YZ; //<! The plane of ( SrVec::null, SrVec::j, SrVec::k )
   public:

    /*! The default constructor initializes as the plane passing at (0,0,0)
        with normal (0,0,1). */
    SrPlane ();

    /*! Constructor from center and normal. Normal will be normalized. */
    SrPlane ( const SrVec& center, const SrVec& normal );

    /*! Constructor from three points in the plane. */
    SrPlane ( const SrVec& p1, const SrVec& p2, const SrVec& p3 );

    /*! Set as the plane passing at center with given normal. Normal will be normalized. */
    bool set ( const SrVec& center, const SrVec& normal );

    /*! Set as the plane passing trough three points. */
    bool set ( const SrVec& p1, const SrVec& p2, const SrVec& p3 );

    /*! determines if the plane is parallel to the line [p1,p2], according
        to the (optional) given precision ds. This method is fast (as the
        plane representation is kept normalized) and performs only one 
        subraction and one dot product. */
    bool parallel ( const SrVec& p1, const SrVec& p2, float ds=0 ) const;

    /*! Returns p, that is the intersection between SrPlane and the infinity
        line <p1,p2>. (0,0,0) is returned if they are parallel. Use parallel()
        to test this before. If t is non null, t will contain the relative
        interpolation factor such that p=p1(1-t)+p2(t). */
    SrVec intersect ( const SrVec& p1, const SrVec& p2, float *t=0 ) const;
 };

# endif // SR_PLANE_H
