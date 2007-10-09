/** \file sr_bv_math.h 
 * BV tree math routines */

# ifndef SR_BV_MATH_H
# define SR_BV_MATH_H

# include "sr.h"

/*! if the following macro is defined, will use floats instead of doubles
    Default is doubles (macro not defined) */
# ifdef SR_BV_MATH_FLOAT
  typedef float srbvreal;
# else
  typedef double srbvreal;
# endif

typedef srbvreal srbvmat[3][3];
typedef srbvreal srbvvec[3];

const srbvreal srbvpi = (srbvreal)SR_PI;

//============================== SrBvMath ===============================

/*! This namespace encapsulates a (row-major) matrix/vector code
    adapted from the PQP package. Their copyright notice
    is displayed in the source file */
namespace SrBvMath
 {
   void Vset ( srbvvec V, float* fp );
   void Midentity ( srbvmat M ); // set to id matrix
   void Videntity ( srbvvec T ); // set to a null vector
   void McM ( srbvmat Mr, const srbvmat M ); // Mr=M
   void VcV ( srbvvec Vr, const srbvvec V ); // Vr=V
   void McolcV ( srbvvec Vr, const srbvmat M, int c ); // Vr=M.column(c)
   void McolcMcol ( srbvmat Mr, int cr, const srbvmat M, int c ); // Vr.column(cr)=M.column(c)
   void MxM ( srbvmat Mr, const srbvmat M1, const srbvmat M2 ); // Mr=M1*M2
   void MTxM ( srbvmat Mr, const srbvmat M1, const srbvmat M2 ); // Mr=M1.transpose()*M2
   void MxV ( srbvvec Vr, const srbvmat M1, const srbvvec V1 ); // Vr=M1*V1
   void MxVpV ( srbvvec Vr, const srbvmat M1, const srbvvec V1, const srbvvec V2 );
   void MTxV ( srbvvec Vr, const srbvmat M1, const srbvvec V1 );
   void VmV ( srbvvec Vr, const srbvvec V1, const srbvvec V2 );
   void VpV ( srbvvec Vr, const srbvvec V1, const srbvvec V2 );
   void VpVxS ( srbvvec Vr, const srbvvec V1, const srbvvec V2, srbvreal s );
   void VcrossV ( srbvvec Vr, const srbvvec V1, const srbvvec V2 );
   srbvreal VdotV ( const srbvvec V1, const srbvvec V2 );
   srbvreal VdistV2 ( const srbvvec V1, const srbvvec V2 );
   void VxS ( srbvvec Vr, const srbvvec V, srbvreal s );
   void Meigen ( srbvmat vout, srbvvec dout, srbvmat a );

   /*! SegPoints returns closest points between a segment pair.
	   Points x and y are the found closest points.
       Parameters p and a are segment 1 origin and vector.
       Parameters q and b are segment 2 origin and vector. */
   void SegPoints( srbvvec vec, srbvvec x, srbvvec y,
                   const srbvvec p, const srbvvec a,
                   const srbvvec q, const srbvvec b );
                   
   /*! TriDist computes the closest points on two triangles, and
       returns the distance between them.
       s and t are the triangles, stored tri[point][dimension].
       If the triangles are disjoint, p and q give the closest
       points of s and t respectively. However, if the triangles
       overlap, p and q are basically a random pair of points from
       the triangles, not coincident points on the intersection of
       the triangles, as might be expected. */
   srbvreal TriDist ( srbvvec p, srbvvec q, 
                      const srbvmat s, const srbvmat t );
 }

//============================== end of file ===============================

# endif // SR_BV_MATH_H
