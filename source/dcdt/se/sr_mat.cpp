#include "precompiled.h"
# include <math.h>

# include "sr_mat.h"
//# include <SR/sr_utils.h>

//================================== Static Data ===================================

const SrMat SrMat::null ( 0.0, 0.0, 0.0, 0.0, 
                          0.0, 0.0, 0.0, 0.0, 
                          0.0, 0.0, 0.0, 0.0, 
                          0.0, 0.0, 0.0, 0.0 );

const SrMat SrMat::id   ( 1.0, 0.0, 0.0, 0.0, 
                          0.0, 1.0, 0.0, 0.0, 
                          0.0, 0.0, 1.0, 0.0, 
                          0.0, 0.0, 0.0, 1.0 );

# define E11 e[0]
# define E12 e[1]
# define E13 e[2]
# define E14 e[3]
# define E21 e[4]
# define E22 e[5]
# define E23 e[6]
# define E24 e[7]
# define E31 e[8]
# define E32 e[9]
# define E33 e[10]
# define E34 e[11]
# define E41 e[12]
# define E42 e[13]
# define E43 e[14]
# define E44 e[15]

//==================================== SrMat ========================================


SrMat::SrMat ( const float *p )
 {
   SR_ASSERT ( p );
   set ( p );
 } 

SrMat::SrMat ( const double *p )
 {
   SR_ASSERT ( p );
   set ( p );
 }

SrMat::SrMat ( float a, float b, float c, float d,
               float e, float f, float g, float h, 
               float i, float j, float k, float l,
               float m, float n, float o, float p )
 {
   setl1 ( a, b, c, d );
   setl2 ( e, f, g, h );
   setl3 ( i, j, k, l );
   setl4 ( m, n, o, p );
 }

void SrMat::set ( const float *p )
 {
   SR_ASSERT ( p );
   setl1 ( p[0],  p[1],  p[2],  p[3]  );
   setl2 ( p[4],  p[5],  p[6],  p[7]  );
   setl3 ( p[8],  p[9],  p[10], p[11] );
   setl4 ( p[12], p[13], p[14], p[15] );
 }

void SrMat::set ( const double *p )
 {
   SR_ASSERT ( p );
   setl1 ( (float)p[0],  (float)p[1],  (float)p[2],  (float)p[3]  );
   setl2 ( (float)p[4],  (float)p[5],  (float)p[6],  (float)p[7]  );
   setl3 ( (float)p[8],  (float)p[9],  (float)p[10], (float)p[11] );
   setl4 ( (float)p[12], (float)p[13], (float)p[14], (float)p[15] );
 }

void SrMat::round ( float epsilon )
 {
   int i;
   for ( i=0; i<16; i++ )
    if ( e[i]>=-epsilon && e[i]<=epsilon ) e[i]=0;
 }

void SrMat::transpose ()
 {
   float tmp;
   SR_SWAP ( E12, E21 );
   SR_SWAP ( E13, E31 );
   SR_SWAP ( E14, E41 );
   SR_SWAP ( E23, E32 );
   SR_SWAP ( E24, E42 );
   SR_SWAP ( E34, E43 );
 }

void SrMat::transpose3x3 ()
 {
   float tmp;
   SR_SWAP ( E12, E21 );
   SR_SWAP ( E13, E31 );
   SR_SWAP ( E23, E32 );
   SR_SWAP ( E34, E43 );
 }

void SrMat::translation ( float tx, float ty, float tz )
 {
   setl1 ( 1.0, 0.0, 0.0, 0.0 );
   setl2 ( 0.0, 1.0, 0.0, 0.0 );
   setl3 ( 0.0, 0.0, 1.0, 0.0 );
   setl4 (  tx,  ty,  tz, 1.0 );
 }

void SrMat::left_combine_translation ( const SrVec &v )
 {
   E41 = v.x*E11 + v.y*E21 + v.z*E31;
   E42 = v.x*E12 + v.y*E22 + v.z*E32;
   E43 = v.x*E13 + v.y*E23 + v.z*E33;
 }

void SrMat::right_combine_translation ( const SrVec &v )
 {
   E41 += v.x;
   E42 += v.y;
   E43 += v.z;
 }

void SrMat::combine_scale ( float sx, float sy, float sz )
 {
   E11*=sx; E12*=sy; E13*=sy;
   E21*=sx; E22*=sy; E23*=sy;
   E31*=sx; E32*=sy; E33*=sy;
 }

void SrMat::scale ( float sx, float sy, float sz )
 {
   setl1 (  sx, 0.0, 0.0, 0.0 );
   setl2 ( 0.0,  sy, 0.0, 0.0 );
   setl3 ( 0.0, 0.0,  sz, 0.0 );
   setl4 ( 0.0, 0.0, 0.0, 1.0 );
 }

void SrMat::rotx ( float sa, float ca )
 {
   setl1 ( 1.0, 0.0, 0.0, 0.0 );
   setl2 ( 0.0,  ca,  sa, 0.0 );
   setl3 ( 0.0, -sa,  ca, 0.0 );
   setl4 ( 0.0, 0.0, 0.0, 1.0 );
 }

void SrMat::roty ( float sa, float ca )
 {
   setl1 (  ca, 0.0, -sa, 0.0 );
   setl2 ( 0.0, 1.0, 0.0, 0.0 );
   setl3 (  sa, 0.0,  ca, 0.0 );
   setl4 ( 0.0, 0.0, 0.0, 1.0 );
 }

void SrMat::rotz ( float sa, float ca )
 {
   setl1 (  ca,  sa, 0.0, 0.0 );
   setl2 ( -sa,  ca, 0.0, 0.0 );
   setl3 ( 0.0, 0.0, 1.0, 0.0 );
   setl4 ( 0.0, 0.0, 0.0, 1.0 );
 }

void SrMat::rotx ( float radians )
 {
   rotx ( sinf(radians), cosf(radians) );
 }

void SrMat::roty ( float radians )
 {
   roty ( sinf(radians), cosf(radians) );
 }

void SrMat::rotz ( float radians )
 {
   rotz ( sinf(radians), cosf(radians) );
 }

void SrMat::rot ( const SrVec &vec, float sa, float ca )
 {
   double x, y, z, norm;
   double xx, yy, zz, xy, yz, zx, xs, ys, zs, oc;

   x=vec.x; y=vec.y; z=vec.z;

   norm = x*x + y*y + z*z;
   if ( norm!=1 )
    { norm = sqrt(norm);
      if (norm!=0) { x/=norm; y/=norm; z/=norm; }
    }

   // The following line was added to invert the sign of the given angle
   // in order to cope with the v*M OpenGL way, that is contrary to M*v of
   // the standard mathematical notation.
   sa*=-1.0;

   xx = x * x;   yy = y * y;   zz = z * z;
   xy = x * y;   yz = y * z;   zx = z * x;
   xs = x * sa;  ys = y * sa;  zs = z * sa;

   oc = 1.0 - ca;

   setl1 ( float(oc * xx + ca), float(oc * xy - zs), float(oc * zx + ys), 0 );
   setl2 ( float(oc * xy + zs), float(oc * yy + ca), float(oc * yz - xs), 0 );
   setl3 ( float(oc * zx - ys), float(oc * yz + xs), float(oc * zz + ca), 0 );
   setl4 (                   0,                   0,                   0, 1 );
 }

void SrMat::rot ( const SrVec &vec, float radians )
 { 
   rot ( vec, sinf(radians), cosf(radians) );
 }

void SrMat::rot ( const SrVec& from, const SrVec& to )
 {
   SrVec axis;
   axis = cross ( from, to );
   rot ( cross(from,to), angle(from,to) );
 }

void SrMat::projxy ( SrVec p1, SrVec p2, SrVec p3 )
 {
   # define PROJERR(d) { sr_out.warning ("degenerated input(%d) in SrMat::projxy()!\n",d); return; }

   SrMat m(SrMat::NotInitialized);
   float ca, sa, v;

   translation ( -p1.x, -p1.y, -p1.z ); // p1 goes to origin
   p2=p2-p1; p3=p3-p1;

   v = sqrtf(p2.y*p2.y + p2.z*p2.z); if (v==0.0) PROJERR(1);

   ca=p2.z/v; sa=p2.y/v; // rotate by x : p1p2 to xz
   m.rotx(ca,sa); p2.rotx(ca,sa); p3.rotx(ca,sa); 
   mult ( *this, m );
   
   v = sqrtf(p2.x*p2.x + p2.z*p2.z); if (v==0.0) PROJERR(2);

   ca=p2.x/v; sa=p2.z/v; // rotate by y: p1p2 to x axis
   m.roty(ca,sa); p3.roty(ca,sa); 
   mult ( *this, m );

   v = sqrtf(p3.y*p3.y + p3.z*p3.z); if (v==0.0) PROJERR(3);

   ca=p3.y/v; sa=p3.z/v; // rotate by x: p3 to xy
   m.rotx(ca,-sa);
   mult ( *this, m );

   # undef PROJERR
 }

void SrMat::perspective ( float fovy, float aspect, float znear, float zfar )
 {
   float top = znear * tanf ( fovy/2 );

   float left = -top * aspect;
   float right = top * aspect;

   float w = right-left; // width
   float h = 2.0f*top;   // height
   float d = zfar-znear; // depth

   setl1 (    (2*znear)/w,           0,                 0,  0 );
   setl2 (              0, (2*znear)/h,                 0,  0 );
   setl3 ( (right+left)/w,           0,   (zfar+znear)/-d, -1 );
   setl4 (              0,           0, (2*zfar*znear)/-d,  0 );

/*
   GLdouble ymax = (GLdouble)znear * tan ( fovy/2 ); // same as fovy

   glFrustum ( (GLdouble) ((-ymax)*(GLdouble)aspect), // xmin
               (GLdouble) (( ymax)*(GLdouble)aspect), // xmax
               (GLdouble) (-ymax),          // ymin
               (GLdouble) ymax, 
               (GLdouble) znear, 
               (GLdouble) zfar   
             );
*/
 }

void SrMat::look_at ( const SrVec& eye, const SrVec& center, const SrVec& up )
 {
   SrVec z = eye-center; z.normalize();
   SrVec x = cross ( up, z ); 
   SrVec y = cross ( z,  x ); 

   x.normalize();
   y.normalize();

   setl1 ( x.x, y.x, z.x, 0.0  );
   setl2 ( x.y, y.y, z.y, 0.0  );
   setl3 ( x.z, y.z, z.z, 0.0  );
   setl4 (   0,   0,   0, 1.0f );

   x = eye*-1.0f;
   left_combine_translation ( x );
 }

void SrMat::inverse ( SrMat &inv ) const
 {
   float d = det();
   if (d==0.0) return;
   d = 1.0f/d;

   float m12 = E21*E32 - E22*E31;
   float m13 = E21*E33 - E23*E31;
   float m14 = E21*E34 - E24*E31;
   float m23 = E22*E33 - E23*E32;
   float m24 = E22*E34 - E24*E32;
   float m34 = E23*E34 - E24*E33;
   inv.E11 = (E42*m34 - E43*m24 + E44*m23) * d;
   inv.E21 = (E43*m14 - E41*m34 - E44*m13) * d;
   inv.E31 = (E41*m24 - E42*m14 + E44*m12) * d;
   inv.E41 = (E42*m13 - E41*m23 - E43*m12) * d;
   inv.E14 = (E13*m24 - E12*m34 - E14*m23) * d;
   inv.E24 = (E11*m34 - E13*m14 + E14*m13) * d;
   inv.E34 = (E12*m14 - E11*m24 - E14*m12) * d;
   inv.E44 = (E11*m23 - E12*m13 + E13*m12) * d;

   m12 = E11*E42 - E12*E41;
   m13 = E11*E43 - E13*E41;
   m14 = E11*E44 - E14*E41;
   m23 = E12*E43 - E13*E42;
   m24 = E12*E44 - E14*E42;
   m34 = E13*E44 - E14*E43;
   inv.E12 = (E32*m34 - E33*m24 + E34*m23) * d;
   inv.E22 = (E33*m14 - E31*m34 - E34*m13) * d;
   inv.E32 = (E31*m24 - E32*m14 + E34*m12) * d;
   inv.E42 = (E32*m13 - E31*m23 - E33*m12) * d;
   inv.E13 = (E23*m24 - E22*m34 - E24*m23) * d;
   inv.E23 = (E21*m34 - E23*m14 + E24*m13) * d;
   inv.E33 = (E22*m14 - E21*m24 - E24*m12) * d;
   inv.E43 = (E21*m23 - E22*m13 + E23*m12) * d;
 }

SrMat SrMat::inverse () const 
 { 
   SrMat inv; 
   inverse ( inv ); 
   return inv; 
 }

float SrMat::det () const
 {
   float m12 = E21*E32 - E22*E31;
   float m13 = E21*E33 - E23*E31;
   float m14 = E21*E34 - E24*E31;
   float m23 = E22*E33 - E23*E32;
   float m24 = E22*E34 - E24*E32;
   float m34 = E23*E34 - E24*E33;
   float d1 = E12*m34 - E13*m24 + E14*m23;
   float d2 = E11*m34 - E13*m14 + E14*m13;
   float d3 = E11*m24 - E12*m14 + E14*m12;
   float d4 = E11*m23 - E12*m13 + E13*m12;
   return -E41*d1 + E42*d2 - E43*d3 + E44*d4;
 }

float SrMat::det3x3 () const
 {
   return   E11 * (E22*E33-E23*E32)
          + E12 * (E23*E31-E21*E33)
          + E13 * (E21*E32-E22*E31);
 }

float SrMat::norm2 () const
 {
   return E11*E11 + E12*E12 + E13*E13 + E14*E14 +
          E21*E21 + E22*E22 + E23*E23 + E24*E24 +
          E31*E31 + E32*E32 + E33*E33 + E34*E34 +
          E41*E41 + E42*E42 + E43*E43 + E44*E44;
 }

float SrMat::norm () const
 {
   return sqrtf ( norm2() );
 }

void SrMat::mult ( const SrMat &m1, const SrMat &m2 )
 {
   SrMat* m = (this==&m1||this==&m2)? new SrMat(SrMat::NotInitialized) : this;

   m->setl1 ( m1.E11*m2.E11 + m1.E12*m2.E21 + m1.E13*m2.E31 + m1.E14*m2.E41,
              m1.E11*m2.E12 + m1.E12*m2.E22 + m1.E13*m2.E32 + m1.E14*m2.E42,
              m1.E11*m2.E13 + m1.E12*m2.E23 + m1.E13*m2.E33 + m1.E14*m2.E43,
              m1.E11*m2.E14 + m1.E12*m2.E24 + m1.E13*m2.E34 + m1.E14*m2.E44 );

   m->setl2 ( m1.E21*m2.E11 + m1.E22*m2.E21 + m1.E23*m2.E31 + m1.E24*m2.E41,
              m1.E21*m2.E12 + m1.E22*m2.E22 + m1.E23*m2.E32 + m1.E24*m2.E42,
              m1.E21*m2.E13 + m1.E22*m2.E23 + m1.E23*m2.E33 + m1.E24*m2.E43,
              m1.E21*m2.E14 + m1.E22*m2.E24 + m1.E23*m2.E34 + m1.E24*m2.E44 );

   m->setl3 ( m1.E31*m2.E11 + m1.E32*m2.E21 + m1.E33*m2.E31 + m1.E34*m2.E41,
              m1.E31*m2.E12 + m1.E32*m2.E22 + m1.E33*m2.E32 + m1.E34*m2.E42,
              m1.E31*m2.E13 + m1.E32*m2.E23 + m1.E33*m2.E33 + m1.E34*m2.E43,
              m1.E31*m2.E14 + m1.E32*m2.E24 + m1.E33*m2.E34 + m1.E34*m2.E44 );

   m->setl4 ( m1.E41*m2.E11 + m1.E42*m2.E21 + m1.E43*m2.E31 + m1.E44*m2.E41,
              m1.E41*m2.E12 + m1.E42*m2.E22 + m1.E43*m2.E32 + m1.E44*m2.E42,
              m1.E41*m2.E13 + m1.E42*m2.E23 + m1.E43*m2.E33 + m1.E44*m2.E43,
              m1.E41*m2.E14 + m1.E42*m2.E24 + m1.E43*m2.E34 + m1.E44*m2.E44 );

   if ( m!=this ) { *this=*m; delete m; }
 }

void SrMat::add ( const SrMat &m1, const SrMat &m2 )
 {
   setl1 ( m1.E11+m2.E11, m1.E12+m2.E12, m1.E13+m2.E13, m1.E14+m2.E14 );
   setl2 ( m1.E21+m2.E21, m1.E22+m2.E22, m1.E23+m2.E23, m1.E24+m2.E24 );
   setl3 ( m1.E31+m2.E31, m1.E32+m2.E32, m1.E33+m2.E33, m1.E34+m2.E34 );
   setl4 ( m1.E41+m2.E41, m1.E42+m2.E42, m1.E43+m2.E43, m1.E44+m2.E44 );
 }

void SrMat::sub ( const SrMat &m1, const SrMat &m2 )
 {
   setl1 ( m1.E11-m2.E11, m1.E12-m2.E12, m1.E13-m2.E13, m1.E14-m2.E14 );
   setl2 ( m1.E21-m2.E21, m1.E22-m2.E22, m1.E23-m2.E23, m1.E24-m2.E24 );
   setl3 ( m1.E31-m2.E31, m1.E32-m2.E32, m1.E33-m2.E33, m1.E34-m2.E34 );
   setl4 ( m1.E41-m2.E41, m1.E42-m2.E42, m1.E43-m2.E43, m1.E44-m2.E44 );
 }

//================================= friends ========================================

float dist ( const SrMat &a, const SrMat &b )
 {
   return sqrtf ( dist2(a,b) );
 }

float dist2 ( const SrMat &a, const SrMat &b )
 {
   return (a-b).norm2();
 }

//================================= operators ========================================

void SrMat::operator *= ( float r )
 { 
   E11*=r; E12*=r; E13*=r; E14*=r;
   E21*=r; E22*=r; E23*=r; E24*=r;
   E31*=r; E32*=r; E33*=r; E34*=r;
   E41*=r; E42*=r; E43*=r; E44*=r;
 }

void SrMat::operator *= ( const SrMat &m )
 {
   SrMat* t = (this==&m)? new SrMat(SrMat::NotInitialized) : this;

   t->setl1 ( E11*m.E11 + E12*m.E21 + E13*m.E31 + E14*m.E41,
              E11*m.E12 + E12*m.E22 + E13*m.E32 + E14*m.E42,
              E11*m.E13 + E12*m.E23 + E13*m.E33 + E14*m.E43,
              E11*m.E14 + E12*m.E24 + E13*m.E34 + E14*m.E44 );

   t->setl2 ( E21*m.E11 + E22*m.E21 + E23*m.E31 + E24*m.E41,
              E21*m.E12 + E22*m.E22 + E23*m.E32 + E24*m.E42,
              E21*m.E13 + E22*m.E23 + E23*m.E33 + E24*m.E43,
              E21*m.E14 + E22*m.E24 + E23*m.E34 + E24*m.E44 );

   t->setl3 ( E31*m.E11 + E32*m.E21 + E33*m.E31 + E34*m.E41,
              E31*m.E12 + E32*m.E22 + E33*m.E32 + E34*m.E42,
              E31*m.E13 + E32*m.E23 + E33*m.E33 + E34*m.E43,
              E31*m.E14 + E32*m.E24 + E33*m.E34 + E34*m.E44 );

   t->setl4 ( E41*m.E11 + E42*m.E21 + E43*m.E31 + E44*m.E41,
              E41*m.E12 + E42*m.E22 + E43*m.E32 + E44*m.E42,
              E41*m.E13 + E42*m.E23 + E43*m.E33 + E44*m.E43,
              E41*m.E14 + E42*m.E24 + E43*m.E34 + E44*m.E44 );

   if ( t!=this ) { *this=*t; delete t; }

/* old non optimized version:
   SrMat mat(SrMat::NotInitialized);
   mat.mult ( *this, m );
   *this = mat;   
*/
 }

void SrMat::operator += ( const SrMat &m )
 {
   E11+=m.E11; E12+=m.E12; E13+=m.E13; E14+=m.E14;
   E21+=m.E21; E22+=m.E22; E23+=m.E23; E24+=m.E24;
   E31+=m.E31; E32+=m.E32; E33+=m.E33; E34+=m.E34;
   E41+=m.E41; E42+=m.E42; E43+=m.E43; E44+=m.E44;

 }

SrMat operator * ( const SrMat &m, float r )
 {
   return SrMat ( m.E11*r, m.E12*r, m.E13*r, m.E14*r,
                  m.E21*r, m.E22*r, m.E23*r, m.E24*r,
                  m.E31*r, m.E32*r, m.E33*r, m.E34*r,
                  m.E41*r, m.E42*r, m.E43*r, m.E44*r );
 }

SrMat operator * ( float r, const SrMat &m )
 {
   return SrMat ( m.E11*r, m.E12*r, m.E13*r, m.E14*r,
                  m.E21*r, m.E22*r, m.E23*r, m.E24*r,
                  m.E31*r, m.E32*r, m.E33*r, m.E34*r,
                  m.E41*r, m.E42*r, m.E43*r, m.E44*r );
 }

SrVec operator * ( const SrMat &m, const SrVec &v )
 {
   SrVec r ( m.E11*v.x + m.E12*v.y + m.E13*v.z + m.E14,
             m.E21*v.x + m.E22*v.y + m.E23*v.z + m.E24,
             m.E31*v.x + m.E32*v.y + m.E33*v.z + m.E34  );

   float w = m.E41*v.x + m.E42*v.y + m.E43*v.z + m.E44;
   if ( w!=0.0 && w!=1.0 ) r/=w;
   return r;
 }

SrVec operator * ( const SrVec &v, const SrMat &m )
 {
   SrVec r ( m.E11*v.x + m.E21*v.y + m.E31*v.z + m.E41,
             m.E12*v.x + m.E22*v.y + m.E32*v.z + m.E42,
             m.E13*v.x + m.E23*v.y + m.E33*v.z + m.E43  );

   float w = m.E14*v.x + m.E24*v.y + m.E34*v.z + m.E44;
   if ( w!=0.0 && w!=1.0 ) r/=w;
   return r;
 }

SrMat operator * ( const SrMat &m1, const SrMat &m2 )
 {
   SrMat mat(SrMat::NotInitialized);
   mat.mult ( m1, m2 );
   return mat;
 }

SrMat operator + ( const SrMat &m1, const SrMat &m2 )
 {
   SrMat mat(SrMat::NotInitialized);
   mat.add ( m1, m2 );
   return mat;
 }

SrMat operator - ( const SrMat &m1, const SrMat &m2 )
 {
   SrMat mat(SrMat::NotInitialized);
   mat.sub ( m1, m2 );
   return mat;
 }

bool operator == ( const SrMat &m1, const SrMat &m2 )
 {
   return  m1.E11==m2.E11 && m1.E12==m2.E12 && m1.E13==m2.E13 && m1.E14==m2.E14 &&
           m1.E21==m2.E21 && m1.E22==m2.E22 && m1.E23==m2.E23 && m1.E24==m2.E24 &&
           m1.E31==m2.E31 && m1.E32==m2.E32 && m1.E33==m2.E33 && m1.E34==m2.E34 &&
           m1.E41==m2.E41 && m1.E42==m2.E42 && m1.E43==m2.E43 && m1.E44==m2.E44 ? true:false;
 }

bool operator != ( const SrMat &m1, const SrMat &m2 )
 {
   return  m1.E11!=m2.E11 || m1.E12!=m2.E12 || m1.E13!=m2.E13 || m1.E14!=m2.E14 ||
           m1.E21!=m2.E21 || m1.E22!=m2.E22 || m1.E23!=m2.E23 || m1.E24!=m2.E24 ||
           m1.E31!=m2.E31 || m1.E32!=m2.E32 || m1.E33!=m2.E33 || m1.E34!=m2.E34 ||
           m1.E41!=m2.E41 || m1.E42!=m2.E42 || m1.E43!=m2.E43 || m1.E44!=m2.E44 ? true:false;
 }

SrOutput& operator<< ( SrOutput& o, const SrMat& m )
 { 
   o << m.E11 <<' '<< m.E12 <<' '<< m.E13 <<' '<< m.E14 << srnl;
   o << m.E21 <<' '<< m.E22 <<' '<< m.E23 <<' '<< m.E24 << srnl;
   o << m.E31 <<' '<< m.E32 <<' '<< m.E33 <<' '<< m.E34 << srnl;
   o << m.E41 <<' '<< m.E42 <<' '<< m.E43 <<' '<< m.E44 << srnl;
   return o;
 }

SrInput& operator>> ( SrInput& in, SrMat& m )
 { 
   return in >> m.E11 >> m.E12 >> m.E13 >> m.E14
             >> m.E21 >> m.E22 >> m.E23 >> m.E24
             >> m.E31 >> m.E32 >> m.E33 >> m.E34
             >> m.E41 >> m.E42 >> m.E43 >> m.E44;
 }


/* Almost included at some point:
    / The matrix keeps a flag saying if it represents a rigid transformation.
        This flag is used to speed up some operations. 
    void rigid_transformation ( bool b ) { _rigid_transf=b?1:0; }
    
    / Returns if the matrix represents a rigid transformation
    bool rigid_transformation () const { return _rigid_transf? true:false; }
*/

//================================== End of File ===========================================
