
# ifndef SR_POINTS_H
# define SR_POINTS_H

/** \file sr_points.h 
 * manages a set of points
 */

# include "sr_box.h"
# include "sr_vec2.h"
# include "sr_color.h"
# include "sr_array.h"

/*! \class SrPoints sr_points.h
    \brief a set of points

    Keeps the description of points.
    Points are stored in array P.
    An optional array A with atributes (color and size) can be used.
    Even if P is a public member, it is the user responsability to
    mantain the size of P equal to the size of A, if A!=0. */   
class SrPoints
 { public :
    SrArray<SrPnt> P;  //<! Array of used points
    struct Atrib { SrColor c; float s; };
    SrArray<Atrib>* A; //<! Optional to use attributes
    static const char* class_name;

   public :

    /* Default constructor. */
    SrPoints ();

    /* Destructor . */
   ~SrPoints ();

    /*! Set the size of array P to zero. */
    void init ();

    /*! Allocates A if needed, and set the size of array P and A to zero. */
    void init_with_attributes ();

    /*! Returns true if P array is empty; false otherwise. */
    bool empty () const { return P.size()==0? true:false; }

    /*! Compress array P and A. */
    void compress ();

    /*! Push in P a new point */
    void push ( const SrPnt& p );
    void push ( const SrPnt2& p );
    void push ( float x, float y, float z=0 );

    /*! Push in P a new point and push attributes in A. Only valid
        if init_with_attributes() was called before */
    void push ( const SrPnt& p, SrColor c, float size=1.0f );
    void push ( const SrPnt2& p, SrColor c, float size=1.0f );
    void push ( float x, float y, SrColor c, float size=1.0f );
    void push ( float x, float y, float z, SrColor c, float size=1.0f );

    /*! Returns the bounding box of all vertices used. The returned box can be empty. */
    void get_bounding_box ( SrBox &b ) const;
 };


//================================ End of File =================================================

# endif  // SR_POINTS_H

