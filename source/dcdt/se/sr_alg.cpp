#include "precompiled.h"
# include <math.h>

# include "sr.h"
# include "sr_alg.h"

/*-------------------------------------------------------------------*/
/* Functions to solve polynomials of 2nd, 3rt and 4th degree.        */
/* Source: graphics gems                                             */
/*-------------------------------------------------------------------*/

//change kai:
//# define aMAXFLOAT    3.40282347E+38F
# define aMAXFLOAT    3.40282347E+28F
# define aEPSILON     1e-9
# define aEPSILON2    0.00001
# define aISZERO(x)   ((x) > -aEPSILON && (x) < aEPSILON)
# define aCBRT(x)     ((x) > 0.0 ? pow((double)(x), 1.0/3.0) : \
                      ((x) < 0.0 ? -pow((double)-(x), 1.0/3.0) : 0.0))

int sr_solve_quadric_polynomial ( double c[3], double s[2] )
 {
   double p, q, D;

   // normal form: x^2 + px + q = 0
   p = c[1] / (2*c[2]);
   q = c[0] / c[2];

   D = p*p - q;

   if ( aISZERO(D) )
    { s[0] = -p;
      return 1;
    }
   else if ( D<0 )
    { return 0;
    }
   else // if (D > 0)
    { double sqrt_D = sqrt(D);
      s[0] =   sqrt_D - p;
      s[1] = - sqrt_D - p;
      return 2;
    }
}

int sr_solve_cubic_polynomial ( double c[4], double s[3] )
 {
   int     i, num;
   double  sub;
   double  A, B, C;
   double  sq_A, p, q;
   double  cb_p, D;

   // normal form: x^3 + Ax^2 + Bx + C = 0
   A = c[2] / c[3];
   B = c[1] / c[3];
   C = c[0] / c[3];

   // substitute x = y - A/3 to eliminate quadric term:
   //   x^3 +px + q = 0 
   sq_A = A * A;
   p = 1.0/3 * (- 1.0/3 * sq_A + B);
   q = 1.0/2 * (A * (2.0/27 * sq_A - 1.0/3 * B) + C);

   // use Cardano's formula
   cb_p = p * p * p;
   D = q * q + cb_p;

   if ( aISZERO(D) )
    { if ( aISZERO(q) ) // one triple solution
       { s[0] = 0;
         num = 1;
       }
      else // one single and one double solution
       { double u = aCBRT(-q);
         s[0] = 2 * u;
         s[1] = - u;
         num = 2;
       }
    }
   else if ( D<0 ) // Casus irreducibilis: three real solutions
    { double phi = 1.0/3 * acos ( -q/sqrt(-cb_p) );
      double t = 2 * sqrt(-p);
      s[0] =   t * cos(phi);
      s[1] = - t * cos(phi + SR_PI / 3);
      s[2] = - t * cos(phi - SR_PI / 3);
      num = 3;
    }
   else /* one real solution */
    { double sqrt_D = sqrt(D);
      double u = aCBRT(sqrt_D - q);
      double v = - aCBRT(sqrt_D + q);
      s[0] = u + v;
      num = 1;
    }

   // resubstitute
   sub = 1.0/3 * A;
   for ( i=0; i<num; ++i ) s[i] -= sub;
   return num;
 }

int sr_solve_quartic_polynomial ( double c[5], double s[4] )
 {
   double  coeffs[4];
   double  z, u, v, sub;
   double  A, B, C, D;
   double  sq_A, p, q, r;
   int     i, num;

   // normal form: x^4 + Ax^3 + Bx^2 + Cx + D = 0
   A = c[3] / c[4];
   B = c[2] / c[4];
   C = c[1] / c[4];
   D = c[0] / c[4];

   // substitute x = y - A/4 to eliminate cubic term:
   // x^4 + px^2 + qx + r = 0
   sq_A = A * A;
   p = - 3.0/8 * sq_A + B;
   q = A * (1.0/8 * sq_A - 1.0/2 * B) + C;
   r = sq_A * (- 3.0/256*sq_A + 1.0/16*B) - 1.0/4*A*C + D;

   if ( aISZERO(r) ) // no absolute term: y(y^3 + py + q) = 0
    { coeffs[0] = q;
      coeffs[1] = p;
      coeffs[2] = 0;
      coeffs[3] = 1;
      num = sr_solve_cubic_polynomial ( coeffs, s );
      s[num++] = 0;
    }
   else // solve the resolvent cubic ...
    { coeffs[0] = 1.0/2 * r * p - 1.0/8 * q * q;
      coeffs[1] = - r;
      coeffs[2] = - 1.0/2 * p;
      coeffs[3] = 1;
      sr_solve_cubic_polynomial ( coeffs, s );
      // ... and take the one real solution ...
      z = s[ 0 ];
      // ... to build two quadric equations
      u = z * z - r;
      v = 2 * z - p;

      if (aISZERO(u)) u = 0;
      else if (u > 0) u = sqrt(u);
      else return 0;

      if (aISZERO(v)) v = 0;
      else if (v > 0) v = sqrt(v);
      else return 0;

      coeffs[0] = z - u;
      coeffs[1] = q < 0 ? -v : v;
      coeffs[2] = 1;
      num = sr_solve_quadric_polynomial ( coeffs, s );

      coeffs[0]= z + u;
      coeffs[1] = q < 0 ? v : -v;
      coeffs[2] = 1;
      num += sr_solve_quadric_polynomial ( coeffs, s+num );
    }

    // resubstitute 
    sub = 1.0/4 * A;
    for ( i=0; i<num; ++i ) s[i] -= sub;
    return num;
}

float sr_in_ellipse ( float x, float y, float a, float b )
 {
   float cx = x/a;
   float cy = y/b;
   return cx*cx + cy*cy - 1;
 }

// replaces (x,y) by its closest point on the ellipse (a,b) 
void sr_get_closest_on_ellipse ( float a, float b, float& x, float& y )
 {
   double c[5], s[4];
   double r = a / b,
          e = 2. * (b - a*r),
          f = 2. * (x * r);

   if ( fabs(y)<aEPSILON2 )
    { x = x>0 ? a : -a;
      y = 0;
      return;
    }

   c [4] = y;
   c [3] = f - e;
   c [2] = 0;
   c [1] = f + e;
   c [0] = -y;

   int nb = sr_solve_quartic_polynomial ( c, s );

   double denom, s_theta, c_theta, dist;
   double test_x [4], test_y [4];
   double min_dist = aMAXFLOAT;
   int i, winner=0;

   // Find the closest point
   for ( i=0; i<nb; i++ )
    { denom = 1. + s [i] * s [i];
	  s_theta = 2. * s [i] / denom;
	  c_theta = (1. - s [i] * s [i]) / denom;

      test_x[i] = a * c_theta;
      test_y[i] = b * s_theta;

      dist = (test_x[i]-x) * (test_x[i]-x) + (test_y[i]-y) * (test_y[i]-y);
      if ( dist<min_dist )
       { min_dist = dist;
         winner = i;
       }
    }

   x = (float)test_x[winner];
   y = (float)test_y[winner];
 }

//============================== end of file ===============================

