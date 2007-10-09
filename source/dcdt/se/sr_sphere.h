
# ifndef SR_SPHERE_H
# define SR_SPHERE_H

/** \file sr_sphere.h 
 * a sphere
 */

# include "sr_vec.h"

class SrBox;

/*! \class SrSphere sr_sphere.h
    \brief a sphere

    SrSphere represents a sphere based on its center and radius.
    By default, the sphere has center (0,0,0) and radius 1*/
class SrSphere
 { public :
    SrPnt center;  
    float radius;
    static const char* class_name; //!< constain the static string "Sphere"

   public :

    /*! Constructs as a sphere centered at (0,0,0) with radius 1 */
    SrSphere ();

    /*! Copy constructor */
    SrSphere ( const SrSphere& s );

    /* Returns the bounding box of all vertices used. The returned box can be empty. */
    void get_bounding_box ( SrBox &b ) const;

    /*! Outputs in format: "center radius". */
    friend SrOutput& operator<< ( SrOutput& o, const SrSphere& sph );

    /*! Input from format: "center radius". */
    friend SrInput& operator>> ( SrInput& in, SrSphere& sph );
 };


//================================ End of File =================================================

# endif  // SR_SCENE_SPHERE_H

