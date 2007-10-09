
# ifndef SR_CYLINDER_H
# define SR_CYLINDER_H

/** \file sr_cylinder.h 
 * a cylinder
 */

# include "sr_vec.h"

class SrBox;

/*! \class SrCylinder sr_cylinder.h
    \brief a cylinder

    SrCylinder represents a cylinder based on its endpoints and radius.
    By default, the cylinder has endpoints (0,0,0) and (1,0,0) and radius 0.1*/
class SrCylinder
 { public :
    SrPnt a, b;
    float radius;
    static const char* class_name; //!< constain the static string "Cylinder"

   public :

    /*! Constructs a cylinder with endpoints (0,0,0) and (1,0,0) and radius 1 */
    SrCylinder ();

    /*! Copy constructor */
    SrCylinder ( const SrCylinder& c );

    /*! Returns the bounding box of all vertices used. The returned box can be empty. */
    void get_bounding_box ( SrBox &b ) const;

    /*! Outputs in format "p1 p2 radius " */
    friend SrOutput& operator<< ( SrOutput& o, const SrCylinder& c );

    /*! Input from format "p1 p2 radius " */
    friend SrInput& operator>> ( SrInput& in, SrCylinder& c );
 };


//================================ End of File =================================================

# endif  // SR_SCENE_CYLINDER_H

