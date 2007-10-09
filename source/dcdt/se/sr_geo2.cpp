#include "precompiled.h"
# include <stdio.h>
# include <math.h>

# include "sr_geo2.h"

// This file is designed to be as most as possible independent of other sr types

//================================= Macros =========================================

# define gABS(x)                  (x>0? (x):-(x))
# define gMAX(a,b)                (a>b? (a):(b))
# define gSWAP(a,b)               { tmp=a; a=b; b=tmp; }
# define gOUTXY(a,b)              printf("(%+6.4f,%+6.4f) ", a, b )
# define gOUTL                    printf("\n");
# define gEPS                     1.0E-14 // doubles have 15 decimals
# define gNEXTZERO(a)             ( (a)>-(gEPS) && (a)<(gEPS) )

// The following macro can be defined to activate some extra operations that
// try to get more precise results in the functions of this file. Normally
// these extra operations are not worth to activate
// # define gMAXPREC 

//================================= funcs =====================================

bool sr_segments_intersect ( double p1x, double p1y, double p2x, double p2y,
                             double p3x, double p3y, double p4x, double p4y )
 {
   double d = (p4y-p3y)*(p1x-p2x)-(p1y-p2y)*(p4x-p3x);
   if ( gNEXTZERO(d) ) return false; // they are parallel
   double t = ((p4y-p3y)*(p4x-p2x)-(p4x-p3x)*(p4y-p2y)) / d;
   if ( t<0.0 || t>1.0 ) return false; // outside [p1,p2]
   double s = ((p4y-p2y)*(p1x-p2x)-(p1y-p2y)*(p4x-p2x)) / d;
   if ( s<0.0 || s>1.0 ) return false; // outside [p3,p4]
   return true;
}

bool sr_segments_intersect ( double p1x, double p1y, double p2x, double p2y,
                             double p3x, double p3y, double p4x, double p4y,
                             double& x, double &y )
 {
   double d = (p4y-p3y)*(p1x-p2x)-(p1y-p2y)*(p4x-p3x);
   if ( gNEXTZERO(d) ) return false; // they are parallel
   double t = ((p4y-p3y)*(p4x-p2x)-(p4x-p3x)*(p4y-p2y)) / d;
   if ( t<0.0 || t>1.0 ) return false; // outside [p1,p2]
   double s = ((p4y-p2y)*(p1x-p2x)-(p1y-p2y)*(p4x-p2x)) / d;
   if ( s<0.0 || s>1.0 ) return false; // outside [p3,p4]
   x = t*p1x+(1-t)*p2x;
   y = t*p1y+(1-t)*p2y;
   # ifdef gMAXPREC 
   x += s*p3x+(1-s)*p4x;
   y += s*p3y+(1-s)*p4y;
   x /= 2;
   y /= 2;
   # endif
   return true;
}

bool sr_lines_intersect ( double p1x, double p1y, double p2x, double p2y,
                          double p3x, double p3y, double p4x, double p4y )
 {
   double d = (p4y-p3y)*(p1x-p2x)-(p1y-p2y)*(p4x-p3x);
   if ( gNEXTZERO(d) ) return false; // they are parallel
   return true;
}

bool sr_lines_intersect ( double p1x, double p1y, double p2x, double p2y,
                          double p3x, double p3y, double p4x, double p4y,
                          double& x, double &y )
 {
   double d = (p4y-p3y)*(p1x-p2x)-(p1y-p2y)*(p4x-p3x);
   if ( gNEXTZERO(d) ) return false; // they are parallel
   double t = ((p4y-p3y)*(p4x-p2x)-(p4x-p3x)*(p4y-p2y)) / d;
   x = t*p1x+(1-t)*p2x;
   y = t*p1y+(1-t)*p2y;
   # ifdef gMAXPREC // 1000 random tests reveal E-10 order error between t and s results
   double s = ((p4y-p2y)*(p1x-p2x)-(p1y-p2y)*(p4x-p2x)) / d;
   x += s*p3x+(1-s)*p4x;
   y += s*p3y+(1-s)*p4y;
   x /= 2;
   y /= 2;
   # endif
   return true;
}

void sr_line_projection ( double p1x, double p1y, double p2x, double p2y, double px, double py, double& qx, double& qy )
 {
   qx = px;
   qy = py;
   double vx = -(p2y-p1y);
   double vy = p2x-p1x; // v = (p2-p1).ortho(), ortho==(-y,x)
   sr_lines_intersect ( p1x, p1y, p2x, p2y, px, py, px+vx, py+vy, qx, qy );
 }

bool sr_segment_projection ( double p1x, double p1y, double p2x, double p2y, double px, double py, double& qx, double& qy, double epsilon )
 {
   double tmp;
   sr_line_projection ( p1x, p1y, p2x, p2y, px, py, qx, qy );
   if ( epsilon==0 ) epsilon=gEPS; // if 0 the inequalities dont work with collinear inputs

/*   if ( p1x>p2x ) gSWAP(p1x,p2x);
   if ( qx+epsilon<=p1x || qx-epsilon>=p2x ) return false;
   if ( p1y>p2y ) gSWAP(p1y,p2y);
   if ( qy+epsilon<=p1y || qy-epsilon>=p2y ) return false;*/

   if ( p1x>p2x ) gSWAP(p1x,p2x);
   if ( qx+epsilon<p1x || qx-epsilon>p2x ) return false;
   if ( p1y>p2y ) gSWAP(p1y,p2y);
   if ( qy+epsilon<p1y || qy-epsilon>p2y ) return false;
   return true;
 }

double sr_dist2 ( double p1x, double p1y, double p2x, double p2y )
 {
   p1x = p2x-p1x;
   p1y = p2y-p1y;
   return p1x*p1x + p1y*p1y;
 }

double sr_point_segment_dist ( double px, double py, double p1x, double p1y, double p2x, double p2y )
 {
   double dist, qx, qy;

   if ( sr_segment_projection(p1x,p1y,p2x,p2y,px,py,qx,qy,0) )
    { dist = sr_dist2(px,py,qx,qy);
    }
   else
    { dist = sr_dist2(px,py,p1x,p1y);
      double d = sr_dist2(px,py,p2x,p2y);
      if (d<dist) dist=d;
    }

   return sqrt(dist);
 }

double sr_point_line_dist ( double px, double py, double p1x, double p1y, double p2x, double p2y )
 {
   double qx, qy;
   sr_line_projection (p1x,p1y,p2x,p2y,px,py,qx,qy);
   return sqrt( sr_dist2(px,py,qx,qy) );
 }

bool sr_next ( double p1x, double p1y, double p2x, double p2y, double epsilon )
 { 
   return sr_dist2(p1x,p1y,p2x,p2y)<=epsilon*epsilon? true:false; 
 }

double sr_ccw ( double p1x, double p1y, double p2x, double p2y, double p3x, double p3y )
 {
   return SR_CCW(p1x,p1y,p2x,p2y,p3x,p3y);
 }

bool sr_in_segment ( double p1x, double p1y, double p2x, double p2y, double px, double py, double epsilon )
 {
   double qx, qy;
   if ( epsilon==0 ) epsilon=gEPS;
   if ( !sr_segment_projection ( p1x, p1y, p2x, p2y, px, py, qx, qy, epsilon ) ) return false;
   return sr_dist2(px,py,qx,qy)<=epsilon*epsilon? true:false; 
 }

bool sr_in_segment ( double p1x, double p1y, double p2x, double p2y, double px, double py,
                     double epsilon, double& dist2 )
 {
   double qx, qy;
   if ( epsilon==0 ) epsilon=gEPS;
   if ( !sr_segment_projection ( p1x, p1y, p2x, p2y, px, py, qx, qy, epsilon ) ) return false;
   dist2 = sr_dist2(px,py,qx,qy);
//sr_out<<dist2<<srspc<<(epsilon*epsilon)<<srnl;
   return dist2<=epsilon*epsilon? true:false; 
 }

bool sr_in_triangle ( double p1x, double p1y, double p2x, double p2y, double p3x, double p3y, double px, double py )
 {
   return SR_CCW(px,py,p1x,p1y,p2x,p2y)>=0 && 
          SR_CCW(px,py,p2x,p2y,p3x,p3y)>=0 && 
          SR_CCW(px,py,p3x,p3y,p1x,p1y)>=0 ? true:false;
 }

// circle_test :
// p1, p2, p3 must be in ccw order
// Calculates the following determinant :
//   | p1x   p1y   p1x*p1x + p1y*p1y   1.0 | 
//   | p2x   p2y   p2x*p2x + p2y*p2y   1.0 | 
//   | p3x   p3y   p3x*p3x + p3y*p3y   1.0 | 
//   | px    py    px*px   + py*py     1.0 | 
// This is not the most accurate calculation, but is the fastest.
bool sr_in_circle ( double p1x, double p1y, double p2x, double p2y, double p3x, double p3y, double px, double py )
 {
   double p1z = p1x*p1x + p1y*p1y;
   double p2z = p2x*p2x + p2y*p2y;
   double p3z = p3x*p3x + p3y*p3y;
   double pz  = px*px   + py*py;

   double m12 = p2x*p3y - p2y*p3x;
   double m13 = p2x*p3z - p2z*p3x;
   double m14 = p2x     - p3x;
   double m23 = p2y*p3z - p2z*p3y;
   double m24 = p2y     - p3y;
   double m34 = p2z     - p3z;
   double d1  = p1y*m34 - p1z*m24 + m23;
   double d2  = p1x*m34 - p1z*m14 + m13;
   double d3  = p1x*m24 - p1y*m14 + m12;
   double d4  = p1x*m23 - p1y*m13 + p1z*m12;

   double det = d4 - px*d1 + py*d2 - pz*d3;

   const double prec = gEPS; // I have encountered cases working only with this tiny epsilon
   return det>prec? true:false;
 }

void sr_barycentric ( double p1x, double p1y, double p2x, double p2y, double p3x, double p3y,
                      double px, double py, double& u, double& v, double& w )
 {
   # define DET3(a,b,c,d,e,f,g,h,i) a*e*i +b*f*g +d*h*c -c*e*g -b*d*i -a*f*h
   double A  = DET3 ( p1x, p2x, p3x, p1y, p2y, p3y, 1, 1, 1 );
   double A1 = DET3 (  px, p2x, p3x,  py, p2y, p3y, 1, 1, 1 );
   double A2 = DET3 ( p1x,  px, p3x, p1y,  py, p3y, 1, 1, 1 );
   //double A3 = DET3 ( p1x, p2x,  px, p1y, p2y,  py, 1, 1, 1 );
   # undef DET3
   u = A1/A;
   v = A2/A;
   w = 1.0-u-v; // == A3/A;
 }

//=============================== Documentation ======================================

/*
Intersection of segments math:

t p1 + (1-t)p2 = p      (1)
s p3 + (1-s)p4 = p      (2)

=> Making (1)=(2) :
   t p1 + (1-t)p2 = s p3 + (1-s)p4 
   t(p1-p2) + s(p4-p3) = p4-p2
   t(p1x-p2x) + s(p4x-p3x) = p4x-p2x    (3)
   t(p1y-p2y) + s(p4y-p3y) = p4y-p2y    (4)

=> Putting t from (3) to (4) :
  t = [(p4x-p2x) - s(p4x-p3x)] / (p1x-p2x)
  [(p4x-p2x) - s(p4x-p3x)] / (p1x-p2x) = [(p4y-p2y) - s(p4y-p3y)] / (p1y-p2y)
  (p1y-p2y)(p4x-p2x) - s(p1y-p2y)(p4x-p3x) = (p4y-p2y)(p1x-p2x) - s(p4y-p3y)(p1x-p2x)
  s(p4y-p3y)(p1x-p2x) - s(p1y-p2y)(p4x-p3x) = (p4y-p2y)(p1x-p2x) - (p1y-p2y)(p4x-p2x)
  s = [(p4y-p2y)(p1x-p2x)-(p1y-p2y)(p4x-p2x)] / [(p4y-p3y)(p1x-p2x)-(p1y-p2y)(p4x-p3x)]
  Let d = (p4y-p3y)(p1x-p2x)-(p1y-p2y)(p4x-p3x)       (5)
  s = [(p4y-p2y)(p1x-p2x)-(p1y-p2y)(p4x-p2x)] / d     (6)

=> Putting s from (3) to (4) :
  s = [(p4x-p2x) - t(p1x-p2x)] / (p4x-p3x)
  [(p4x-p2x) - t(p1x-p2x)] / (p4x-p3x) =  [(p4y-p2y) - t(p1y-p2y)] / (p4y-p3y)
  (p4y-p3y)(p4x-p2x) - t(p4y-p3y)(p1x-p2x) = (p4x-p3x)(p4y-p2y) - t(p4x-p3x)(p1y-p2y)
  t(p4x-p3x)(p1y-p2y) - t(p4y-p3y)(p1x-p2x) = (p4x-p3x)(p4y-p2y) - (p4y-p3y)(p4x-p2x)
  t = [(p4x-p3x)(p4y-p2y)-(p4y-p3y)(p4x-p2x)] / [(p4x-p3x)(p1y-p2y)-(p4y-p3y)(p1x-p2x)]
  t = -1*[(p4x-p3x)(p4y-p2y)-(p4y-p3y)(p4x-p2x)] / -1*[(p4x-p3x)(p1y-p2y)-(p4y-p3y)(p1x-p2x)]
  t = [(p4y-p3y)(p4x-p2x)-(p4x-p3x)(p4y-p2y)] / [(p4y-p3y)(p1x-p2x)-(p4x-p3x)(p1y-p2y)]
  Using (5) :
  t = [(p4y-p3y)(p4x-p2x)-(p4x-p3x)(p4y-p2y)] / d     (7)

=> From (6) and (7), t and s determines p: 
  p = t p1 + (1-t)p2 = s p3 + (1-s)p4  */

//=============================== End of File ======================================
