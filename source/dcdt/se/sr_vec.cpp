#include "precompiled.h"
#include "0ad_warning_disable.h"
# include <math.h>
# include "sr_vec.h"
# include "sr_vec2.h"

//===========================================================================

const SrVec SrVec::null(0,0,0);
const SrVec SrVec::minusone(-1.0f,-1.0f,-1.0f);
const SrVec SrVec::one(1.0f,1.0f,1.0f);
const SrVec SrVec::i(1.0f,0,0);
const SrVec SrVec::j(0,1.0f,0);
const SrVec SrVec::k(0,0,1.0f);

//============================== SrVec ====================================

SrVec::SrVec ( const SrVec2& v )
 { 
   x = v.x;
   y = v.y;
   z = 0;
 }

void SrVec::set ( const SrVec2& v )
 { 
   x = v.x;
   y = v.y;
   z = 0;
 }

void SrVec::rotx ( float sa, float ca )
 {
   set ( x, y*ca-z*sa, y*sa+z*ca );
 }

void SrVec::roty ( float sa, float ca )
 {
   set ( x*ca+z*sa, y, -x*sa+z*ca );
 }

void SrVec::rotz ( float sa, float ca )
 {
   set ( x*ca-y*sa, x*sa+y*ca, z );
 }

void SrVec::rotx ( float radians )
 {
   rotx ( sinf(radians), cosf(radians) );
 }

void SrVec::roty ( float radians )
 {
   roty ( sinf(radians), cosf(radians) );
 }

void SrVec::rotz ( float radians )
 {
   rotz ( sinf(radians), cosf(radians) );
 }

void SrVec::abs ()
 {
   x=SR_ABS(x); y=SR_ABS(y); z=SR_ABS(z); 
 }

void SrVec::normalize ()
 {
   float f = sqrtf(x*x + y*y + z*z);
   if ( f>0 ) { x/=f; y/=f; z/=f; }
 }

float SrVec::len ( float n )
 {
   float f = sqrtf (x*x + y*y + z*z);
   if ( f>0 ) { n/=f; x*=n; y*=n; z*=n; }
   return f;
 }

float SrVec::norm () const
 {
   return sqrtf ( x*x + y*y + z*z );
 }

float SrVec::norm2 () const
 {
   return x*x + y*y + z*z;
 }

float SrVec::norm_max () const
 {
   float a = SR_ABS(x);
   float b = SR_ABS(y);
   float c = SR_ABS(z);
   return SR_MAX3 ( a, b, c );
 }

//=================================== Friend Functions ===================================

void swap ( SrVec& v1, SrVec& v2 )
 {
   float tmp;
   SR_SWAP(v1.x,v2.x);
   SR_SWAP(v1.y,v2.y);
   SR_SWAP(v1.z,v2.z);
 }

float dist_max ( const SrVec& v1, const SrVec& v2 )
 {
   float a = v1.x-v2.x;
   float b = v1.y-v2.y;
   float c = v1.z-v2.z;
   a = SR_ABS(a);
   b = SR_ABS(b);
   c = SR_ABS(c);
   return a>b? (a>c? a:c) : (b>c? b:c);
 }

float dist ( const SrVec& v1, const SrVec& v2 )
 {
   float dx, dy, dz;

   dx=v1.x-v2.x; dy=v1.y-v2.y; dz=v1.z-v2.z;

   return sqrtf (dx*dx + dy*dy + dz*dz);
 }

float dist2 ( const SrVec& v1, const SrVec& v2 )
 {
   float dx, dy, dz;
   dx=v1.x-v2.x; dy=v1.y-v2.y; dz=v1.z-v2.z;
   return dx*dx + dy*dy + dz*dz;
 }

float angle ( const SrVec& v1, const SrVec& v2 )
 {
   return acosf ( dot(v1,v2)/(v1.norm()*v2.norm()) );
 }

float angle_fornormvecs ( const SrVec& v1, const SrVec& v2 )
 {
   return acosf ( dot(v1,v2) );
 }

SrVec cross ( const SrVec& v1, const SrVec& v2 )
 {
   return SrVec ( v1.y*v2.z - v1.z*v2.y,
                  v1.z*v2.x - v1.x*v2.z,
                  v1.x*v2.y - v1.y*v2.x  );
 }

void SrVec::cross ( const SrVec& v1, const SrVec& v2 )
 {
   x = v1.y*v2.z - v1.z*v2.y;
   y = v1.z*v2.x - v1.x*v2.z;
   z = v1.x*v2.y - v1.y*v2.x;
 }

float dot ( const SrVec& v1, const SrVec& v2 )
 {
   return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
 }

SrVec triangle_normal ( const SrPnt& a, const SrPnt& b, const SrPnt& c )
 { 
   SrVec n; 
   n.cross ( b-a, c-a ); 
   n.normalize(); 
   return n; 
 }

int compare ( const SrVec& v1, const SrVec& v2 )
 {
   if ( v1.x > v2.x ) return  1;
   if ( v1.x < v2.x ) return -1;
   if ( v1.y > v2.y ) return  1;
   if ( v1.y < v2.y ) return -1;
   if ( v1.z > v2.z ) return  1;
   if ( v1.z < v2.z ) return -1;
   return 0;
 }

int compare ( const SrVec* v1, const SrVec* v2 )
 {
   return compare ( *v1, *v2 );
 }

SrVec lerp ( const SrVec& v1, const SrVec& v2, float t )
 {
   return v1*(((float)1.0)-t) + v2*t;
 }

SrOutput& operator<< ( SrOutput& o, const SrVec& v )
 {
   return o << v.x <<' '<< v.y <<' '<< v.z;
 }

SrInput& operator>> ( SrInput& in, SrVec& v )
 {
   return in >> v.x >> v.y >> v.z;
 }

//================================== End of File ===========================================

