#include "precompiled.h"/* Note: this code was adapted from the PQP library; 
   their copyright notice can be found at the end of this file. */
#include "0ad_warning_disable.h"

# include "sr_bv_tree_query.h"

//====================== SrBvTreeQuery ======================

SrBvTreeQuery::SrBvTreeQuery ()
 {
   init ();
 }

void SrBvTreeQuery::init ()
 {
   _num_bv_tests = 0;
   _num_tri_tests = 0;
   _pairs.size(0);
   _pairs.compress();
   _dist = 0;
   _rel_err = _abs_err = 0;
   _closer_than_tolerance = false;
   _toler = 0;
 }

inline void setmv ( srbvmat m, srbvvec v, const SrMat& srm )
 {
   # define SRM(i) (srbvreal)(srm[i])
   // here we pass from column-major to line-major order:
   m[0][0]=SRM(0); m[1][0]=SRM(1); m[2][0]=SRM(2);
   m[0][1]=SRM(4); m[1][1]=SRM(5); m[2][1]=SRM(6);
   m[0][2]=SRM(8); m[1][2]=SRM(9); m[2][2]=SRM(10);
   v[0]=SRM(12); v[1]=SRM(13); v[2]=SRM(14);
   # undef SRM
 }


bool SrBvTreeQuery::collide ( const SrMat& m1, const SrBvTree* t1,
                              const SrMat& m2, const SrBvTree* t2 )
 {
   srbvmat R1, R2;
   srbvvec T1, T2;
   setmv ( R1, T1, m1 );
   setmv ( R2, T2, m2 );
   collide ( R1, T1, t1, R2, T2, t2, CollideFirstContact );
   return _pairs.size()>0? true:false;
 }

int SrBvTreeQuery::collide_all ( const SrMat& m1, const SrBvTree* t1,
                                 const SrMat& m2, const SrBvTree* t2 )
 {
   srbvmat R1, R2;
   srbvvec T1, T2;
   setmv ( R1, T1, m1 );
   setmv ( R2, T2, m2 );
   collide ( R1, T1, t1, R2, T2, t2, CollideAllContacts );
   return _pairs.size();
 }

float SrBvTreeQuery::distance ( const SrMat& m1, SrBvTree* t1,
                                const SrMat& m2, SrBvTree* t2,
                                float rel_err, float abs_err )
 {
   srbvmat R1, R2;
   srbvvec T1, T2;
   setmv ( R1, T1, m1 );
   setmv ( R2, T2, m2 );
   _distance ( R1, T1, t1, R2, T2, t2, rel_err, abs_err );
   _srp1.set ( _p1 );
   _srp2.set ( _p2 );
   return distance();
 }

bool SrBvTreeQuery::tolerance ( const SrMat& m1, SrBvTree* t1,
                                const SrMat& m2, SrBvTree* t2,
                                float tolerance )
 {
   srbvmat R1, R2;
   srbvvec T1, T2;
   setmv ( R1, T1, m1 );
   setmv ( R2, T2, m2 );
   return SrBvTreeQuery::tolerance ( R1, T1, t1, R2, T2, t2, tolerance );
 }

float SrBvTreeQuery::greedy_distance ( const SrMat& m1, SrBvTree* t1,
                                       const SrMat& m2, SrBvTree* t2, float delta )
 {
   srbvmat R1, R2;
   srbvvec T1, T2;
   setmv ( R1, T1, m1 );
   setmv ( R2, T2, m2 );
   return SrBvTreeQuery::greedy_distance ( R1, T1, t1, R2, T2, t2, delta );
 }

//====================== static functions ======================

inline srbvreal max ( srbvreal a, srbvreal b, srbvreal c )
{
  srbvreal t = a;
  if (b > t) t = b;
  if (c > t) t = c;
  return t;
}

inline srbvreal min ( srbvreal a, srbvreal b, srbvreal c )
{
  srbvreal t = a;
  if (b < t) t = b;
  if (c < t) t = c;
  return t;
}

static int project6 ( srbvreal *ax, 
         srbvreal *p1, srbvreal *p2, srbvreal *p3, 
         srbvreal *q1, srbvreal *q2, srbvreal *q3 )
{
  using namespace SrBvMath;
  srbvreal P1 = VdotV(ax, p1);
  srbvreal P2 = VdotV(ax, p2);
  srbvreal P3 = VdotV(ax, p3);
  srbvreal Q1 = VdotV(ax, q1);
  srbvreal Q2 = VdotV(ax, q2);
  srbvreal Q3 = VdotV(ax, q3);
  
  srbvreal mx1 = max(P1, P2, P3);
  srbvreal mn1 = min(P1, P2, P3);
  srbvreal mx2 = max(Q1, Q2, Q3);
  srbvreal mn2 = min(Q1, Q2, Q3);

  if (mn1 > mx2) return 0;
  if (mn2 > mx1) return 0;
  return 1;
}

// very robust triangle intersection test
// uses no divisions
// works on coplanar triangles
static int TriContact( 
           const srbvreal *P1, const srbvreal *P2, const srbvreal *P3,
           const srbvreal *Q1, const srbvreal *Q2, const srbvreal *Q3 )
{
  // One triangle is (p1,p2,p3).  Other is (q1,q2,q3).
  // Edges are (e1,e2,e3) and (f1,f2,f3).
  // Normals are n1 and m1
  // Outwards are (g1,g2,g3) and (h1,h2,h3).
  //  
  // We assume that the triangle vertices are in the same coordinate system.
  //
  // First thing we do is establish a new c.s. so that p1 is at (0,0,0).

  srbvreal p1[3], p2[3], p3[3];
  srbvreal q1[3], q2[3], q3[3];
  srbvreal e1[3], e2[3], e3[3];
  srbvreal f1[3], f2[3], f3[3];
  srbvreal g1[3], g2[3], g3[3];
  srbvreal h1[3], h2[3], h3[3];
  srbvreal n1[3], m1[3];

  srbvreal ef11[3], ef12[3], ef13[3];
  srbvreal ef21[3], ef22[3], ef23[3];
  srbvreal ef31[3], ef32[3], ef33[3];
  
  p1[0] = P1[0] - P1[0];  p1[1] = P1[1] - P1[1];  p1[2] = P1[2] - P1[2];
  p2[0] = P2[0] - P1[0];  p2[1] = P2[1] - P1[1];  p2[2] = P2[2] - P1[2];
  p3[0] = P3[0] - P1[0];  p3[1] = P3[1] - P1[1];  p3[2] = P3[2] - P1[2];
  
  q1[0] = Q1[0] - P1[0];  q1[1] = Q1[1] - P1[1];  q1[2] = Q1[2] - P1[2];
  q2[0] = Q2[0] - P1[0];  q2[1] = Q2[1] - P1[1];  q2[2] = Q2[2] - P1[2];
  q3[0] = Q3[0] - P1[0];  q3[1] = Q3[1] - P1[1];  q3[2] = Q3[2] - P1[2];
  
  e1[0] = p2[0] - p1[0];  e1[1] = p2[1] - p1[1];  e1[2] = p2[2] - p1[2];
  e2[0] = p3[0] - p2[0];  e2[1] = p3[1] - p2[1];  e2[2] = p3[2] - p2[2];
  e3[0] = p1[0] - p3[0];  e3[1] = p1[1] - p3[1];  e3[2] = p1[2] - p3[2];

  f1[0] = q2[0] - q1[0];  f1[1] = q2[1] - q1[1];  f1[2] = q2[2] - q1[2];
  f2[0] = q3[0] - q2[0];  f2[1] = q3[1] - q2[1];  f2[2] = q3[2] - q2[2];
  f3[0] = q1[0] - q3[0];  f3[1] = q1[1] - q3[1];  f3[2] = q1[2] - q3[2];
  
  using namespace SrBvMath;

  VcrossV(n1, e1, e2);
  VcrossV(m1, f1, f2);

  VcrossV(g1, e1, n1);
  VcrossV(g2, e2, n1);
  VcrossV(g3, e3, n1);
  VcrossV(h1, f1, m1);
  VcrossV(h2, f2, m1);
  VcrossV(h3, f3, m1);

  VcrossV(ef11, e1, f1);
  VcrossV(ef12, e1, f2);
  VcrossV(ef13, e1, f3);
  VcrossV(ef21, e2, f1);
  VcrossV(ef22, e2, f2);
  VcrossV(ef23, e2, f3);
  VcrossV(ef31, e3, f1);
  VcrossV(ef32, e3, f2);
  VcrossV(ef33, e3, f3);
  
  // now begin the series of tests

  if (!project6(n1, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(m1, p1, p2, p3, q1, q2, q3)) return 0;
  
  if (!project6(ef11, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(ef12, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(ef13, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(ef21, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(ef22, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(ef23, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(ef31, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(ef32, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(ef33, p1, p2, p3, q1, q2, q3)) return 0;

  if (!project6(g1, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(g2, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(g3, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(h1, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(h2, p1, p2, p3, q1, q2, q3)) return 0;
  if (!project6(h3, p1, p2, p3, q1, q2, q3)) return 0;

  return 1;
}

/**************************/
/******* COLLIDE **********/
/**************************/

void SrBvTreeQuery::collide (
            srbvmat R1, srbvvec T1, const SrBvTree* o1,
            srbvmat R2, srbvvec T2, const SrBvTree* o2,
            CollideFlag flag )
{
  // clear the data:
  _num_bv_tests = 0;
  _num_tri_tests = 0;
  _pairs.size(0);

  // make sure that the models are built:
  if ( o1->nodes()==0 || o2->nodes()==0 ) return;

  // Okay, compute what transform [R,T] that takes us from cs1 to cs2.
  // [R,T] = [R1,T1]'[R2,T2] = [R1',-R1'T][R2,T2] = [R1'R2, R1'(T2-T1)]
  // First compute the rotation part, then translation part
  using namespace SrBvMath;
  MTxM(_R,R1,R2);
  srbvvec Ttemp;
  VmV(Ttemp, T2, T1);  
  MTxV(_T, R1, Ttemp);
  
  // compute the transform from o1->child(0) to o2->child(0)
  srbvmat Rtemp, R;
  srbvvec T;
  MxM(Rtemp,_R,o2->const_child(0)->R);
  MTxM(R,o1->const_child(0)->R,Rtemp);
  MxVpV(Ttemp,_R,o2->const_child(0)->To,_T);
  VmV(Ttemp,Ttemp,o1->const_child(0)->To);
  MTxV(T,o1->const_child(0)->R,Ttemp);

  // now start with both top level BVs  
  _collide_recurse ( R, T, o1, 0, o2, 0, flag );
}

float SrBvTreeQuery::greedy_distance ( srbvmat R1, srbvvec T1, SrBvTree* o1,
			                           srbvmat R2, srbvvec T2, SrBvTree* o2, float delta )
 {
  using namespace SrBvMath;
  MTxM(_R,R1,R2);
  double Ttemp[3];
  VmV(Ttemp, T2, T1);  
  MTxV(_T, R1, Ttemp);
  
  _num_bv_tests = 0;
  _num_tri_tests = 0;

  srbvmat Rtemp, R;
  srbvvec T;

  MxM(Rtemp,_R,o2->child(0)->R);
  MTxM(R,o1->child(0)->R,Rtemp);
  
  MxVpV(Ttemp,_R,o2->child(0)->Tr,_T);
  VmV(Ttemp,Ttemp,o1->child(0)->Tr);
  MTxV(T,o1->child(0)->R,Ttemp);

  // first, see if top-level BVs are separated
  _num_bv_tests += 1;
  srbvreal d = SrBv::distance(R, T, o1->child(0), o2->child(0));

  // if we already found separation don't bother recursing to refine it
  if ( float(d) > delta )
    return (float)d;
  else // if top-level BVs are not separated, recurse
    return (float)_greedy_distance_recurse(R,T,o1,0,o2,0,delta);
 }

//====================== private methods ======================

inline
srbvreal
TriDistance(srbvreal R[3][3], srbvreal T[3], const SrBvTri *t1, const SrBvTri *t2,
            srbvreal p[3], srbvreal q[3])
{
  // transform tri 2 into same space as tri 1

  srbvreal tri1[3][3], tri2[3][3];
  using namespace SrBvMath;
  VcV(tri1[0], t1->p1);
  VcV(tri1[1], t1->p2);
  VcV(tri1[2], t1->p3);
  MxVpV(tri2[0], R, t2->p1, T);
  MxVpV(tri2[1], R, t2->p2, T);
  MxVpV(tri2[2], R, t2->p3, T);
                                
  return TriDist(p,q,tri1,tri2);
}

void SrBvTreeQuery::_collide_recurse (
               srbvmat R, srbvvec T, // b2 relative to b1
               const SrBvTree* o1, int b1, 
               const SrBvTree* o2, int b2, int flag)
{
  // first thing, see if we're overlapping

  _num_bv_tests++;

  if ( !SrBv::overlap(R,T,o1->const_child(b1), o2->const_child(b2))) return;

  // if we are, see if we test triangles next

  bool l1 = o1->const_child(b1)->leaf();
  bool l2 = o2->const_child(b2)->leaf();
  using namespace SrBvMath;

  if (l1 && l2) 
  {
    _num_tri_tests++;

    // transform the points in b2 into space of b1, then compare
    const SrBvTri *t1 = o1->tri(-o1->const_child(b1)->first_child - 1);
    const SrBvTri *t2 = o2->tri(-o2->const_child(b2)->first_child - 1);
    srbvreal q1[3], q2[3], q3[3];
    const srbvreal *p1 = t1->p1;
    const srbvreal *p2 = t1->p2;
    const srbvreal *p3 = t1->p3;    
    MxVpV(q1, _R, t2->p1, _T);
    MxVpV(q2, _R, t2->p2, _T);
    MxVpV(q3, _R, t2->p3, _T);
    if (TriContact(p1, p2, p3, q1, q2, q3)) 
    { // add this to result
      _pairs.push() = t1->id;
      _pairs.push() = t2->id;
    }
    return;
  }

  // we dont, so decide whose children to visit next

  srbvreal sz1 = o1->const_child(b1)->size();
  srbvreal sz2 = o2->const_child(b2)->size();

  srbvreal Rc[3][3],Tc[3],Ttemp[3];
    
  if (l2 || (!l1 && (sz1 > sz2)))
  {
    int c1 = o1->const_child(b1)->first_child;
    int c2 = c1 + 1;

    MTxM(Rc,o1->const_child(c1)->R,R);
    VmV(Ttemp,T,o1->const_child(c1)->To);
    MTxV(Tc,o1->const_child(c1)->R,Ttemp);
    _collide_recurse(Rc,Tc,o1,c1,o2,b2,flag);

    if ( flag==CollideFirstContact && _pairs.size()>0 ) return;

    MTxM(Rc,o1->const_child(c2)->R,R);
    VmV(Ttemp,T,o1->const_child(c2)->To);
    MTxV(Tc,o1->const_child(c2)->R,Ttemp);
    _collide_recurse(Rc,Tc,o1,c2,o2,b2,flag);
  }
  else 
  {
    int c1 = o2->const_child(b2)->first_child;
    int c2 = c1 + 1;

    MxM(Rc,R,o2->const_child(c1)->R);
    MxVpV(Tc,R,o2->const_child(c1)->To,T);
    _collide_recurse(Rc,Tc,o1,b1,o2,c1,flag);

    if ( flag==CollideFirstContact && _pairs.size()>0 ) return;

    MxM(Rc,R,o2->const_child(c2)->R);
    MxVpV(Tc,R,o2->const_child(c2)->To,T);
    _collide_recurse(Rc,Tc,o1,b1,o2,c2,flag);
  }
}

/***************************/
/******* DISTANCE **********/
/***************************/
void SrBvTreeQuery::_distance ( srbvmat R1, srbvvec T1, SrBvTree* o1,
                                srbvmat R2, srbvvec T2, SrBvTree* o2,
                                srbvreal rel_err, srbvreal abs_err )
 {
  // init data:
  _dist = 0;
  _num_bv_tests = 0;
  _num_tri_tests = 0;
  _abs_err = abs_err;
  _rel_err = rel_err;

  // make sure that the models are built:
  if ( o1->nodes()==0 || o2->nodes()==0 ) return;

  // Okay, compute what transform [R,T] that takes us from cs2 to cs1.
  // [R,T] = [R1,T1]'[R2,T2] = [R1',-R1'T][R2,T2] = [R1'R2, R1'(T2-T1)]
  // First compute the rotation part, then translation part
  using namespace SrBvMath;

  MTxM(_R,R1,R2);
  srbvreal Ttemp[3];
  VmV(Ttemp, T2, T1);  
  MTxV(_T, R1, Ttemp);
  
  // establish initial upper bound using last triangles which 
  // provided the minimum distance
  srbvvec p,q;
  _dist = TriDistance(_R,_T,o1->_last_tri,o2->_last_tri,p,q);
  VcV(_p1,p);
  VcV(_p2,q);
  
  // compute the transform from o1->child(0) to o2->child(0)
  srbvreal Rtemp[3][3], R[3][3], T[3];

  MxM(Rtemp,_R,o2->const_child(0)->R);
  MTxM(R,o1->const_child(0)->R,Rtemp);
  
  MxVpV(Ttemp,_R,o2->const_child(0)->Tr,_T);
  VmV(Ttemp,Ttemp,o1->const_child(0)->Tr);
  MTxV(T,o1->const_child(0)->R,Ttemp);

  _distance_recurse(R,T,o1,0,o2,0);    

  // res->p2 is in cs 1 ; transform it to cs 2
  srbvvec u;
  VmV(u, _p2, _T);
  MTxV(_p2, _R, u);
 }

void SrBvTreeQuery::_distance_recurse (
                srbvmat R, srbvvec T, // b2 relative to b1
                SrBvTree* o1, int b1,
                SrBvTree* o2, int b2 )
{
  using namespace SrBvMath;

  srbvreal sz1 = o1->const_child(b1)->size();
  srbvreal sz2 = o2->const_child(b2)->size();
  bool l1 = o1->const_child(b1)->leaf();
  bool l2 = o2->const_child(b2)->leaf();

  if ( l1 && l2 )
  { // both leaves.  Test the triangles beneath them.
    _num_tri_tests++;

    srbvreal p[3], q[3];

    SrBvTri *t1 = &o1->_tris[-o1->child(b1)->first_child - 1];
    SrBvTri *t2 = &o2->_tris[-o2->child(b2)->first_child - 1];

    srbvreal d = TriDistance(_R,_T,t1,t2,p,q);
  
    if ( d<_dist ) 
    {
      _dist = d;

      VcV(_p1, p);         // p already in c.s. 1
      VcV(_p2, q);         // q must be transformed 
                               // into c.s. 2 later
      o1->_last_tri = t1;
      o2->_last_tri = t2;
    }
    return;
  }

  // First, perform distance tests on the children. Then traverse 
  // them recursively, but test the closer pair first, the further 
  // pair second.

  int a1,a2,c1,c2;  // new bv tests 'a' and 'c'
  srbvreal R1[3][3], T1[3], R2[3][3], T2[3], Ttemp[3];

  if (l2 || (!l1 && (sz1 > sz2)))
  {
    // visit the children of b1

    a1 = o1->child(b1)->first_child;
    a2 = b2;
    c1 = o1->child(b1)->first_child+1;
    c2 = b2;
    
    MTxM(R1,o1->const_child(a1)->R,R);
    VmV(Ttemp,T,o1->const_child(a1)->Tr);
    MTxV(T1,o1->const_child(a1)->R,Ttemp);

    MTxM(R2,o1->const_child(c1)->R,R);
    VmV(Ttemp,T,o1->const_child(c1)->Tr);
    MTxV(T2,o1->const_child(c1)->R,Ttemp);
  }
  else 
  {
    // visit the children of b2
    a1 = b1;
    a2 = o2->child(b2)->first_child;
    c1 = b1;
    c2 = o2->child(b2)->first_child+1;

    MxM(R1,R,o2->const_child(a2)->R);
    MxVpV(T1,R,o2->const_child(a2)->Tr,T);

    MxM(R2,R,o2->const_child(c2)->R);
    MxVpV(T2,R,o2->const_child(c2)->Tr,T);
  }

  _num_bv_tests += 2;

  srbvreal d1 = SrBv::distance(R1, T1, o1->const_child(a1), o2->const_child(a2));
  srbvreal d2 = SrBv::distance(R2, T2, o1->const_child(c1), o2->const_child(c2));

  if (d2 < d1)
  {
    if ((d2 < (_dist - _abs_err)) || 
        (d2*(1 + _rel_err) < _dist)) 
    {      
      _distance_recurse(R2, T2, o1, c1, o2, c2);      
    }

    if ((d1 < (_dist - _abs_err)) || 
        (d1*(1 + _rel_err) < _dist)) 
    {      
      _distance_recurse(R1, T1, o1, a1, o2, a2);
    }
  }
  else 
  {
    if ((d1 < (_dist - _abs_err)) || 
        (d1*(1 + _rel_err) < _dist)) 
    {      
      _distance_recurse(R1, T1, o1, a1, o2, a2);
    }

    if ((d2 < (_dist - _abs_err)) || 
        (d2*(1 + _rel_err) < _dist)) 
    {      
      _distance_recurse(R2, T2, o1, c1, o2, c2);      
    }
  }
}

/***************************/
/******* TOLERANCE *********/
/***************************/

void SrBvTreeQuery::_tolerance (  srbvmat R1, srbvvec T1, SrBvTree* o1,
                                  srbvmat R2, srbvvec T2, SrBvTree* o2,
                                  srbvreal tolerance )
{
  using namespace SrBvMath;

  // init data:
//  _dist = 0;
  _num_bv_tests = 0;
  _num_tri_tests = 0;
  if ( tolerance<0 ) tolerance=0.0;
  _toler = tolerance;
  _closer_than_tolerance = false;

  // make sure that the models are built:
  if ( o1->nodes()==0 || o2->nodes()==0 ) return;
  
  // Compute the transform [R,T] that takes us from cs2 to cs1.
  // [R,T] = [R1,T1]'[R2,T2] = [R1',-R1'T][R2,T2] = [R1'R2, R1'(T2-T1)]

  MTxM(_R,R1,R2);
  srbvreal Ttemp[3];
  VmV(Ttemp, T2, T1);
  MTxV(_T, R1, Ttemp);

  // compute the transform from o1->child(0) to o2->child(0)
  srbvreal Rtemp[3][3], R[3][3], T[3];

  MxM(Rtemp,_R,o2->child(0)->R);
  MTxM(R,o1->child(0)->R,Rtemp);
  MxVpV(Ttemp,_R,o2->child(0)->Tr,_T);
  VmV(Ttemp,Ttemp,o1->child(0)->Tr);
  MTxV(T,o1->child(0)->R,Ttemp);

  // find a distance lower bound for trivial reject

  srbvreal d = SrBv::distance(R, T, o1->child(0), o2->child(0));
  
  if ( d <= _toler )
  { _tolerance_recurse ( R, T, o1, 0, o2, 0 );
  }

  // res->p2 is in cs 1 ; transform it to cs 2
  srbvreal u[3];
  VmV(u, _p2, _T);
  MTxV(_p2, _R, u);
}

void SrBvTreeQuery::_tolerance_recurse 
                    ( srbvmat R, srbvvec T,
                      SrBvTree* o1, int b1,
                      SrBvTree* o2, int b2 )
{
  using namespace SrBvMath;

  srbvreal sz1 = o1->child(b1)->size();
  srbvreal sz2 = o2->child(b2)->size();
  int l1 = o1->child(b1)->leaf();
  int l2 = o2->child(b2)->leaf();

  if (l1 && l2) 
  {
    // both leaves - find if tri pair within tolerance
    
    _num_tri_tests++;

    srbvreal p[3], q[3];

    SrBvTri *t1 = &o1->_tris[-o1->child(b1)->first_child - 1];
    SrBvTri *t2 = &o2->_tris[-o2->child(b2)->first_child - 1];

    srbvreal d = TriDistance(_R,_T,t1,t2,p,q);
    
    if ( d<=_toler )  
    {  
      // triangle pair distance less than tolerance
      _closer_than_tolerance = true;
      _dist = d;
      VcV(_p1, p);         // p already in c.s. 1
      VcV(_p2, q);         // q must be transformed into c.s. 2 later
    }
    return;
  }

  int a1,a2,c1,c2;  // new bv tests 'a' and 'c'
  srbvreal R1[3][3], T1[3], R2[3][3], T2[3], Ttemp[3];

  if (l2 || (!l1 && (sz1 > sz2)))
  {
    // visit the children of b1

    a1 = o1->child(b1)->first_child;
    a2 = b2;
    c1 = o1->child(b1)->first_child+1;
    c2 = b2;
    
    MTxM(R1,o1->child(a1)->R,R);
    VmV(Ttemp,T,o1->child(a1)->Tr);
    MTxV(T1,o1->child(a1)->R,Ttemp);

    MTxM(R2,o1->child(c1)->R,R);
    VmV(Ttemp,T,o1->child(c1)->Tr);
    MTxV(T2,o1->child(c1)->R,Ttemp);
  }
  else 
  {
    // visit the children of b2

    a1 = b1;
    a2 = o2->child(b2)->first_child;
    c1 = b1;
    c2 = o2->child(b2)->first_child+1;

    MxM(R1,R,o2->child(a2)->R);
    MxVpV(T1,R,o2->child(a2)->Tr,T);
    MxM(R2,R,o2->child(c2)->R);
    MxVpV(T2,R,o2->child(c2)->Tr,T);
  }

  _num_bv_tests += 2;

  srbvreal d1 = SrBv::distance(R1, T1, o1->child(a1), o2->child(a2));
  srbvreal d2 = SrBv::distance(R2, T2, o1->child(c1), o2->child(c2));

  if (d2 < d1) 
  {
    if (d2 <= _toler) _tolerance_recurse( R2, T2, o1, c1, o2, c2);
    if (_closer_than_tolerance) return;
    if (d1 <= _toler) _tolerance_recurse( R1, T1, o1, a1, o2, a2);
  }
  else 
  {
    if (d1 <= _toler) _tolerance_recurse( R1, T1, o1, a1, o2, a2);
    if (_closer_than_tolerance) return;
    if (d2 <= _toler) _tolerance_recurse( R2, T2, o1, c1, o2, c2);
  }
}


srbvreal SrBvTreeQuery::_greedy_distance_recurse ( 
                        srbvmat R, srbvvec T, SrBvTree* o1, int b1,
                        SrBvTree* o2, int b2, srbvreal delta )
{
  using namespace SrBvMath;

  int l1 = o1->child(b1)->leaf();
  int l2 = o2->child(b2)->leaf();

  if (l1 && l2)
  {
    // both leaves.  return their distance
    _num_tri_tests++;

    double p[3], q[3];

    const SrBvTri *t1 = o1->tri(-o1->child(b1)->first_child - 1);
    const SrBvTri *t2 = o2->tri(-o2->child(b2)->first_child - 1);

    return TriDistance(_R,_T,t1,t2,p,q);  
  }

  // First, perform distance tests on the children. Then traverse 
  // them recursively, but test the closer pair first, the further 
  // pair second.

  int a1,a2,c1,c2;  // new bv tests 'a' and 'c'
  double R1[3][3], T1[3], R2[3][3], T2[3], Ttemp[3];

  srbvreal sz1 = o1->child(b1)->size();
  srbvreal sz2 = o2->child(b2)->size();

  if (l2 || (!l1 && (sz1 > sz2)))
  {
    // visit the children of b1

    a1 = o1->child(b1)->first_child;
    a2 = b2;
    c1 = o1->child(b1)->first_child+1;
    c2 = b2;
    
    MTxM(R1,o1->child(a1)->R,R);
    VmV(Ttemp,T,o1->child(a1)->Tr);
    MTxV(T1,o1->child(a1)->R,Ttemp);

    MTxM(R2,o1->child(c1)->R,R);
    VmV(Ttemp,T,o1->child(c1)->Tr);
    MTxV(T2,o1->child(c1)->R,Ttemp);
  }
  else 
  {
    // visit the children of b2

    a1 = b1;
    a2 = o2->child(b2)->first_child;
    c1 = b1;
    c2 = o2->child(b2)->first_child+1;

    MxM(R1,R,o2->child(a2)->R);
    MxVpV(T1,R,o2->child(a2)->Tr,T);

    MxM(R2,R,o2->child(c2)->R);
    MxVpV(T2,R,o2->child(c2)->Tr,T);
  }

  double d1 = SrBv::distance(R1, T1, o1->child(a1), o2->child(a2));
  double d2 = SrBv::distance(R2, T2, o1->child(c1), o2->child(c2));
  _num_bv_tests += 2;

  // if we already found separation, don't further recurse to refine it
  double min_d1d2 = (d1 < d2) ? d1 : d2;
  if ( min_d1d2 > delta )
    return min_d1d2;

  // else recurse
  if (d2 < d1) // and thus at least d2 <= delta (d1 maybe too)
  {
    srbvreal alpha = _greedy_distance_recurse(R2, T2, o1, c1, o2, c2, delta);
    
    if ( alpha > delta ) {
      if ( d1 > delta )
	return (alpha < d1)? alpha: d1;
      srbvreal beta = _greedy_distance_recurse(R1, T1, o1, a1, o2, a2, delta);
      if ( beta > delta )
	return (alpha < beta)? alpha: beta;
    }
    return 0;
  }
  else // at least d1 <= delta (d2 maybe too)
  {
    srbvreal alpha = _greedy_distance_recurse(R1, T1, o1, a1, o2, a2, delta);

    if ( alpha > delta ) {
      if ( d2 > delta )
	return (alpha < d2)? alpha: d2;
      srbvreal beta = _greedy_distance_recurse(R2, T2, o1, c1, o2, c2, delta);
      if ( beta > delta )
	return (alpha < beta)? alpha: beta;
    }
    return 0;
  }
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
