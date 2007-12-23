#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_sphere.h"
# include "sr_box.h"

//================================== SrSphere ====================================

const char* SrSphere::class_name = "Sphere";

SrSphere::SrSphere () : center(SrPnt::null)
 {
   radius = 1.0f;
 }

SrSphere::SrSphere ( const SrSphere& s ) : center(s.center)
 {
   radius = s.radius;
 }

void SrSphere::get_bounding_box ( SrBox& box ) const
 { 
   SrVec r ( radius, radius, radius );
   box.set ( center-r, center+r );
 }

SrOutput& operator<< ( SrOutput& o, const SrSphere& sph )
 {
   return o << sph.center << ' ' << sph.radius;
 }

SrInput& operator>> ( SrInput& in, SrSphere& sph )
 {
   return in >> sph.center >> sph.radius;
 }

//================================ EOF =================================================
