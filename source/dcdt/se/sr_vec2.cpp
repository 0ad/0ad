#include "precompiled.h"
# include <math.h>
# include "sr_vec2.h"
# include "sr_geo2.h"

//============================= static data ================================

const SrVec2 SrVec2::null(0,0);
const SrVec2 SrVec2::minusone(-1.0f,-1.0f);
const SrVec2 SrVec2::one(1.0f,1.0f);
const SrVec2 SrVec2::i(1.0f,0);
const SrVec2 SrVec2::j(0,1.0f);

//============================== SrVec2 ====================================

void SrVec2::rot ( float sa, float ca )
 {
   set ( x*ca-y*sa, x*sa+y*ca );
 }

void SrVec2::rot ( const SrVec2& cent, float sa, float ca )
 {
   x-=cent.x; y-=cent.y;
   set ( x*ca-y*sa, x*sa+y*ca );
   x+=cent.x; y+=cent.y;
 }

void SrVec2::rot ( float radians )
 {
   rot ( sinf(radians), cosf(radians) );
 }

void SrVec2::rot ( const SrVec2& cent, float radians )
 {
   rot ( cent, sinf(radians), cosf(radians) );
 }

void SrVec2::abs ()
 {
   x=SR_ABS(x); y=SR_ABS(y); 
 }

void SrVec2::normalize ()
 {
   float f = x*x + y*y;
   if ( f==1.0 || f==0.0 ) return;
   f = sqrtf ( f );
   x/=f; y/=f;
 }

float SrVec2::len ( float n )
 {
   float f = sqrtf (x*x + y*y);
   if ( f>0 ) { n/=f; x*=n; y*=n; }
   return f;
 }

float SrVec2::norm () const
 {
   float f = x*x + y*y;
   if ( f==1.0 || f==0.0 ) return f;
   return sqrtf ( f );
 }


float SrVec2::norm_max () const
 {
   float a = SR_ABS(x);
   float b = SR_ABS(y);
   return SR_MAX ( a, b );
 }

float SrVec2::angle () const 
 {
   float ang=atan2f(y,x);
   if ( ang<0 ) ang += sr2pi;
   return ang;
 }

float SrVec2::angle_max () const 
 {
   float comp, b, a;
   a=SR_ABS(x); b=SR_ABS(y);
   if (b==0.0 && x>=0.0) return 0.0;
   comp = b>=a? ((float)2.0)-(a/b) : (b/a);
   if (x<0.0) comp = ((float)4.0)-comp;
   if (y<0.0) comp = ((float)8.0)-comp;
   return comp;
 }

//=================================== Friend Functions ===================================

void swap ( SrVec2 &v1, SrVec2 &v2 )
 {
   float tmp;
   SR_SWAP(v1.x,v2.x);
   SR_SWAP(v1.y,v2.y);
 }

float dist_max ( const SrVec2 &v1, const SrVec2 &v2 )
 {
   float a = v1.x-v2.x;
   float b = v1.y-v2.y;
   a = SR_ABS(a);
   b = SR_ABS(b);
   return SR_MAX(a,b);
 }

float dist ( const SrVec2 &v1, const SrVec2 &v2 )
 {
   float dx, dy;

   dx=v1.x-v2.x; dy=v1.y-v2.y;

   return sqrtf (dx*dx + dy*dy);
 }

float dist2 ( const SrVec2 &v1, const SrVec2 &v2 )
 {
   float dx, dy;
   dx=v1.x-v2.x; dy=v1.y-v2.y;
   return dx*dx + dy*dy;
 }

float angle ( const SrVec2 &v1, const SrVec2 &v2 )
 {
   return acosf ( dot(v1,v2)/(v1.norm()*v2.norm()) );
 }

float angle_fornormvecs ( const SrVec2 &v1, const SrVec2 &v2 )
 {
   return acosf ( dot(v1,v2) );
 }

float angle_ori ( const SrVec2& v1, const SrVec2& v2 ) // (-pi,pi]
 {
   float a = acosf ( dot(v1,v2)/(v1.norm()*v2.norm()) );
   if ( SR_CCW(v1.x,v1.y,0,0,v2.x,v2.y)>0 ) a=-a;
   return a;
 }

float angle_max ( const SrVec2 &v1, const SrVec2 &v2 ) // [0,4]
 {
   float a1 = v1.angle_max();
   float a2 = v2.angle_max();
   float a = a1>a2? a1-a2:a2-a1;
   if ( a>4.0f ) a = 8.0f-a;
   return a;
 }

float angle_max_ori ( const SrVec2 &v1, const SrVec2 &v2 ) // (-4,4]
 {
   float a = v2.angle_max() - v1.angle_max();

//   sr_out<<" ["<<v2.angle_max()<<","<< v1.angle_max()<<"] ";
   if ( a>4.0f ) a -= 8.0f;
   if ( a<=-4.0f ) a += 8.0f;
   return a;
 }

float cross ( const SrVec2 &v1, const SrVec2 &v2 )
 {
   return v1.x*v2.y - v1.y*v2.x;
 }

float dot ( const SrVec2 &v1, const SrVec2 &v2 )
 {
   return v1.x*v2.x + v1.y*v2.y;
 }

SrVec2 lerp ( const SrVec2 &v1, const SrVec2 &v2, float t )
 {
   return v1*(1.0f-t) + v2*t;
 }

int compare ( const SrVec2 &v1, const SrVec2 &v2 )
 {
   if ( v1.x > v2.x ) return  1;
   if ( v1.x < v2.x ) return -1;
   if ( v1.y > v2.y ) return  1;
   if ( v1.y < v2.y ) return -1;
   return 0;
 }

int compare ( const SrVec2* v1, const SrVec2* v2 )
 {
   return compare ( *v1, *v2 );
 }

int compare_polar ( const SrVec2 &v1, const SrVec2 &v2 )
 {
   float a1 = angle_max(v1,SrVec2::i);
   float a2 = angle_max(v2,SrVec2::i);
   if ( a1 > a2 ) return  1;
   if ( a1 < a2 ) return -1;
   return 0;
 }

int compare_polar ( const SrVec2* v1, const SrVec2* v2 )
 {
   return compare_polar ( *v1, *v2 );
 }

void barycentric ( const SrPnt2& p1, const SrPnt2& p2, const SrPnt2& p3, const SrPnt2& p,
                   float& u, float& v, float& w )
 {
   # define DET3(a,b,c,d,e,f,g,h,i) a*e*i +b*f*g +d*h*c -c*e*g -b*d*i -a*f*h
   float A  = DET3 ( p1.x, p2.x, p3.x, p1.y, p2.y, p3.y, 1, 1, 1 );
   float A1 = DET3 (  p.x, p2.x, p3.x,  p.y, p2.y, p3.y, 1, 1, 1 );
   float A2 = DET3 ( p1.x,  p.x, p3.x, p1.y,  p.y, p3.y, 1, 1, 1 );
   //float A3 = DET3 ( p1.x, p2.x,  p.x, p1.y, p2.y,  p.y, 1, 1, 1 );
   # undef DET3
   u = A1/A;
   v = A2/A;
   w = 1.0f-u-v; // == A3/A;
 }

float ccw ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3 )
 {
   return (float) sr_ccw(p1.x,p1.y,p2.x,p2.y,p3.x,p3.y);
 }

bool segments_intersect ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p4 )
 {
   return sr_segments_intersect ( p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y );
 }

bool segments_intersect ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p4, SrVec2& p )
 {
   double x, y;
   bool b = sr_segments_intersect ( p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, x, y );
   p.set ( float(x), float(y) );
   return b;   
 }

bool lines_intersect ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p4 )
 {
   return sr_lines_intersect ( p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y );
 }

bool lines_intersect ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p4, SrVec2& p )
 {
   double x, y;
   bool b = sr_lines_intersect ( p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, x, y );
   p.set ( float(x), float(y) );
   return b;   
 }

void line_projection ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p, SrVec2& q )
 {
   double x, y;
   sr_line_projection ( p1.x, p1.y, p2.x, p2.y, p.x, p.y, x, y );
   q.set ( float(x), float(y) );
 }

bool segment_projection ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p, SrVec2& q, float epsilon )
 {
   double x, y;
   bool b = sr_segment_projection ( p1.x, p1.y, p2.x, p2.y, p.x, p.y, x, y, epsilon );
   q.set ( float(x), float(y) );
   return b;   
 }

bool in_segment ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p, float epsilon )
 {
   return sr_in_segment ( p1.x, p1.y, p2.x, p2.y, p.x, p.y, epsilon );
 }

bool in_segment ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p, float epsilon, float& dist2 )
 {
   double d;
   bool b = sr_in_segment ( p1.x, p1.y, p2.x, p2.y, p.x, p.y, epsilon, d );
   dist2 = (float) d;
   return b;
 }

bool in_triangle ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p )
 {
   return sr_in_triangle ( p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p.x, p.y );
 }

bool in_circle ( const SrVec2& p1, const SrVec2& p2, const SrVec2& p3, const SrVec2& p )
 {
   return sr_in_circle ( p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p.x, p.y );
 }

SrOutput& operator<< ( SrOutput& o, const SrVec2& v )
 {
   return o << v.x <<' '<< v.y;
 }

SrInput& operator>> ( SrInput& in, SrVec2& v )
 {
   return in >> v.x >> v.y;
 }

//================================== End of File ===========================================

/*
double Area ( const OVec2d &p1, const OVec2d &p2, const OVec2d &p3 )
 {
   return 0.5 * ( (p1.x*p2.y) + (p2.x*p3.y) + (p3.x*p1.y) -
                  (p1.x*p3.y) - (p3.x*p2.y) - (p2.x*p1.y) );
 }

double Ccw ( const OVec2d &p1, const OVec2d &p2, const OVec2d &p3 )
 {
   return ( (p1.x*p2.y) + (p2.x*p3.y) + (p3.x*p1.y) -
            (p1.x*p3.y) - (p3.x*p2.y) - (p2.x*p1.y) );
 }

void LineCoefs ( const OVec2d &p1, const OVec2d &p2,
                 double &a, double &b, double &c )
 {
 // need to test if this is correct :
   a = p2.y-p1.y;
   b = p1.x-p2.x;
   c = p2.x*p1.y - p1.x*p2.y;
 }

double Dist ( const OVec2d &p1, const OVec2d &p2, const OVec2d &p )
 {
   double a, b, c, abs;
   LineCoefs ( p1, p2, a, b, c );
   abs = a*p.x + b*p.y + c;
   return ABS(abs) / sqrt ( a*a + b*b );
 }

OVec2d CircleCenter ( const OVec2d &p1, const OVec2d &p2, const OVec2d &p3 )
 {
   OVec2d v1 = ((p2-p1)*0.5);
   OVec2d v2 = ((p3-p1)*0.5);
   OVec2d m1 = ( p1 + v1 );
   OVec2d m2 = ( p1 + v2 );
   OVec2d inter;
   InterLines ( m1, m1+v1.Orthogonal(), m2, m2+v2.Orthogonal(), inter );
   return inter;
 }

static float dist_pt_line2 ( float x, float y, float x1, float y1, float x2, float y2 )
 {
   float vx, vy, u;

   vx=x2-x1; vy=y2-y1;
   float n2 = vx*vx + vy*vy;

   if ( n2==0 ) return 0;

   u = ( (x-x1)*(x2-x1) + (y-y1)*(y2-y1) ) / n2;

   vx = x1 + u * (x2 - x1);
   vy = y1 + u * (y2 - y1);

   x = vx-x;
   y = vy-y;
   return x*x + y*y;
 }

*/
