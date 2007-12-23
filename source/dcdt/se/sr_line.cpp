#include "precompiled.h"
#include "0ad_warning_disable.h"
# include <math.h>
# include "sr_box.h"
# include "sr_line.h"
# include "sr_input.h"
# include "sr_output.h"

//===========================================================================

// Important: Static initializations cannot use other static initialized
// variables ( as SrVec::i, ... ), as we cannot know the order that they
// will be initialized by the compiler.

const SrLine SrLine::x ( SrVec(0,0,0), SrVec(1.0f,0,0) );
const SrLine SrLine::y ( SrVec(0,0,0), SrVec(0,1.0f,0) );
const SrLine SrLine::z ( SrVec(0,0,0), SrVec(0,0,1.0f) );

//============================== SrLine ====================================

#define EPSILON 0.00001 // floats have 7 decimals

// Original code from : http://www.acm.org/jgt/papers/MollerTrumbore97/
bool SrLine::intersects_triangle ( const SrVec &v0, const SrVec &v1, const SrVec &v2,
                                   float &t, float &u, float &v ) const
 {
   SrVec dir, edge1, edge2, tvec, pvec, qvec;
   float det, inv_det;

   dir   = p2 - p1;
   edge1 = v1 - v0;                      // find vectors for two edges sharing v0 
   edge2 = v2 - v0;
   pvec  = cross ( dir, edge2 );         // begin calculating determinant - also used to calculate U parameter 
   det   = dot ( edge1, pvec );          // if determinant is near zero, ray lies in plane of triangle 
   //   printf("det=%f\n",det);

   if ( SR_NEXTZ(det,EPSILON) )
     { //sr_out.warning("det in ray_triangle fails => %f",(float)det);
       return false;
     }
   inv_det = 1.0f / det;

   tvec = p1 - v0;                       // calculate distance from v0 to ray origin 
   u = dot(tvec, pvec) * inv_det;        // calculate U parameter and test bounds 
   if ( u<0.0 || u>1.0 ) return false;

   qvec = cross ( tvec, edge1 );         // prepare to test V parameter 
   v = dot(dir, qvec) * inv_det;         // calculate V parameter and test bounds 
   if ( v<0.0 || u+v>1.0 ) return false;
   t = dot(edge2,qvec) * inv_det;        // calculate t, ray intersects triangle 

   return true;
 }

bool SrLine::intersects_square ( const SrVec &v1, const SrVec &v2,
                                 const SrVec &v3, const SrVec &v4, float& t ) const
 {
   float u, v;
   if ( intersects_triangle ( v1, v2, v3, t, u, v ) ) return true;
   if ( intersects_triangle ( v1, v3, v4, t, u, v ) ) return true;
   return false;
 }

int SrLine::intersects_box ( const SrBox& box, float& t1, float& t2, SrVec* vp ) const
 {
   SrVec p1, p2, p3, p4, p;
   float t[6];
   int side[6];
   int tsize=0;

   # define INTERSECT(s) if ( intersects_square(p1,p2,p3,p4,t[tsize]) ) { side[tsize]=s; tsize++; }

   box.get_side ( p1, p2, p3, p4, 0 );
   INTERSECT(0);
   box.get_side ( p1, p2, p3, p4, 1 );
   INTERSECT(1);
   box.get_side ( p1, p2, p3, p4, 2 );
   INTERSECT(2);
   box.get_side ( p1, p2, p3, p4, 3 );
   INTERSECT(3);
   box.get_side ( p1, p2, p3, p4, 4 );
   INTERSECT(4);
   box.get_side ( p1, p2, p3, p4, 5 );
   INTERSECT(5);

   # undef INTERSECT

   if ( tsize==0 )
    { t1=t2=0; }
   else if ( tsize==1 )
    { t1=t2=t[0]; }
   else if ( tsize==2 )
    { float tmpf;
      int tmpi;
      if ( t[1]<t[0] ) { SR_SWAPT(t[0],t[1],tmpf); SR_SWAPT(side[0],side[1],tmpi); }
      t1 = t[0];
      t2 = t[1];
    }
   else // sort according to t and take the two extremes
    { int i, j, tmpi;
      float tmpf;
      for ( i=0; i<tsize; i++ )
       for ( j=i; j<tsize; j++ )
        if ( t[j]<t[i] ) { SR_SWAPT(t[i],t[j],tmpf); SR_SWAPT(side[i],side[j],tmpi); }
      t1 = t[0];
      t2 = t[tsize-1];
      tsize = 2;
    }

   if (tsize>0 && vp) box.get_side ( vp[0], vp[1], vp[2], vp[3], side[0] );
   return tsize;
 }

int SrLine::intersects_sphere ( const SrPnt& center, float radius, SrPnt* vp ) const
 {
   // set up quadratic Q(t) = a*t^2 + 2*b*t + c
   SrVec dir = p2-p1;
   SrVec kdiff = p1 - center;
   float a = dir.norm2();
   float b = dot ( kdiff, dir );
   float c = kdiff.norm2() - radius*radius;

   float aft[2];
   float discr = b*b - a*c;

   if ( discr < 0.0f )
    { return 0;
    }
   else if ( discr > 0.0f )
    { float root = sqrtf(discr);
      float inva = 1.0f/a;
      aft[0] = (-b - root)*inva;
      aft[1] = (-b + root)*inva;
      if ( vp )
       { vp[0] = p1 + aft[0]*dir;
         vp[1] = p1 + aft[1]*dir;
         if ( dist2(vp[1],p1)<dist2(vp[0],p1) )
          { SrPnt tmp;
            SR_SWAP(vp[0],vp[1]);
          }
       }
      return 2;
    }
   else
    { aft[0] = -b/a;
      if ( vp ) vp[0] = p1 + aft[0]*dir;
      return 1;
     }
 }

//http://astronomy.swin.edu.au/~pbourke/geometry/pointline/source.c
SrPnt SrLine::closestpt ( SrPnt p, float* k ) const
 {
   SrVec v (p2-p1);
 
   float u = ( ( (p.x-p1.x) * (v.x) ) +
               ( (p.y-p1.y) * (v.y) ) +
               ( (p.z-p1.z) * (v.z) ) ) / ( v.norm2() );
 
   //if( u<0.0f || u>1.0f ) // closest point does not fall within the line segment
   if ( k ) *k=u;
   
   return p1 + u*v;
 }
 
//============================== friends ====================================

SrOutput& operator<< ( SrOutput& o, const SrLine& l )
 {
   return o << l.p1 <<" "<< l.p2;
 }

SrInput& operator>> ( SrInput& in, SrLine& l )
 {
   return in >> l.p1 >> l.p2;
 }

//============================= End of File ===========================================
