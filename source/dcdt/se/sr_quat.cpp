#include "precompiled.h"
#include "0ad_warning_disable.h"

# include "sr_quat.h"
# include "sr_vec.h"
# include "sr_random.h"

//============================== Static Data ====================================

const SrQuat SrQuat::null ( 1.0f, 0, 0, 0 );

//============================ public members ====================================

// from: Kuffner, "Effective Sampling and Distance Metrics for 3D Rigid Body Path Planning"
void SrQuat::random ()
 {
   float s = SrRandom::randf ();
   float s1 = sqrtf ( 1-s );
   float s2 = sqrtf ( s );
   float t1 = sr2pi * SrRandom::randf ();
   float t2 = sr2pi * SrRandom::randf ();
   w = cosf(t2) * s2;
   x = sinf(t1) * s1;
   y = cosf(t1) * s1;
   z = sinf(t2) * s2;
 }

void SrQuat::set ( const SrVec& v1, const SrVec& v2 )
 {
   SrVec axis = cross ( v1, v2 );
   set ( axis, ::angle(v1,v2) );
 }

void SrQuat::set ( const SrVec& axis, float radians )
 { 
   float f;
   
   // normalize axis:
   x=axis.x; y=axis.y; z=axis.z;
   f = x*x + y*y + z*z;
   if ( f>0 )
    { f = sqrtf ( f );
      x/=f; y/=f; z/=f;
    }
    
   // set the quaternion:
   radians/=2;
   f = sinf ( radians );
   x*=f; y*=f; z*=f;
   w = cosf ( radians );
 }

void SrQuat::set ( const SrVec& axisangle )
 { 
   float ang;
   
   // normalize axis ang extract angle:
   x=axisangle.x; y=axisangle.y; z=axisangle.z;
   ang = x*x + y*y + z*z;
   if ( ang>0 )
    { ang = sqrtf ( ang );
      x/=ang; y/=ang; z/=ang;
    }
    
   // set the quaternion:
   ang/=2;
   w = cosf ( ang );
   ang = sinf ( ang );
   x*=ang; y*=ang; z*=ang;
 }

void SrQuat::set ( const SrMat& m )
 {
   # define E(i)   m.get(i)
   # define M(i,j) m.get(i,j)
   # define Q(i)   *((&x)+i)
   float s;
   float tr = E(0) + E(5) + E(10);
    
   if ( tr>0 )
    { s = sqrtf ( 1.0f + tr );
      w = s / 2.0f;
      s = 0.5f / s;
      x = (E(6) - E(9)) * s;
      y = (E(8) - E(2)) * s;
      z = (E(1) - E(4)) * s;
    }
   else
    { int i, j, k;

      i = M(1,1)>M(0,0)? 1:0;
      if ( M(2,2)> M(i,i) ) i=2;
      j = (i+1)%3;
      k = (j+1)%3;

      s = sqrtf ( (M(i,i) - (M(j,j)+M(k,k))) + 1.0f );
      Q(i) = s * 0.5f;

      if ( s!=0 ) // s should never be equal to 0 if matrix is orthogonal 
       s = 0.5f / s;

      w    = (M(j,k) - M(k,j)) * s;
      Q(j) = (M(i,j) + M(j,i)) * s;
      Q(k) = (M(i,k) + M(k,i)) * s;
    }
   # undef E
   # undef M
   # undef Q
 }

void SrQuat::get ( SrVec& axis, float& radians ) const
 {
   // if SrQuat==(1,0,0,0), the axis will be null, so we
   // set the axis to (1,0,0) (SrVec::i); the angle will be 0.
   // this is also done in SrQuat::axis()
   axis.set ( x, y, z );
   float n = axis.norm();
   if ( n==0 ) axis=SrVec::i; else axis/=n;
   radians = 2.0f * acosf ( w );
 }

SrVec SrQuat::axis () const
 {
   SrVec axis ( x, y, z );
   float n = axis.norm();
   if ( n==0 ) axis=SrVec::i; else axis/=n;
   return axis;
 }

float SrQuat::angle () const
 {
   return 2.0f * acosf ( w );
 }

void SrQuat::normalize ()
 {
   float f = sqrtf( norm2() );
   if ( f==0 ) return;
   w /= f;
   x/=f; y/=f; z/=f;
   if ( w<0 ) { w=-w; x=-x; y=-y; z=-z; }
 }

SrQuat SrQuat::inverse() const
 {
   return conjugate();///norm();
 }

void SrQuat::invert () 
{
  //float n = norm();
  x=-x; y=-y; z=-z; // conjugate
  //w/=n; x/=n; y/=n; z/=n;
 }

inline void get_rot_mat ( float w, float x, float y, float z,
                          float& x1, float& y1, float& z1,
                          float& x2, float& y2, float& z2,
                          float& x3, float& y3, float& z3 )
 {
   x2  = x+x;
   float x2x = x2*x;
   float x2y = x2*y;
   float x2z = x2*z;
   float x2w = x2*w;
   y2  = y+y;
   float y2y = y2*y;
   float y2z = y2*z;
   float y2w = y2*w;
   z2  = z+z;
   float z2z = z2*z;
   float z2w = z2*w;

   x1 = 1.0f - y2y - z2z; y1 = x2y + z2w;        z1 = x2z - y2w;
   x2 = x2y - z2w;        y2 = 1.0f - x2x - z2z; z2 = y2z + x2w;
   x3 = x2z + y2w;        y3 = y2z - x2w;        z3 = 1.0f - x2x - y2y;

/* the transposed one is :
   setl1 ( 1.0f - y2y - z2z, x2y - z2w,        x2z + y2w,        0.0f );
   setl2 ( x2y + z2w,        1.0f - x2x - z2z, y2z - x2w,        0.0f );
   setl3 ( x2z - y2w,        y2z + x2w,        1.0f - x2x - y2y, 0.0f );
   setl4 ( 0,                0,                0,                1.0f );
   */
 }

void SrQuat::get_rot_mat ( float& a, float& b, float& c,
                           float& d, float& e, float& f,
                           float& g, float& h, float& i ) const
 {
   ::get_rot_mat ( w, x, y, z, a, b, c,
                               d, e, f,
                               g, h, i );
 }

SrMat& SrQuat::get_mat ( SrMat& m ) const
 {
   ::get_rot_mat ( w, x, y, z, m[0], m[1], m[2],
                               m[4], m[5], m[6],
                               m[8], m[9], m[10] );

   m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.0f;
   m[15] = 1.0f;

   return m;
 }

//=================================== Friend Functions ===================================

SrVec operator * ( const SrVec &v, const SrQuat &q )
 {
   SrQuat qv ( 0, v.x, v.y, v.z );
   qv = q * qv * q.conjugate();
   return SrVec ( qv.x, qv.y, qv.z );
 }

SrMat operator * ( const SrMat &m, const SrQuat &q )
 {
   SrMat mat;
   return m * q.get_mat(mat);
 }

SrMat operator * ( const SrQuat &q, const SrMat &m )
 {
   SrMat mat;
   return q.get_mat(mat) * m;
 }

SrQuat operator * ( const SrQuat &q1, const SrQuat &q2 )
 {
   SrQuat q;

   q.w = (q1.w*q2.w) - (q1.x*q2.x + q1.y*q2.y + q1.z*q2.z); // w1*w2-dot(v1,v2)
   q.x = q1.y*q2.z - q1.z*q2.y; // cross (q1.v,q2.v)
   q.y = q1.z*q2.x - q1.x*q2.z;
   q.z = q1.x*q2.y - q1.y*q2.x;
   q.x += (q1.x*q2.w) + (q2.x*q1.w);
   q.y += (q1.y*q2.w) + (q2.y*q1.w);
   q.z += (q1.z*q2.w) + (q2.z*q1.w);

   return q;
 }

bool operator == ( const SrQuat &q1, const SrQuat &q2 )
 { 
   return q1.w==q2.w && q1.x==q2.x && q1.y==q2.y && q1.z==q2.z ? true:false; 
 }

bool operator != ( const SrQuat &q1, const SrQuat &q2 )
 { 
   return q1.w!=q2.w && q1.x!=q2.x && q1.y!=q2.y && q1.z!=q2.z ? true:false; 
 }

void swap ( SrQuat &q1, SrQuat &q2 )
 {
   float tmp;
   SR_SWAP(q1.w,q2.w);
   SR_SWAP(q1.x,q2.x);
   SR_SWAP(q1.y,q2.y);
   SR_SWAP(q1.z,q2.z);
 }

SrQuat slerp ( SrQuat &q1, const SrQuat &q2, float t )
 { 
   float dot = q1.dot(q2);
   if ( dot < 0 ) 
    { // the quaternions are pointing in opposite directions, so
      // use the equivalent alternative representation for q1
      q1.set ( -q1.w, -q1.x, -q1.y, -q1.z );
      dot = -dot;
    }

   // interpolation factors
   float r, s;
   
   // decide according to an epsilon (30fps motions are of E-6 order)
   // this IS needed for baked motion to avoid dealing with the E-6 values in floats
   if ( 1.0f-dot < 0.05f )
    { // the quaternions are nearly parallel, so use linear interpolation
      r = 1-t;
      s = t;
    }
   else
    { // calculate spherical linear interpolation factors
      float a = acosf(dot);
      float g = 1.0f / sinf(a);
      r = sinf ( (1-t)*a ) * g;
      s = sinf ( t*a ) * g;
    }
    
   // set the interpolated quaternion
   SrQuat q ( r*q1.w + s*q2.w,
              r*q1.x + s*q2.x,
              r*q1.y + s*q2.y,
              r*q1.z + s*q2.z );
   q.normalize();
   return q;
 }

SrOutput& operator<< ( SrOutput& out, const SrQuat& q )
 {
   return out << "axis " << q.axis() << " ang " << SR_TODEG(q.angle());
 }

SrInput& operator>> ( SrInput& in, SrQuat& q )
 {
   SrVec axis;
   float ang;

   in.get_token(); // "axis"
   in >> axis;
   
   in.get_token(); // "ang"
   in >> ang;
   
   q.set ( axis, SR_TORAD(ang) );

   return in;
 }

//================================== End of File ===========================================

/*
double dist(const QUAT &left, const QUAT &right)
{
 return acos(dot_product(left,right));
}

// approximate distance: (Kuffner)
dot = dot_product(left,right); // in [-1,1]
return ( 1-||dot|| ) ;

(avec dot_product le produit membre a membre des 4 composantes du quaternion)

L'interpretation geometrique est toute simple: c'est la longueur de l'arc geodesique
 entre les 2 points sur la sphere unite en 4D, et donc cela vaut tout simplement... 
l'angle (puisque le rayon de la sphere est 1)
*/

