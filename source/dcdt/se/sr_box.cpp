#include "precompiled.h"

# include "sr_box.h"
# include "sr_mat.h"

//======================================== SrBox =======================================

const char* SrBox::class_name = "Box";

SrBox::SrBox ( const SrBox& x, const SrBox& y )
      : a ( SR_MIN(x.a.x,y.a.x), SR_MIN(x.a.y,y.a.y), SR_MIN(x.a.z,y.a.z) ),
        b ( SR_MAX(x.b.x,y.b.x), SR_MAX(x.b.y,y.b.y), SR_MAX(x.b.z,y.b.z) ) 
 {
 }

bool SrBox::empty () const
 {
   return a.x>b.x || a.y>b.y || a.z>b.z ? true:false;
 }

float SrBox::volume () const
 {
   return (b.x-a.x) * (b.y-a.y) * (b.z-a.z);
 }

SrVec SrBox::center () const 
 { 
   return (a+b)/2.0f; // == a + (b-a)/2 == a + b/2 - a/2
 }

void SrBox::center ( const SrVec& p )
 {
   (*this) += p-center();
 }

void SrBox::size ( const SrVec& v )
 {
   b = a+v;
 }

SrVec SrBox::size () const 
 { 
   return b-a; 
 }

float SrBox::max_size () const
 {
   SrVec s = b-a;
   return SR_MAX3(s.x,s.y,s.z);
 }

float SrBox::min_size () const
 {
   SrVec s = b-a;
   return SR_MIN3(s.x,s.y,s.z);
 }

void SrBox::extend ( const SrPnt &p )
 {
   if ( empty() ) { a=p; b=p; }
   SR_UPDMIN ( a.x, p.x ); SR_UPDMAX ( b.x, p.x );
   SR_UPDMIN ( a.y, p.y ); SR_UPDMAX ( b.y, p.y );
   SR_UPDMIN ( a.z, p.z ); SR_UPDMAX ( b.z, p.z );
 }

void SrBox::extend ( const SrBox &box )
 {
   if ( empty() ) *this=box;
   if ( box.empty() ) return;
   SR_UPDMIN ( a.x, box.a.x ); SR_UPDMAX ( b.x, box.b.x );
   SR_UPDMIN ( a.y, box.a.y ); SR_UPDMAX ( b.y, box.b.y );
   SR_UPDMIN ( a.z, box.a.z ); SR_UPDMAX ( b.z, box.b.z );
 }

void SrBox::grows ( float dx, float dy, float dz )
 {
   a.x-=dx; a.y-=dy; a.z-=dz;
   b.x+=dx; b.y+=dy; b.z+=dz;
 }

bool SrBox::contains ( const SrVec& p ) const
 {
   return p.x<a.x || p.y<a.y || p.z<a.z || p.x>b.x || p.y>b.y || p.z>b.z ? false : true;
 }

bool SrBox::intersects ( const SrBox& box ) const
 {
   if ( box.contains(a) ) return true;
   if ( box.contains(b) ) return true;
   SrVec x(a.x,a.y,b.z); if ( box.contains(x) ) return true;
   x.set  (a.x,b.y,a.z); if ( box.contains(x) ) return true;
   x.set  (b.x,a.y,a.z); if ( box.contains(x) ) return true;
   x.set  (b.x,b.y,a.z); if ( box.contains(x) ) return true;
   x.set  (b.x,a.y,b.z); if ( box.contains(x) ) return true;
   x.set  (a.x,b.y,b.z); if ( box.contains(x) ) return true;
   return false;
 }

void SrBox::get_side ( SrPnt& p1, SrPnt& p2, SrPnt& p3, SrPnt& p4, int s ) const
 {
   switch (s)
    { case 0 : p1.set ( a.x, a.y, a.z );
               p2.set ( a.x, a.y, b.z );
               p3.set ( a.x, b.y, b.z );
               p4.set ( a.x, b.y, a.z );
               break;
      case 1 : p1.set ( b.x, a.y, a.z );
               p2.set ( b.x, b.y, a.z );
               p3.set ( b.x, b.y, b.z );
               p4.set ( b.x, a.y, b.z );
               break;
      case 2 : p1.set ( a.x, a.y, a.z );
               p2.set ( b.x, a.y, a.z );
               p3.set ( b.x, a.y, b.z );
               p4.set ( a.x, a.y, b.z );
               break;
      case 3 : p1.set ( a.x, b.y, a.z );
               p2.set ( a.x, b.y, b.z );
               p3.set ( b.x, b.y, b.z );
               p4.set ( b.x, b.y, a.z );
               break;
      case 4 : p1.set ( a.x, a.y, a.z );
               p2.set ( a.x, b.y, a.z );
               p3.set ( b.x, b.y, a.z );
               p4.set ( b.x, a.y, a.z );
               break;
      case 5 : p1.set ( a.x, a.y, b.z );
               p2.set ( b.x, a.y, b.z );
               p3.set ( b.x, b.y, b.z );
               p4.set ( a.x, b.y, b.z );
               break;
    }
 }

void SrBox::operator += ( const SrVec& v )
 {
   a += v;
   b += v;
 }

void SrBox::operator *= ( float s )
 {
   a *= s;
   b *= s;
 }

//============================== friends ========================================

SrBox operator * ( const SrBox& b, const SrMat& m )
 {
   SrBox x; // init as an empty box

   if ( b.empty() ) return x;

   SrVec v(b.a); x.extend(v*m);
   v.x=b.b.x; x.extend(v*m);
   v.y=b.b.y; x.extend(v*m);
   v.x=b.a.x; x.extend(v*m);
   v.z=b.b.z; x.extend(v*m);
   v.x=b.b.x; x.extend(v*m);
   v.y=b.a.y; x.extend(v*m);
   v.x=b.a.x; x.extend(v*m);
   return x;
 }

SrBox operator * ( const SrMat& m, const SrBox& b )
 {
   SrBox x; // init as an empty box

   if ( b.empty() ) return x;

   SrVec v(b.a); x.extend(m*v);
   v.x=b.b.x; x.extend(m*v);
   v.y=b.b.y; x.extend(m*v);
   v.x=b.a.x; x.extend(m*v);
   v.z=b.b.z; x.extend(m*v);
   v.x=b.b.x; x.extend(m*v);
   v.y=b.a.y; x.extend(m*v);
   v.x=b.a.x; x.extend(m*v);
   return x;
 }

SrOutput& operator<< ( SrOutput& o, const SrBox& box )
 {
   return o << box.a << ' ' << box.b;
 }

SrInput& operator>> ( SrInput& in, SrBox& box )
 {
   return in >> box.a >> box.b;
 }

//================================ End of File =================================================
