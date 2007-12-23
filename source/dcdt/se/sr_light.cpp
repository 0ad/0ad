#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_light.h"

//===================================== SrLight ================================


SrLight::SrLight ()
 {
   init ();
 }

void SrLight::init ()
 {
   spot_exponent = 0;
   spot_direction.set ( 0, 0, -1.0f );
   spot_cutoff = 180;
   constant_attenuation = 1.0f;
   linear_attenuation = 0;
   quadratic_attenuation = 0;
   ambient = SrColor::black;
   diffuse = SrColor::white;
   specular = SrColor::black;
   position = SrVec::k;
   directional = true;
 }



//================================ End of File =================================================
