#include "precompiled.h"
# include "sr_box.h"
# include "sr_mat.h"
# include "sr_vec2.h"
# include "sr_lines.h"

//# define SR_USE_TRACE1 // Constructor and Destructor
# include "sr_trace.h"

//======================================= SrLines ====================================

const char* SrLines::class_name = "Lines";

SrLines::SrLines ()
 {
   SR_TRACE1 ( "Constructor" );
 }

SrLines::~SrLines ()
 {
   SR_TRACE1 ( "Destructor" );
 }

void SrLines::init ()
 {
   V.size(0);
   C.size(0);
   I.size(0);
 }

void SrLines::compress ()
 {
   V.compress();
   C.compress();
   I.compress();
 }

void SrLines::push_line ( const SrVec &p1, const SrVec &p2 )
 { 
   V.push()=p1;
   V.push()=p2;
 }

void SrLines::push_line ( const SrVec2 &p1, const SrVec2 &p2 )
 {
   V.push().set(p1.x,p1.y,0);
   V.push().set(p2.x,p2.y,0);
 }

void SrLines::push_line ( float ax, float ay, float az, float bx, float by, float bz )
 {
   V.push().set(ax,ay,az);
   V.push().set(bx,by,bz);
 }

void SrLines::push_line ( float ax, float ay, float bx, float by )
 {
   V.push().set(ax,ay,0);
   V.push().set(bx,by,0);
 }

void SrLines::begin_polyline ()
 {
   I.push() = V.size();
 }

void SrLines::end_polyline ()
 {
   I.push() = V.size()-1;
 }

void SrLines::push_vertex ( const SrVec& p )
 { 
   V.push() = p; 
 }

void SrLines::push_vertex ( const SrVec2& p )
 { 
   V.push().set(p.x,p.y,0);
 }

void SrLines::push_vertex ( float x, float y, float z )
 { 
   V.push().set(x,y,z); 
 }

void SrLines::push_color ( const SrColor &c )
 {
   I.push() = V.size();
   C.push() = c;
   I.push() = -C.size();
 }

void SrLines::push_cross ( SrVec2 c, float r )
 {
   SrVec2 p(r,r);
   push_line ( c-p, c+p ); p.x*=-1;
   push_line ( c-p, c+p );
 }

void SrLines::push_axis ( const SrPnt& orig, float len, int dim, const char* let,
                          bool rule, SrBox* box )
 {
   float r, mr;
   float a, b, c, k;
   const float z = 0.0f;

   r = box? box->max_size()/2.0f : len;
   mr = -r;
   a=r/25.0f;  b=a/2.0f; c=a*3.0f; k=a/3.0f;

   bool letx=false, lety=false, letz=false;
   int vi = V.size();

   if ( let )
    while ( *let )
     { char c = SR_UPPER(*let);
       if ( c=='X' ) letx=true;
        else if ( c=='Y' ) lety=true;
         else if ( c=='Z' ) letz=true;
       let++;
     }

   if ( dim>=1 )
    { if ( box ) { mr=box->a.x; r=box->b.x; }
      push_color ( SrColor::red );
      push_line ( mr,  z,  z, r, z, z ); // X axis
      if ( letx && r>0 )
       { push_line (   r, -a, z, r-a, -c,    z ); // Letter X
         push_line ( r-a, -a, z, r,   -c,    z );
       }
      if ( rule && r>1.0 )
       { int i; int mini=SR_CEIL(mr); int maxi=SR_FLOOR(r);
         push_color ( SrColor::red );
         for ( i=mini; i<maxi; i++ )
          { if ( i==0 ) continue;
            push_line (  (float)i,  z,  z,  (float)i, k, z );
          }
       }
    }

   if ( dim>=2 )
    { if ( box ) { mr=box->a.y; r=box->b.y; }
      push_color ( SrColor::green );
      push_line (  z, mr,  z, z, r, z ); // Y axis
      if ( lety && r>0 )
       { push_line (   a,  r, z, a+b, r-a,   z ); // Letter Y
         push_line ( a+a,  r, z, a,   r-a-a, z );
       }
      if ( rule && r>1.0 )
       { int i; int mini=SR_CEIL(mr); int maxi=SR_FLOOR(r);
         push_color ( SrColor::green );
         for ( i=mini; i<maxi; i++ )
          { if ( i==0 ) continue;
            push_line (  z,   (float)i,  z, -k,  (float)i, z );
          }
       }
    }

   if ( dim>=3 ) 
    { if ( box ) { mr=box->a.z; r=box->b.z; }
      push_color ( SrColor::blue );
      push_line (  z,  z, mr, z, z, r ); // Z axis
      if ( letz && r>0 )
       { begin_polyline ();
         push_vertex ( z, -a, r-a ); // Letter Z
         push_vertex ( z, -a,   r ); 
         push_vertex ( z, -c, r-a ); 
         push_vertex ( z, -c,   r );
         end_polyline ();
       }
      if ( rule && r>1.0 )
       { int i; int mini=SR_CEIL(mr); int maxi=SR_FLOOR(r);
         push_color ( SrColor::blue );
         for ( i=mini; i<maxi; i++ )
          { if ( i==0 ) continue;
            push_line (  z,  z,  (float)i,  z, k,  (float)i );
          }
       }
    }

   if ( orig!=SrPnt::null )
    { int i;
      for ( i=vi; i<V.size(); i++ ) V[i]+=orig;
    }
 }

void SrLines::push_box ( const SrBox& box, bool multicolor )
 {
   const SrPnt& a = box.a;
   const SrPnt& b = box.b;

   if ( multicolor ) push_color ( SrColor::red );
   push_line ( a.x, a.y, a.z, b.x, a.y, a.z );
   push_line ( a.x, a.y, b.z, b.x, a.y, b.z );
   push_line ( a.x, b.y, a.z, b.x, b.y, a.z );
   push_line ( a.x, b.y, b.z, b.x, b.y, b.z );

   if ( multicolor ) push_color ( SrColor::green );
   push_line ( a.x, a.y, a.z, a.x, b.y, a.z );
   push_line ( a.x, a.y, b.z, a.x, b.y, b.z );
   push_line ( b.x, a.y, b.z, b.x, b.y, b.z );
   push_line ( b.x, a.y, a.z, b.x, b.y, a.z );

   if ( multicolor ) push_color ( SrColor::blue );
   push_line ( a.x, a.y, a.z, a.x, a.y, b.z );
   push_line ( a.x, b.y, a.z, a.x, b.y, b.z );
   push_line ( b.x, b.y, a.z, b.x, b.y, b.z );
   push_line ( b.x, a.y, a.z, b.x, a.y, b.z );
 }

void SrLines::push_polyline ( const SrArray<SrVec2>& a )
 {
   int i;
   if ( a.size()<2 ) return;
   I.push() = V.size();
   for ( i=0; i<a.size(); i++ ) V.push().set(a[i].x,a[i].y,0);
   I.push() = V.size()-1;
 }

void SrLines::push_polygon ( const SrArray<SrVec2>& a )
 {
   int i;
   if ( a.size()<2 ) return;
   I.push() = V.size();
   for ( i=0; i<a.size(); i++ ) V.push().set(a[i].x,a[i].y,0);
   V.push().set(a[0].x,a[0].y,0);
   I.push() = V.size()-1;
 }

void SrLines::push_lines ( const SrArray<SrVec2>& a )
 {
   int i;
   for ( i=0; i<a.size(); i++ ) V.push().set(a[i].x,a[i].y,0);
 }

void SrLines::push_circle_approximation ( const SrPnt& center, const SrVec& radius,
                                          const SrVec& normal, int nvertices )
 {
   SrVec x = radius; // rotating vec to draw the circle
   SrVec x1st = x;

   SrMat mr;
   float dr = sr2pi / (float)nvertices;
   mr.rot ( normal, dr );

   begin_polyline ();
   while ( nvertices-->0 )
    { push_vertex ( center + x );
      x = x*mr;
    }
   push_vertex ( center+x1st ); // to make it close exactly
   end_polyline ();
 }

void SrLines::get_bounding_box ( SrBox& b ) const
 { 
   int i;
   b.set_empty ();
   for ( i=0; i<V.size(); i++ ) 
    { b.extend ( V[i] );
    }
 }

//================================ EOF =================================================
