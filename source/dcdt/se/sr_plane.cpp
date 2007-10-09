#include "precompiled.h"
# include "sr_plane.h"

//============================== SrPlane ======================================

// Important: Static initializations cannot use other static initialized
// variables ( as SrVec::i, ... ), as we cannot know the order that they
// will be initialized by the compiler.

const SrPlane SrPlane::XY ( SrVec(0,0,0), SrVec(1.0f,0,0), SrVec(0,1.0f,0) );
const SrPlane SrPlane::XZ ( SrVec(0,0,0), SrVec(1.0f,0,0), SrVec(0,0,1.0f) );
const SrPlane SrPlane::YZ ( SrVec(0,0,0), SrVec(0,1.0f,0), SrVec(0,0,1.0f) );

SrPlane::SrPlane () : coords ( SrVec::k ), coordsw(0)
 {
 }

SrPlane::SrPlane ( const SrVec& center, const SrVec& normal )
 {
   set ( center, normal );
 }

SrPlane::SrPlane ( const SrVec& p1, const SrVec& p2, const SrVec& p3 )
 {
   set ( p1, p2, p3 );
 }

bool SrPlane::set ( const SrVec& center, const SrVec& normal )
 {
   coords = normal;
   coordsw = -dot(normal,center);
   float n = normal.norm();
   if (n==0.0) return false;
   coords/=n; coordsw/=n;
   return true;
 }

bool SrPlane::set ( const SrVec& p1, const SrVec& p2, const SrVec& p3 )
 {
   SrVec normal = cross ( p2-p1, p3-p1 );
   return set ( p1, normal );
 }

bool SrPlane::parallel ( const SrVec& p1, const SrVec& p2, float ds ) const
 {
   float fact = dot ( coords, p1-p2 );
   return SR_NEXTZ(fact,ds)? true:false;
 }

/*! Returns p, that is the intersection between plane and infinity line of <p1,p2>,
    (0,0,0) is returned if they are parallel. */
SrVec SrPlane::intersect ( const SrVec& p1, const SrVec& p2, float *t ) const
 {
   float fact = dot ( coords, p1-p2 );
   if ( fact==0.0 ) return SrVec::null;
   float k = (coordsw+dot(coords,p1)) / fact;
   if (t) *t=k;
   return lerp ( p1, p2, k );
 }

/* my old functions :

float Dist ( const OVec3f &p, const OVec4f &plane )
 {
   float c = plane.x*p.x + plane.y*p.y + plane.z*p.z + plane.w;
   return ABS(c);
 }

bool Dist ( float &dist, const OVec3f &p,
            const OVec3f &p1, const OVec3f &p2, const OVec3f &p3 )
 {
   OVec4f plane;
   if ( !PlaneCoords(p1,p2,p3,plane) ) return false;
   dist = ::Dist(p,plane);
   return true;
 }

float Dist ( const OVec3f &p, const OVec3f &p0, const OVec3f &n )
 {
   OVec3f w ( p-p0 );
   w = Cross ( n, w );
   return w.Norm();
 }

float IsParallel ( const OVec4f &plane,
                    const OVec3f &p1, const OVec3f &p2 )
 {
   OVec3f p(plane.x,plane.y,plane.z);
   float fact = Dot( p, p1-p2 );
   return ABS(fact);
 }

bool InterSeg ( const OVec4f &plane,
                const OVec3f &p1, const OVec3f &p2, OVec3f &p )
 {
   OVec3f pl(plane.x,plane.y,plane.z);
   float t, fact = Dot( pl, p1-p2 );
   if ( fact==0.0 ) return false;
   t = (plane.w+Dot(pl,p1)) / fact;
   if ( t<0.0 || t>1.0 ) return false;
   p = Evaluate( p1, p2, t );
   return true;
 }

*/

