#include "precompiled.h"/* Note: this code was adapted from the PQP library; 
   their copyright notice can be found at the end of this file. */
#include "0ad_warning_disable.h"

# include "sr_bv_tree.h"
# include "sr_model.h"

//====================================
//======= SrBvTree::SrBvTree =========
//====================================

SrBvTree::SrBvTree ()
 {
   _last_tri=0;
 }

//=====================================
//======= SrBvTree::~SrBvTree =========
//=====================================
SrBvTree::~SrBvTree ()
 {
 }

//================================
//======= SrBvTree::init =========
//================================
void SrBvTree::init ()
 {
   _tris.size(0); _tris.compress();
   _bvs.size(0); _bvs.compress();
   _last_tri=0;
 }

//====================================
//======= SrBvTree::make =========
//====================================
void SrBvTree::make ( const SrModel& m )
 {
   using namespace SrBvMath;

   if ( m.F.size()==0 ) { init(); return; }

   // get the geometry of the model:
   _tris.size ( m.F.size() ); 
   int i;
   for ( i=0; i<_tris.size(); i++ )
    { Vset ( _tris[i].p1, m.V[m.F[i].a] );
      Vset ( _tris[i].p2, m.V[m.F[i].b] );
      Vset ( _tris[i].p3, m.V[m.F[i].c] );
      _tris[i].id = i;
    }
   _last_tri = &_tris[0];

   // create an array of BVs for the model:
   _bvs.ensure_capacity ( 2*_tris.size()-1 );
   _bvs.size ( 1 );

   // build recursively the tree:
   _build_recurse ( 0/*node*/, 0/*first*/, _tris.size()/*num*/ );

   // change BV orientations from world-relative to parent-relative:
   srbvmat R; srbvvec T;
   Midentity(R);
   Videntity(T);
   _make_parent_relative(0,R,T,T);

   // make sure the sizes were well defined:
   _tris.compress();
   _bvs.compress();
 }

static void get_centroid_triverts ( srbvvec c, SrBvTri *tris, int num_tris )
{
  int i;

  c[0] = c[1] = c[2] = 0.0;

  // get center of mass
  for(i=0; i<num_tris; i++)
  {
    srbvreal *p1 = tris[i].p1;
    srbvreal *p2 = tris[i].p2;
    srbvreal *p3 = tris[i].p3;

    c[0] += p1[0] + p2[0] + p3[0];
    c[1] += p1[1] + p2[1] + p3[1];
    c[2] += p1[2] + p2[2] + p3[2];      
  }

  srbvreal n = (srbvreal)(3 * num_tris);

  c[0] /= n;
  c[1] /= n;
  c[2] /= n;
}

static void get_covariance_triverts ( srbvmat M, SrBvTri *tris, int num_tris )
{
  int i;
  srbvreal S1[3];
  srbvreal S2[3][3];

  S1[0] = S1[1] = S1[2] = 0.0;
  S2[0][0] = S2[1][0] = S2[2][0] = 0.0;
  S2[0][1] = S2[1][1] = S2[2][1] = 0.0;
  S2[0][2] = S2[1][2] = S2[2][2] = 0.0;

  // get center of mass
  for(i=0; i<num_tris; i++)
  {
    srbvreal *p1 = tris[i].p1;
    srbvreal *p2 = tris[i].p2;
    srbvreal *p3 = tris[i].p3;

    S1[0] += p1[0] + p2[0] + p3[0];
    S1[1] += p1[1] + p2[1] + p3[1];
    S1[2] += p1[2] + p2[2] + p3[2];

    S2[0][0] += (p1[0] * p1[0] +  
                 p2[0] * p2[0] +  
                 p3[0] * p3[0]);
    S2[1][1] += (p1[1] * p1[1] +  
                 p2[1] * p2[1] +  
                 p3[1] * p3[1]);
    S2[2][2] += (p1[2] * p1[2] +  
                 p2[2] * p2[2] +  
                 p3[2] * p3[2]);
    S2[0][1] += (p1[0] * p1[1] +  
                 p2[0] * p2[1] +  
                 p3[0] * p3[1]);
    S2[0][2] += (p1[0] * p1[2] +  
                 p2[0] * p2[2] +  
                 p3[0] * p3[2]);
    S2[1][2] += (p1[1] * p1[2] +  
                 p2[1] * p2[2] +  
                 p3[1] * p3[2]);
  }

  srbvreal n = (srbvreal)(3 * num_tris);

  // now get covariances

  M[0][0] = S2[0][0] - S1[0]*S1[0] / n;
  M[1][1] = S2[1][1] - S1[1]*S1[1] / n;
  M[2][2] = S2[2][2] - S1[2]*S1[2] / n;
  M[0][1] = S2[0][1] - S1[0]*S1[1] / n;
  M[1][2] = S2[1][2] - S1[1]*S1[2] / n;
  M[0][2] = S2[0][2] - S1[0]*S1[2] / n;
  M[1][0] = M[0][1];
  M[2][0] = M[0][2];
  M[2][1] = M[1][2];
}

// given a list of triangles, a splitting axis, and a coordinate on
// that axis, partition the triangles into two groups according to
// where their centroids fall on the axis (under axial projection).
// Returns the number of tris in the first half
static int split_tris ( SrBvTri* tris, int num_tris, srbvvec a, srbvreal c )
{
  int i;
  int c1 = 0;
  srbvvec p;
  srbvreal x;
  SrBvTri temp;

  using namespace SrBvMath;

  for(i = 0; i < num_tris; i++)
  {
    // loop invariant: up to (but not including) index c1 in group 1,
    // then up to (but not including) index i in group 2
    //
    //  [1] [1] [1] [1] [2] [2] [2] [x] [x] ... [x]
    //                   c1          i
    //
    VcV(p, tris[i].p1);
    VpV(p, p, tris[i].p2);
    VpV(p, p, tris[i].p3);      
    x = VdotV(p, a);
    x /= 3.0;
    if (x <= c)
    {
	    // group 1
	    temp = tris[i];
	    tris[i] = tris[c1];
	    tris[c1] = temp;
	    c1++;
    }
    else
    {
	    // group 2 -- do nothing
    }
  }

  // split arbitrarily if one group empty

  if ((c1 == 0) || (c1 == num_tris)) c1 = num_tris/2;

  return c1;
}

// Fits m->child(bn) to the num_tris triangles starting at first_tri
// Then, if num_tris is greater than one, partitions the tris into two
// sets, and recursively builds two children of m->child(bn)
void SrBvTree::_build_recurse ( int bn, int first_tri, int num_tris )
{
  using namespace SrBvMath;

  SrBv* b = child ( bn );

  // compute a rotation matrix

  srbvreal C[3][3], E[3][3], R[3][3], s[3], axis[3], mean[3], coord;

  get_covariance_triverts(C,&_tris[first_tri],num_tris);

  Meigen(E, s, C);

  // place axes of E in order of increasing s

  int min, mid, max;
  if (s[0] > s[1]) { max = 0; min = 1; }
  else { min = 0; max = 1; }
  if (s[2] < s[min]) { mid = min; min = 2; }
  else if (s[2] > s[max]) { mid = max; max = 2; }
  else { mid = 2; }
  McolcMcol(R,0,E,max);
  McolcMcol(R,1,E,mid);
  R[0][2] = E[1][max]*E[2][mid] - E[1][mid]*E[2][max];
  R[1][2] = E[0][mid]*E[2][max] - E[0][max]*E[2][mid];
  R[2][2] = E[0][max]*E[1][mid] - E[0][mid]*E[1][max];

  // fit the BV
  b->fit ( R, &_tris[first_tri], num_tris );

  if (num_tris == 1)
  {
    // BV is a leaf BV - first_child will index a triangle
    b->first_child = -(first_tri + 1);
  }
  else if (num_tris > 1)
  {
    // BV not a leaf - first_child will index a BV
    b->first_child = _bvs.size();
    _bvs.push();
    _bvs.push();

    // choose splitting axis and splitting coord
    McolcV(axis,R,0);

    get_centroid_triverts(mean,&_tris[first_tri],num_tris);
    coord = VdotV(axis, mean);

    // now split
    int num_first_half = split_tris(&_tris[first_tri], num_tris, 
                                    axis, coord);

    // recursively build the children
    _build_recurse( child(bn)->first_child, first_tri, num_first_half); 
    _build_recurse( child(bn)->first_child + 1,
                  first_tri + num_first_half, num_tris - num_first_half); 
  }
}

// this descends the hierarchy, converting world-relative 
// transforms to parent-relative transforms
void SrBvTree::_make_parent_relative ( int bn,
                                       const srbvmat parentR,
                                       const srbvvec parentTr,
                                       const srbvvec parentTo )
{
  srbvmat Rpc;
  srbvvec Tpc;

  if ( !child(bn)->leaf() )
  { // make children parent-relative
    _make_parent_relative ( child(bn)->first_child, 
                            child(bn)->R,
                            child(bn)->Tr,
                            child(bn)->To
                          );
    _make_parent_relative ( child(bn)->first_child+1, 
                            child(bn)->R,
                            child(bn)->Tr,
                            child(bn)->To
                          );
  }

  // make self parent relative
  using namespace SrBvMath;
  MTxM(Rpc,parentR,child(bn)->R);
  McM(child(bn)->R,Rpc);
  VmV(Tpc,child(bn)->Tr,parentTr);
  MTxV(child(bn)->Tr,parentR,Tpc);
  VmV(Tpc,child(bn)->To,parentTo);
  MTxV(child(bn)->To,parentR,Tpc);
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
