#include "precompiled.h"
# include "sr_cylinder.h"
# include "sr_box.h"

//================================== SrCylinder ====================================

const char* SrCylinder::class_name = "Cylinder";

SrCylinder::SrCylinder () : a(SrVec::null), b(SrVec::i)
 {
   radius = 0.1f;
 }

SrCylinder::SrCylinder ( const SrCylinder& c ) : a(c.a), b(c.b)
 {
   radius = c.radius;
 }

void SrCylinder::get_bounding_box ( SrBox& box ) const
 { 
   SrVec va = b-a; 
   va.normalize();
   SrVec vr1;
   if ( angle(SrVec::i,va)<0.1f )
     vr1 = cross ( SrVec::j, va );
   else
     vr1 = cross ( SrVec::i, va );
   
   SrVec vr2 = cross ( vr1, va );

   vr1.len ( radius );
   vr2.len ( radius );

   box.set_empty();
   box.extend ( a+vr1 );
   box.extend ( a-vr1 );
   box.extend ( a+vr2 );
   box.extend ( a-vr2 );
   box.extend ( b+vr1 );
   box.extend ( b-vr1 );
   box.extend ( b+vr2 );
   box.extend ( b-vr2 );
 }

SrOutput& operator<< ( SrOutput& o, const SrCylinder& c )
 {
   return o << c.a << srspc << c.b << srspc << c.radius;
 }

SrInput& operator>> ( SrInput& in, SrCylinder& c )
 {
   return in >> c.a >> c.b >> c.radius;
 }

//================================ EOF =================================================
