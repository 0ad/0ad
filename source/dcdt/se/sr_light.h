
# ifndef SR_LIGHT_H
# define SR_LIGHT_H

/** \file sr_light.h 
 * Keeps light parameters
 */

# include "sr_vec.h"
# include "sr_color.h"

/*! \class SrLight sr_light.h
    \brief Keeps light parameters

    SrLight Keeps light parameters following the OpenGL specifications.
    Note however that this class only stores data and has no dependency on
    OpenGL functions. */
class SrLight
 { public :

    /*! Higher spot exponents result in a more focused light source. The default 
        spot exponent is 0, resulting in uniform light distribution. Values must
        be in the range of [0,128]. */
    float spot_exponent;

    /*! The direction of the light in homogeneous object coordinates. The spot 
        direction is transformed by the inverse of the modelview matrix when 
        glLight is called (just as if it was a normal), and it is stored in 
        eye coordinates. It is significant only when GL_SPOT_CUTOFF is not 
        180, which it is by default. The default direction is (0,0,-1). */
    SrVec spot_direction;

    /*! Specifies the maximum spread angle. Only values in the range [0,90], 
        and the special value 180, are accepted. If the angle between the 
        direction of the light and the direction from the light to the vertex
        being lighted is greater than the spot cutoff angle, then the light 
        is completely masked. Otherwise, its intensity is controlled by the 
        spot exponent and the attenuation factors. The default spot cutoff 
        is 180, resulting in uniform light distribution. */
    float spot_cutoff;

    /*! If the light is positional, rather than directional, its intensity 
        is attenuated by the reciprocal of the sum of: the constant factor,
        the linear factor multiplied by the distance between the light and
        the vertex being lighted, and the quadratic factor multiplied by 
        the square of the same distance. The default attenuation factors
        are cte=1, linear=0, quad=0, resulting in no attenuation. Only 
        nonnegative values are accepted. */
    float constant_attenuation;
    float linear_attenuation;     //!< See constant_attenuation. 
    float quadratic_attenuation;  //!< See constant_attenuation. 

    SrColor ambient;  //!< Default is black
    SrColor diffuse;  //!< Default is white
    SrColor specular; //!< Default is black

    /*! The position is transformed by the modelview matrix when glLight is
        called (just as if it was a point), and it is stored in eye 
        coordinates. If directional, diffuse and specular lighting calculations
        take the lights direction, but not its actual position, into account,
        and attenuation is disabled. Otherwise, diffuse and specular lighting 
        calculations are based on the actual location of the light in eye
        coordinates, and attenuation is enabled. The default position is (0,0,1). */
    SrVec position;

    /*! When true means that the position w coord is 0, otherwise 1. Default is true. */
    bool directional;

   public :
    
    /*! Initialize the camera with the default parameters. */
    SrLight ();

    /*! Sets the default parameters. */
    void init ();
 };


//================================ End of File =================================================

# endif // SR_LIGHT_H

