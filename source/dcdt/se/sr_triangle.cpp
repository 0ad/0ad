#include "precompiled.h"
#include "0ad_warning_disable.h"
//# include <math.h>
# include "sr_triangle.h"
//# include "sr_vec2.h"

// ==================== static funcs ====================================

#define EPSILON srtiny

static void matinverse ( float M[9], const SrVec &l1, const SrVec &l2, const SrVec &l3 )
 {
   M[0]=l2.y*l3.z-l2.z*l3.y; M[3]=l2.z*l3.x-l2.x*l3.z; M[6]=l2.x*l3.y-l2.y*l3.x;
   M[1]=l1.z*l3.y-l1.y*l3.z; M[4]=l1.x*l3.z-l1.z*l3.x; M[7]=l1.y*l3.x-l1.x*l3.y;
   M[2]=l1.y*l2.z-l1.z*l2.y; M[5]=l1.z*l2.x-l1.x*l2.z; M[8]=l1.x*l2.y-l1.y*l2.x;

   float d = l1.x*M[0] + l1.y*M[3] + l1.z*M[6];
   SR_ASSERT ( d<-EPSILON || d>EPSILON ); // check if singular matrix
   d = 1.0f/d;

   M[0]*=d; M[1]*=d; M[2]*=d; M[3]*=d; M[4]*=d; M[5]*=d; M[6]*=d; M[7]*=d; M[8]*=d;
 }

static void matposmult ( const SrVec& v, const float M[9], SrVec &r ) // r = v M
 {
   r.x = v.x*M[0] + v.y*M[3] + v.z*M[6];
   r.y = v.x*M[1] + v.y*M[4] + v.z*M[7];
   r.z = v.x*M[2] + v.y*M[5] + v.z*M[8];
 }

//============================== SrTriangle ====================================

/*! The following version was tested with 2d triangle and works:
    Returns k, such that a*k.x + b*k.y + c*k.z == p, k.x+k.y+k.z==1 */
/*    
static SrVec barycentric ( const SrPnt2& a, const SrPnt2& b, const SrPnt2& c, const SrPnt2& p )
 {
   # define DET3(a,b,c,d,e,f,g,h,i) a*e*i +b*f*g +d*h*c -c*e*g -b*d*i -a*f*h
   float A  = DET3 ( a.x, b.x, c.x, a.y, b.y, c.y, 1, 1, 1 );
   float A1 = DET3 ( p.x, b.x, c.x, p.y, b.y, c.y, 1, 1, 1 );
   float A2 = DET3 ( a.x, p.x, c.x, a.y, p.y, c.y, 1, 1, 1 );
   float A3 = DET3 ( a.x, b.x, p.x, a.y, b.y, p.y, 1, 1, 1 );
   return SrVec ( A1/A, A2/A, A3/A );
   # undef DET3
 }*/

SrVec SrTriangle::barycentric ( const SrVec &p ) const
 {
   float m[9];
   SrVec k;
   matinverse ( m, a, b, c );
   matposmult ( m, p, k );
   return k;
 }

void SrTriangle::translate ( const SrVec &k, const SrVec& v )
 {
   float k2 = k.x*k.x + k.y*k.y + k.z*k.z;

   a += (k.x+1.0f-k2)*v;
   b += (k.y+1.0f-k2)*v;
   c += (k.z+1.0f-k2)*v;
 }

void SrTriangle::translate ( const SrVec& v )
 {
   a += v;
   b += v;
   c += v;
 }

SrVec SrTriangle::normal () const
 { 
   SrVec n; 
   n.cross ( b-a, c-a ); 
   n.normalize(); 
   return n; 
 }

SrOutput& operator<< ( SrOutput& o, const SrTriangle& t )
 {
   return o << t.a <<','<< t.b <<','<< t.c;
 }

SrInput& operator>> ( SrInput& in, SrTriangle& t )
 {
   in >> t.a; in.getd();
   in >> t.b; in.getd();
   in >> t.c;
   return in;
 }

//================================== End of File ===========================================

