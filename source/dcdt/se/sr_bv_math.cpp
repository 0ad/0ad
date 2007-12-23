#include "precompiled.h"/* Note: this code was adapted from the PQP library; 
   their copyright notice can be found at the end of this file. */
#include "0ad_warning_disable.h"

# include <math.h>
# include <stdio.h>

# include "sr_bv_math.h"

//====================== namespace SrBvMath =======================

/*   void Mprint ( const srbvmat M );
void SrBvMath::Mprint ( const srbvmat M )
{
  printf("%g %g %g\n%g %g %g\n%g %g %g\n",
	 M[0][0], M[0][1], M[0][2],
	 M[1][0], M[1][1], M[1][2],
	 M[2][0], M[2][1], M[2][2]);
}*/

/*   void Vprint ( const srbvvec V );
void SrBvMath::Vprint ( const srbvvec V )
{
  printf("%g %g %g\n", V[0], V[1], V[2]);
}*/

/*   void Vset ( srbvvec V, srbvreal x, srbvreal y, srbvreal z );
void SrBvMath::Vset ( srbvvec V, srbvreal x, srbvreal y, srbvreal z )
{
  V[0]=x; V[1]=y; V[2]=z;
}*/

void SrBvMath::Vset ( srbvvec V, float* fp )
{
  V[0]=fp[0]; V[1]=fp[1]; V[2]=fp[2];
}

void SrBvMath::Midentity ( srbvmat M )
{
  M[0][0] = M[1][1] = M[2][2] = 1;
  M[0][1] = M[1][2] = M[2][0] = 0;
  M[0][2] = M[1][0] = M[2][1] = 0;
}

void SrBvMath::Videntity ( srbvvec T )
{
  T[0] = T[1] = T[2] = 0.0;
}

void SrBvMath::McM ( srbvmat Mr, const srbvmat M )
{
  Mr[0][0] = M[0][0];  Mr[0][1] = M[0][1];  Mr[0][2] = M[0][2];
  Mr[1][0] = M[1][0];  Mr[1][1] = M[1][1];  Mr[1][2] = M[1][2];
  Mr[2][0] = M[2][0];  Mr[2][1] = M[2][1];  Mr[2][2] = M[2][2];
}

/*void MTcM ( srbvmat Mr, const srbvmat M ); // Mr=M.transpose()
void SrBvMath::MTcM ( srbvmat Mr, const srbvmat M )
{
  Mr[0][0] = M[0][0];  Mr[1][0] = M[0][1];  Mr[2][0] = M[0][2];
  Mr[0][1] = M[1][0];  Mr[1][1] = M[1][1];  Mr[2][1] = M[1][2];
  Mr[0][2] = M[2][0];  Mr[1][2] = M[2][1];  Mr[2][2] = M[2][2];
}*/

void SrBvMath::VcV ( srbvvec Vr, const srbvvec V )
{
  Vr[0] = V[0];  Vr[1] = V[1];  Vr[2] = V[2];
}

void SrBvMath::McolcV ( srbvvec Vr, const srbvmat M, int c )
{
  Vr[0] = M[0][c];
  Vr[1] = M[1][c];
  Vr[2] = M[2][c];
}

void SrBvMath::McolcMcol ( srbvmat Mr, int cr, const srbvmat M, int c )
{
  Mr[0][cr] = M[0][c];
  Mr[1][cr] = M[1][c];
  Mr[2][cr] = M[2][c];
}
/*
void MxMpV ( srbvmat Mr, const srbvmat M1, const srbvmat M2, const srbvvec T ); // Mr=M1*M2+T
   
   void SrBvMath::MxMpV ( srbvmat Mr, const srbvmat M1, const srbvmat M2, const srbvvec T )
{
  Mr[0][0] = (M1[0][0] * M2[0][0] +
	      M1[0][1] * M2[1][0] +
	      M1[0][2] * M2[2][0] +
	      T[0]);
  Mr[1][0] = (M1[1][0] * M2[0][0] +
	      M1[1][1] * M2[1][0] +
	      M1[1][2] * M2[2][0] +
	      T[1]);
  Mr[2][0] = (M1[2][0] * M2[0][0] +
	      M1[2][1] * M2[1][0] +
	      M1[2][2] * M2[2][0] +
	      T[2]);
  Mr[0][1] = (M1[0][0] * M2[0][1] +
	      M1[0][1] * M2[1][1] +
	      M1[0][2] * M2[2][1] +
	      T[0]);
  Mr[1][1] = (M1[1][0] * M2[0][1] +
	      M1[1][1] * M2[1][1] +
 	      M1[1][2] * M2[2][1] +
	      T[1]);
  Mr[2][1] = (M1[2][0] * M2[0][1] +
	      M1[2][1] * M2[1][1] +
	      M1[2][2] * M2[2][1] +
	      T[2]);
  Mr[0][2] = (M1[0][0] * M2[0][2] +
	      M1[0][1] * M2[1][2] +
	      M1[0][2] * M2[2][2] +
	      T[0]);
  Mr[1][2] = (M1[1][0] * M2[0][2] +
	      M1[1][1] * M2[1][2] +
	      M1[1][2] * M2[2][2] +
	      T[1]);
  Mr[2][2] = (M1[2][0] * M2[0][2] +
	      M1[2][1] * M2[1][2] +
	      M1[2][2] * M2[2][2] +
	      T[2]);
}*/

void SrBvMath::MxM ( srbvmat Mr, const srbvmat M1, const srbvmat M2 )
{
  Mr[0][0] = (M1[0][0] * M2[0][0] +
	      M1[0][1] * M2[1][0] +
	      M1[0][2] * M2[2][0]);
  Mr[1][0] = (M1[1][0] * M2[0][0] +
	      M1[1][1] * M2[1][0] +
	      M1[1][2] * M2[2][0]);
  Mr[2][0] = (M1[2][0] * M2[0][0] +
	      M1[2][1] * M2[1][0] +
	      M1[2][2] * M2[2][0]);
  Mr[0][1] = (M1[0][0] * M2[0][1] +
	      M1[0][1] * M2[1][1] +
	      M1[0][2] * M2[2][1]);
  Mr[1][1] = (M1[1][0] * M2[0][1] +
	      M1[1][1] * M2[1][1] +
 	      M1[1][2] * M2[2][1]);
  Mr[2][1] = (M1[2][0] * M2[0][1] +
	      M1[2][1] * M2[1][1] +
	      M1[2][2] * M2[2][1]);
  Mr[0][2] = (M1[0][0] * M2[0][2] +
	      M1[0][1] * M2[1][2] +
	      M1[0][2] * M2[2][2]);
  Mr[1][2] = (M1[1][0] * M2[0][2] +
	      M1[1][1] * M2[1][2] +
	      M1[1][2] * M2[2][2]);
  Mr[2][2] = (M1[2][0] * M2[0][2] +
	      M1[2][1] * M2[1][2] +
	      M1[2][2] * M2[2][2]);
}

/*   void MxMT ( srbvmat Mr, const srbvmat M1, const srbvmat M2 ); // Mr=M1*M2.transpose()
void SrBvMath::MxMT ( srbvmat Mr, const srbvmat M1, const srbvmat M2 )
{
  Mr[0][0] = (M1[0][0] * M2[0][0] +
	      M1[0][1] * M2[0][1] +
	      M1[0][2] * M2[0][2]);
  Mr[1][0] = (M1[1][0] * M2[0][0] +
	      M1[1][1] * M2[0][1] +
	      M1[1][2] * M2[0][2]);
  Mr[2][0] = (M1[2][0] * M2[0][0] +
	      M1[2][1] * M2[0][1] +
	      M1[2][2] * M2[0][2]);
  Mr[0][1] = (M1[0][0] * M2[1][0] +
	      M1[0][1] * M2[1][1] +
	      M1[0][2] * M2[1][2]);
  Mr[1][1] = (M1[1][0] * M2[1][0] +
	      M1[1][1] * M2[1][1] +
 	      M1[1][2] * M2[1][2]);
  Mr[2][1] = (M1[2][0] * M2[1][0] +
	      M1[2][1] * M2[1][1] +
	      M1[2][2] * M2[1][2]);
  Mr[0][2] = (M1[0][0] * M2[2][0] +
	      M1[0][1] * M2[2][1] +
	      M1[0][2] * M2[2][2]);
  Mr[1][2] = (M1[1][0] * M2[2][0] +
	      M1[1][1] * M2[2][1] +
	      M1[1][2] * M2[2][2]);
  Mr[2][2] = (M1[2][0] * M2[2][0] +
	      M1[2][1] * M2[2][1] +
	      M1[2][2] * M2[2][2]);
}*/

void SrBvMath::MTxM ( srbvmat Mr, const srbvmat M1, const srbvmat M2 )
{
  Mr[0][0] = (M1[0][0] * M2[0][0] +
	      M1[1][0] * M2[1][0] +
	      M1[2][0] * M2[2][0]);
  Mr[1][0] = (M1[0][1] * M2[0][0] +
	      M1[1][1] * M2[1][0] +
	      M1[2][1] * M2[2][0]);
  Mr[2][0] = (M1[0][2] * M2[0][0] +
	      M1[1][2] * M2[1][0] +
	      M1[2][2] * M2[2][0]);
  Mr[0][1] = (M1[0][0] * M2[0][1] +
	      M1[1][0] * M2[1][1] +
	      M1[2][0] * M2[2][1]);
  Mr[1][1] = (M1[0][1] * M2[0][1] +
	      M1[1][1] * M2[1][1] +
 	      M1[2][1] * M2[2][1]);
  Mr[2][1] = (M1[0][2] * M2[0][1] +
	      M1[1][2] * M2[1][1] +
	      M1[2][2] * M2[2][1]);
  Mr[0][2] = (M1[0][0] * M2[0][2] +
	      M1[1][0] * M2[1][2] +
	      M1[2][0] * M2[2][2]);
  Mr[1][2] = (M1[0][1] * M2[0][2] +
	      M1[1][1] * M2[1][2] +
	      M1[2][1] * M2[2][2]);
  Mr[2][2] = (M1[0][2] * M2[0][2] +
	      M1[1][2] * M2[1][2] +
	      M1[2][2] * M2[2][2]);
}

void SrBvMath::MxV ( srbvvec Vr, const srbvmat M1, const srbvvec V1 )
{
  Vr[0] = (M1[0][0] * V1[0] +
	   M1[0][1] * V1[1] + 
	   M1[0][2] * V1[2]);
  Vr[1] = (M1[1][0] * V1[0] +
	   M1[1][1] * V1[1] + 
	   M1[1][2] * V1[2]);
  Vr[2] = (M1[2][0] * V1[0] +
	   M1[2][1] * V1[1] + 
	   M1[2][2] * V1[2]);
}

void SrBvMath::MxVpV ( srbvvec Vr, const srbvmat M1, const srbvvec V1, const srbvvec V2)
{
  Vr[0] = (M1[0][0] * V1[0] +
	   M1[0][1] * V1[1] + 
	   M1[0][2] * V1[2] + 
	   V2[0]);
  Vr[1] = (M1[1][0] * V1[0] +
	   M1[1][1] * V1[1] + 
	   M1[1][2] * V1[2] + 
	   V2[1]);
  Vr[2] = (M1[2][0] * V1[0] +
	   M1[2][1] * V1[1] + 
	   M1[2][2] * V1[2] + 
	   V2[2]);
}

/*   void sMxVpV ( srbvvec Vr, srbvreal s1, const srbvmat M1, const srbvvec V1, const srbvvec V2 );
void SrBvMath::sMxVpV ( srbvvec Vr, srbvreal s1, const srbvmat M1, const srbvvec V1, const srbvvec V2 )
{
  Vr[0] = s1 * (M1[0][0] * V1[0] +
		M1[0][1] * V1[1] + 
		M1[0][2] * V1[2]) +
		V2[0];
  Vr[1] = s1 * (M1[1][0] * V1[0] +
		M1[1][1] * V1[1] + 
		M1[1][2] * V1[2]) + 
		V2[1];
  Vr[2] = s1 * (M1[2][0] * V1[0] +
		M1[2][1] * V1[1] + 
		M1[2][2] * V1[2]) + 
		V2[2];
}*/

void SrBvMath::MTxV ( srbvvec Vr, const srbvmat M1, const srbvvec V1 )
{
  Vr[0] = (M1[0][0] * V1[0] +
	   M1[1][0] * V1[1] + 
	   M1[2][0] * V1[2]); 
  Vr[1] = (M1[0][1] * V1[0] +
	   M1[1][1] * V1[1] + 
	   M1[2][1] * V1[2]);
  Vr[2] = (M1[0][2] * V1[0] +
	   M1[1][2] * V1[1] + 
	   M1[2][2] * V1[2]); 
}

/*   void sMTxV ( srbvvec Vr, srbvreal s1, const srbvmat M1, const srbvvec V1 );
void SrBvMath::sMTxV ( srbvvec Vr, srbvreal s1, const srbvmat M1, const srbvvec V1 )
{
  Vr[0] = s1*(M1[0][0] * V1[0] +
	      M1[1][0] * V1[1] + 
	      M1[2][0] * V1[2]); 
  Vr[1] = s1*(M1[0][1] * V1[0] +
	      M1[1][1] * V1[1] + 
	      M1[2][1] * V1[2]);
  Vr[2] = s1*(M1[0][2] * V1[0] +
	      M1[1][2] * V1[1] + 
	      M1[2][2] * V1[2]); 
}*/

/*   void sMxV ( srbvvec Vr, srbvreal s1, const srbvmat M1, const srbvvec V1 );
void SrBvMath::sMxV ( srbvvec Vr, srbvreal s1, const srbvmat M1, const srbvvec V1 )
{
  Vr[0] = s1*(M1[0][0] * V1[0] +
	      M1[0][1] * V1[1] + 
	      M1[0][2] * V1[2]); 
  Vr[1] = s1*(M1[1][0] * V1[0] +
	      M1[1][1] * V1[1] + 
	      M1[1][2] * V1[2]);
  Vr[2] = s1*(M1[2][0] * V1[0] +
	      M1[2][1] * V1[1] + 
	      M1[2][2] * V1[2]); 
}*/

void SrBvMath::VmV ( srbvvec Vr, const srbvvec V1, const srbvvec V2 )
{
  Vr[0] = V1[0] - V2[0];
  Vr[1] = V1[1] - V2[1];
  Vr[2] = V1[2] - V2[2];
}

void SrBvMath::VpV ( srbvvec Vr, const srbvvec V1, const srbvvec V2 )
{
  Vr[0] = V1[0] + V2[0];
  Vr[1] = V1[1] + V2[1];
  Vr[2] = V1[2] + V2[2];
}

void SrBvMath::VpVxS ( srbvvec Vr, const srbvvec V1, const srbvvec V2, srbvreal s )
{
  Vr[0] = V1[0] + V2[0] * s;
  Vr[1] = V1[1] + V2[1] * s;
  Vr[2] = V1[2] + V2[2] * s;
}

/*   void MskewV ( srbvmat M, const srbvvec v );
void SrBvMath::MskewV ( srbvmat M, const srbvvec v )
{
  M[0][0] = M[1][1] = M[2][2] = 0.0;
  M[1][0] = v[2];
  M[0][1] = -v[2];
  M[0][2] = v[1];
  M[2][0] = -v[1];
  M[1][2] = -v[0];
  M[2][1] = v[0];
}*/

void SrBvMath::VcrossV ( srbvvec Vr, const srbvvec V1, const srbvvec V2 )
{
  Vr[0] = V1[1]*V2[2] - V1[2]*V2[1];
  Vr[1] = V1[2]*V2[0] - V1[0]*V2[2];
  Vr[2] = V1[0]*V2[1] - V1[1]*V2[0];
}

/*   srbvreal Vlength ( srbvvec V );
srbvreal SrBvMath::Vlength ( srbvvec V )
{
  return sqrt(V[0]*V[0] + V[1]*V[1] + V[2]*V[2]);
}*/

/*   void Vnormalize ( srbvvec V );
void SrBvMath::Vnormalize ( srbvvec V )
{
  srbvreal d = srbvreal(1.0) / sqrt(V[0]*V[0] + V[1]*V[1] + V[2]*V[2]);
  V[0] *= d;
  V[1] *= d;
  V[2] *= d;
}*/

srbvreal SrBvMath::VdotV ( const srbvvec V1, const srbvvec V2 )
{
  return (V1[0]*V2[0] + V1[1]*V2[1] + V1[2]*V2[2]);
}


srbvreal SrBvMath::VdistV2 ( const srbvvec V1, const srbvvec V2 )
{
  return ( (V1[0]-V2[0]) * (V1[0]-V2[0]) + 
	   (V1[1]-V2[1]) * (V1[1]-V2[1]) + 
	   (V1[2]-V2[2]) * (V1[2]-V2[2]));
}


void SrBvMath::VxS ( srbvvec Vr, const srbvvec V, srbvreal s )
{
  Vr[0] = V[0] * s;
  Vr[1] = V[1] * s;
  Vr[2] = V[2] * s;
}

/*   void MRotZ ( srbvmat Mr, srbvreal t );
   void MRotX ( srbvmat Mr, srbvreal t );
   void MRotY ( srbvmat Mr, srbvreal t );
   void Mqinverse ( srbvmat Mr, srbvmat M );*/
/*
void SrBvMath::MRotZ ( srbvmat Mr, srbvreal t)
{
  Mr[0][0] = cos(t);
  Mr[1][0] = sin(t);
  Mr[0][1] = -Mr[1][0];
  Mr[1][1] = Mr[0][0];
  Mr[2][0] = Mr[2][1] = 0.0;
  Mr[0][2] = Mr[1][2] = 0.0;
  Mr[2][2] = 1.0;
}

void SrBvMath::MRotX ( srbvmat Mr, srbvreal t )
{
  Mr[1][1] = cos(t);
  Mr[2][1] = sin(t);
  Mr[1][2] = -Mr[2][1];
  Mr[2][2] = Mr[1][1];
  Mr[0][1] = Mr[0][2] = 0.0;
  Mr[1][0] = Mr[2][0] = 0.0;
  Mr[0][0] = 1.0;
}

void SrBvMath::MRotY ( srbvmat Mr, srbvreal t )
{
  Mr[2][2] = cos(t);
  Mr[0][2] = sin(t);
  Mr[2][0] = -Mr[0][2];
  Mr[0][0] = Mr[2][2];
  Mr[1][2] = Mr[1][0] = 0.0;
  Mr[2][1] = Mr[0][1] = 0.0;
  Mr[1][1] = 1.0;
}

void SrBvMath::Mqinverse ( srbvmat Mr, srbvmat M )
{
  int i,j;

  for(i=0; i<3; i++)
    for(j=0; j<3; j++)
    {
      int i1 = (i+1)%3;
      int i2 = (i+2)%3;
      int j1 = (j+1)%3;
      int j2 = (j+2)%3;
      Mr[i][j] = (M[j1][i1]*M[j2][i2] - M[j1][i2]*M[j2][i1]);
    }
}*/

// Meigen from Numerical Recipes in C

#define ROTATE(a,i,j,k,l) g=a[i][j]; h=a[k][l]; a[i][j]=g-s*(h+g*tau); a[k][l]=h+s*(g-h*tau)

void SrBvMath::Meigen ( srbvmat vout, srbvvec dout, srbvmat a )
{
  int n = 3;
  int j,iq,ip,i;
  srbvreal tresh,theta,tau,t,sm,s,h,g,c;
  int nrot;
  srbvvec b, z, d;
  srbvmat v;
  
  Midentity(v);
  for(ip=0; ip<n; ip++) 
    {
      b[ip] = a[ip][ip];
      d[ip] = a[ip][ip];
      z[ip] = 0.0;
    }
  
  nrot = 0;
  
  for(i=0; i<50; i++)
   {  sm=0.0;
      for(ip=0;ip<n;ip++) for(iq=ip+1;iq<n;iq++) sm+=fabs(a[ip][iq]);
      if (sm == 0.0)
	   { McM(vout, v);
	     VcV(dout, d);
	     return;
	   }
      if (i < 3)
       tresh=srbvreal(0.2)*sm/(n*n);
      else
       tresh=0.0;
      
      for(ip=0; ip<n; ip++) for(iq=ip+1; iq<n; iq++)
	   { g = srbvreal(100.0)*fabs(a[ip][iq]);
	     if (i>3 && 
	         fabs(d[ip])+g==fabs(d[ip]) && 
	         fabs(d[iq])+g==fabs(d[iq]))
	      { a[ip][iq]=0.0; }
	     else if (fabs(a[ip][iq])>tresh)
	      { h = d[iq]-d[ip];
	        if (fabs(h)+g == fabs(h)) t=(a[ip][iq])/h;
	         else
		     { theta=(srbvreal)0.5*h/(a[ip][iq]);
		       t=(srbvreal)(1.0/(fabs(theta)+sqrt(1.0+theta*theta)));
		       if (theta < 0.0) t = -t;
		     }
	        c=(srbvreal)1.0/sqrt(1+t*t);
	        s=t*c;
	        tau=s/((srbvreal)1.0+c);
	        h=t*a[ip][iq];
	        z[ip] -= h;
	        z[iq] += h;
	        d[ip] -= h;
	        d[iq] += h;
	        a[ip][iq]=0.0;
	        for(j=0;j<ip;j++) { ROTATE(a,j,ip,j,iq); } 
	        for(j=ip+1;j<iq;j++) { ROTATE(a,ip,j,j,iq); } 
	        for(j=iq+1;j<n;j++) { ROTATE(a,ip,j,iq,j); } 
	        for(j=0;j<n;j++) { ROTATE(v,j,ip,j,iq); } 
	        nrot++;
	      }
	   }
      for(ip=0;ip<n;ip++)
	   { b[ip] += z[ip];
	     d[ip] = b[ip];
	     z[ip] = 0.0;
	   }
   }

  fprintf(stderr, "eigen: too many iterations in Jacobi transform.\n");

  return;
}

#ifdef _WIN32
#include <float.h>
#ifndef isnan
#define isnan _isnan
#endif
#endif

//--------------------------------------------------------------------------
// SegPoints() 
//
// Returns closest points between an segment pair.
// Implemented from an algorithm described in
//
// Vladimir J. Lumelsky,
// On fast computation of distance between line segments.
// In Information Processing Letters, no. 21, pages 55-61, 1985.   
//--------------------------------------------------------------------------
void SrBvMath::SegPoints ( srbvvec VEC, srbvvec X, srbvvec Y,
                           const srbvvec P, const srbvvec A,
                           const srbvvec Q, const srbvvec B )
{
  srbvreal T[3], A_dot_A, B_dot_B, A_dot_B, A_dot_T, B_dot_T;
  srbvreal TMP[3];

  VmV(T,Q,P);
  A_dot_A = VdotV(A,A);
  B_dot_B = VdotV(B,B);
  A_dot_B = VdotV(A,B);
  A_dot_T = VdotV(A,T);
  B_dot_T = VdotV(B,T);

  // t parameterizes ray P,A 
  // u parameterizes ray Q,B 

  srbvreal t,u;

  // compute t for the closest point on ray P,A to
  // ray Q,B

  srbvreal denom = A_dot_A*B_dot_B - A_dot_B*A_dot_B;

  t = (A_dot_T*B_dot_B - B_dot_T*A_dot_B) / denom;

  // clamp result so t is on the segment P,A

  if ((t < 0) || isnan(t)) t = 0; else if (t > 1) t = 1;

  // find u for point on ray Q,B closest to point at t

  u = (t*A_dot_B - B_dot_T) / B_dot_B;

  // if u is on segment Q,B, t and u correspond to 
  // closest points, otherwise, clamp u, recompute and
  // clamp t 

  if ((u <= 0) || isnan(u)) {

    VcV(Y, Q);

    t = A_dot_T / A_dot_A;

    if ((t <= 0) || isnan(t)) {
      VcV(X, P);
      VmV(VEC, Q, P);
    }
    else if (t >= 1) {
      VpV(X, P, A);
      VmV(VEC, Q, X);
    }
    else {
      VpVxS(X, P, A, t);
      VcrossV(TMP, T, A);
      VcrossV(VEC, A, TMP);
    }
  }
  else if (u >= 1) {

    VpV(Y, Q, B);

    t = (A_dot_B + A_dot_T) / A_dot_A;

    if ((t <= 0) || isnan(t)) {
      VcV(X, P);
      VmV(VEC, Y, P);
    }
    else if (t >= 1) {
      VpV(X, P, A);
      VmV(VEC, Y, X);
    }
    else {
      VpVxS(X, P, A, t);
      VmV(T, Y, P);
      VcrossV(TMP, T, A);
      VcrossV(VEC, A, TMP);
    }
  }
  else {

    VpVxS(Y, Q, B, u);

    if ((t <= 0) || isnan(t)) {
      VcV(X, P);
      VcrossV(TMP, T, B);
      VcrossV(VEC, B, TMP);
    }
    else if (t >= 1) {
      VpV(X, P, A);
      VmV(T, Q, X);
      VcrossV(TMP, T, B);
      VcrossV(VEC, B, TMP);
    }
    else {
      VpVxS(X, P, A, t);
      VcrossV(VEC, A, B);
      if (VdotV(VEC, T) < 0) {
        VxS(VEC, VEC, -1);
      }
    }
  }
}

//--------------------------------------------------------------------------
// TriDist() 
//
// Computes the closest points on two triangles, and returns the 
// distance between them.
// 
// S and T are the triangles, stored tri[point][dimension].
//
// If the triangles are disjoint, P and Q give the closest points of 
// S and T respectively. However, if the triangles overlap, P and Q 
// are basically a random pair of points from the triangles, not 
// coincident points on the intersection of the triangles, as might 
// be expected.
//--------------------------------------------------------------------------
srbvreal 
SrBvMath::TriDist( srbvvec P, srbvvec Q,
                   const srbvmat S, const srbvmat T )  
{
  // Compute vectors along the 6 sides

  srbvreal Sv[3][3], Tv[3][3];
  srbvreal VEC[3];

  VmV(Sv[0],S[1],S[0]);
  VmV(Sv[1],S[2],S[1]);
  VmV(Sv[2],S[0],S[2]);

  VmV(Tv[0],T[1],T[0]);
  VmV(Tv[1],T[2],T[1]);
  VmV(Tv[2],T[0],T[2]);

  // For each edge pair, the vector connecting the closest points 
  // of the edges defines a slab (parallel planes at head and tail
  // enclose the slab). If we can show that the off-edge vertex of 
  // each triangle is outside of the slab, then the closest points
  // of the edges are the closest points for the triangles.
  // Even if these tests fail, it may be helpful to know the closest
  // points found, and whether the triangles were shown disjoint

  srbvreal V[3];
  srbvreal Z[3];
  srbvreal minP[3], minQ[3], mindd;
  int shown_disjoint = 0;

  mindd = VdistV2(S[0],T[0]) + 1;  // Set first minimum safely high

  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      // Find closest points on edges i & j, plus the 
      // vector (and distance squared) between these points

      SegPoints(VEC,P,Q,S[i],Sv[i],T[j],Tv[j]);
      
      VmV(V,Q,P);
      srbvreal dd = VdotV(V,V);

      // Verify this closest point pair only if the distance 
      // squared is less than the minimum found thus far.

      if (dd <= mindd)
      {
        VcV(minP,P);
        VcV(minQ,Q);
        mindd = dd;

        VmV(Z,S[(i+2)%3],P);
        srbvreal a = VdotV(Z,VEC);
        VmV(Z,T[(j+2)%3],Q);
        srbvreal b = VdotV(Z,VEC);

        if ((a <= 0) && (b >= 0)) return sqrt(dd);

        srbvreal p = VdotV(V, VEC);

        if (a < 0) a = 0;
        if (b > 0) b = 0;
        if ((p - a + b) > 0) shown_disjoint = 1;	
      }
    }
  }

  // No edge pairs contained the closest points.  
  // either:
  // 1. one of the closest points is a vertex, and the
  //    other point is interior to a face.
  // 2. the triangles are overlapping.
  // 3. an edge of one triangle is parallel to the other's face. If
  //    cases 1 and 2 are not true, then the closest points from the 9
  //    edge pairs checks above can be taken as closest points for the
  //    triangles.
  // 4. possibly, the triangles were degenerate.  When the 
  //    triangle points are nearly colinear or coincident, one 
  //    of above tests might fail even though the edges tested
  //    contain the closest points.

  // First check for case 1

  srbvreal Sn[3], Snl;       
  VcrossV(Sn,Sv[0],Sv[1]); // Compute normal to S triangle
  Snl = VdotV(Sn,Sn);      // Compute square of length of normal
  
  // If cross product is long enough,

  if (Snl > 1e-15)  
  {
    // Get projection lengths of T points

    srbvreal Tp[3]; 

    VmV(V,S[0],T[0]);
    Tp[0] = VdotV(V,Sn);

    VmV(V,S[0],T[1]);
    Tp[1] = VdotV(V,Sn);

    VmV(V,S[0],T[2]);
    Tp[2] = VdotV(V,Sn);

    // If Sn is a separating direction,
    // find point with smallest projection

    int point = -1;
    if ((Tp[0] > 0) && (Tp[1] > 0) && (Tp[2] > 0))
    {
      if (Tp[0] < Tp[1]) point = 0; else point = 1;
      if (Tp[2] < Tp[point]) point = 2;
    }
    else if ((Tp[0] < 0) && (Tp[1] < 0) && (Tp[2] < 0))
    {
      if (Tp[0] > Tp[1]) point = 0; else point = 1;
      if (Tp[2] > Tp[point]) point = 2;
    }

    // If Sn is a separating direction, 

    if (point >= 0) 
    {
      shown_disjoint = 1;

      // Test whether the point found, when projected onto the 
      // other triangle, lies within the face.
    
      VmV(V,T[point],S[0]);
      VcrossV(Z,Sn,Sv[0]);
      if (VdotV(V,Z) > 0)
      {
        VmV(V,T[point],S[1]);
        VcrossV(Z,Sn,Sv[1]);
        if (VdotV(V,Z) > 0)
        {
          VmV(V,T[point],S[2]);
          VcrossV(Z,Sn,Sv[2]);
          if (VdotV(V,Z) > 0)
          {
            // T[point] passed the test - it's a closest point for 
            // the T triangle; the other point is on the face of S

            VpVxS(P,T[point],Sn,Tp[point]/Snl);
            VcV(Q,T[point]);
            return sqrt(VdistV2(P,Q));
          }
        }
      }
    }
  }

  srbvreal Tn[3], Tnl;       
  VcrossV(Tn,Tv[0],Tv[1]); 
  Tnl = VdotV(Tn,Tn);      
  
  if (Tnl > 1e-15)  
  {
    srbvreal Sp[3]; 

    VmV(V,T[0],S[0]);
    Sp[0] = VdotV(V,Tn);

    VmV(V,T[0],S[1]);
    Sp[1] = VdotV(V,Tn);

    VmV(V,T[0],S[2]);
    Sp[2] = VdotV(V,Tn);

    int point = -1;
    if ((Sp[0] > 0) && (Sp[1] > 0) && (Sp[2] > 0))
    {
      if (Sp[0] < Sp[1]) point = 0; else point = 1;
      if (Sp[2] < Sp[point]) point = 2;
    }
    else if ((Sp[0] < 0) && (Sp[1] < 0) && (Sp[2] < 0))
    {
      if (Sp[0] > Sp[1]) point = 0; else point = 1;
      if (Sp[2] > Sp[point]) point = 2;
    }

    if (point >= 0) 
    { 
      shown_disjoint = 1;

      VmV(V,S[point],T[0]);
      VcrossV(Z,Tn,Tv[0]);
      if (VdotV(V,Z) > 0)
      {
        VmV(V,S[point],T[1]);
        VcrossV(Z,Tn,Tv[1]);
        if (VdotV(V,Z) > 0)
        {
          VmV(V,S[point],T[2]);
          VcrossV(Z,Tn,Tv[2]);
          if (VdotV(V,Z) > 0)
          {
            VcV(P,S[point]);
            VpVxS(Q,S[point],Tn,Sp[point]/Tnl);
            return sqrt(VdistV2(P,Q));
          }
        }
      }
    }
  }

  // Case 1 can't be shown.
  // If one of these tests showed the triangles disjoint,
  // we assume case 3 or 4, otherwise we conclude case 2, 
  // that the triangles overlap.
  
  if (shown_disjoint)
  {
    VcV(P,minP);
    VcV(Q,minQ);
    return sqrt(mindd);
  }
  else return 0;
}

/*************************************************************************\
  Copyright 1999 The University of North Carolina at Chapel Hill.
  All Rights Reserved.

  Permission to use, copy, modify and distribute this software and its
  documentation for educational, research and non-profit purposes, without
  fee, and without a written agreement is hereby granted, provided that the
  above copyright notice and the following three paragraphs appear in all
  copies.

  IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL HILL BE
  LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
  CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE
  USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY
  OF NORTH CAROLINA HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
  DAMAGES.

  THE UNIVERSITY OF NORTH CAROLINA SPECIFICALLY DISCLAIM ANY
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE
  PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY OF
  NORTH CAROLINA HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT,
  UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

  The authors may be contacted via:

  US Mail:             S. Gottschalk, E. Larsen
                       Department of Computer Science
                       Sitterson Hall, CB #3175
                       University of N. Carolina
                       Chapel Hill, NC 27599-3175
  Phone:               (919)962-1749
  EMail:               geom@cs.unc.edu
\**************************************************************************/

