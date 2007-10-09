
/** \file sr_material.h
 * specifies a material
 */

# ifndef SR_MATERIAL_H
# define SR_MATERIAL_H

# include "sr_input.h" 
# include "sr_output.h" 

# include "sr_color.h"

/*! \class SrMaterial sr_material.h
    \brief specifies a material

    Defines a material. */
class SrMaterial
 { public :
    SrColor ambient;   //!< default in float coords: 0.2, 0.2, 0.2, 1.0
    SrColor diffuse;   //!< default in float coords: 0.8, 0.8, 0.8, 1.0
    SrColor specular;  //!< default in float coords: 0.0, 0.0, 0.0, 1.0
    SrColor emission;  //!< default in float coords: 0.0, 0.0, 0.0, 1.0
    srbyte  shininess; //!< default: 0, can be in : [0,128]
   public :
    
    /*! Initializes with the default values. */
    SrMaterial ();

    /*! Set again the default values. Note that .2 is mapped
        to 51, and .8 to 204 in the SrColor format. */
    void init ();

    /*! Exact comparison operator == */
    friend bool operator == ( const SrMaterial& m1, const SrMaterial& m2 );

    /*! Exact comparison operator != */
    friend bool operator != ( const SrMaterial& m1, const SrMaterial& m2 );

    /*! Outputs in format: "ar ag ab aa dr dg db da sr sg sb sa er eg eb ea s". */
    friend SrOutput& operator<< ( SrOutput& o, const SrMaterial& m );

    /*! Inputs from format: "ar ag ab aa dr dg db da sr sg sb sa er eg eb ea s". */
    friend SrInput& operator>> ( SrInput& in, SrMaterial& m );
 };

//================================ End of File =================================================

# endif // SR_MATERIAL_H
