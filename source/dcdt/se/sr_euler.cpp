#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_euler.h"
# include "sr_mat.h"
# include <math.h>

//# define SR_USE_TRACE1
# include "sr_trace.h"

# define ISZERO(a) ( (a)>-(srtiny) && (a)<(srtiny) )

# define EQUAL(a,b) ( ( (a)>(b)? ((a)-(b)):((b)-(a)) )<=(srtiny) )

# define GETSINCOS double cx=cos(rx); double cy=cos(ry); double cz=cos(rz); \
                   double sx=sin(rx); double sy=sin(ry); double sz=sin(rz)

# define ATAN2(x,y) (float) atan2 ( (double)x, (double)y )
# define NORM(x,y) sqrt(double(x)*double(x)+double(y)*double(y))

//============================ Get Angles ================================

void sr_euler_angles ( int order, const SrMat& m, float& rx, float& ry, float& rz )
 {
   switch ( order )
    { case 123: sr_euler_angles_xyz(m,rx,ry,rz); break;
      case 132: sr_euler_angles_xzy(m,rx,ry,rz); break;
      case 213: sr_euler_angles_yxz(m,rx,ry,rz); break;
      case 231: sr_euler_angles_yzx(m,rx,ry,rz); break;
      case 312: sr_euler_angles_zxy(m,rx,ry,rz); break;
      case 321: sr_euler_angles_zyx(m,rx,ry,rz); break;
      default: rx=ry=rz=0;
    }
 }

void sr_euler_angles_xyz ( const SrMat& m, float& rx, float& ry, float& rz )
 {
   ry = ATAN2 ( -m[2], NORM(m[0],m[1]) );
   rx = ATAN2 ( m[6], m[10] );
   rz = ATAN2 ( m[1], m[0] );
 }
 
void sr_euler_angles_xzy ( const SrMat& m, float& rx, float& ry, float& rz )
 {
   rx = ATAN2 ( -m[9], m[5] );
   ry = ATAN2 ( -m[2], m[0] );
   rz = ATAN2 ( m[1], NORM(m[5],m[9]) );
 }

void sr_euler_angles_yxz ( const SrMat& m, float& rx, float& ry, float& rz )
 {
   rx = ATAN2 ( m[6], NORM(m[2],m[10]) );
   ry = ATAN2 ( -m[2], m[10] );
   rz = ATAN2 ( -m[4], m[5] );
 }

void sr_euler_angles_yzx ( const SrMat& m, float& rx, float& ry, float& rz )
 {
   rz = ATAN2 ( -m[4], NORM(m[0],m[8]) );
   rx = ATAN2 (  m[6], m[5] );
   ry = ATAN2 (  m[8], m[0] );
 }

void sr_euler_angles_zxy ( const SrMat& m, float& rx, float& ry, float& rz )
 {
   rx = ATAN2 ( -m[9], NORM(m[1],m[5]) );
   rz = ATAN2 ( m[1], m[5] );
   ry = ATAN2 ( m[8], m[10] );
 }

void sr_euler_angles_zyx ( const SrMat& m, float& rx, float& ry, float& rz )
 {
   ry = ATAN2 ( m[8], NORM(m[0],m[4]) );
   rx = ATAN2 ( -m[9], m[10] );
   rz = ATAN2 ( -m[4], m[0] );
 }

//============================ Get Mat ================================

void sr_euler_mat ( int order, SrMat& m, float rx, float ry, float rz )
 {
   switch ( order )
    { case 123: sr_euler_mat_xyz(m,rx,ry,rz); break;
      case 132: sr_euler_mat_xzy(m,rx,ry,rz); break;
      case 213: sr_euler_mat_yxz(m,rx,ry,rz); break;
      case 231: sr_euler_mat_yzx(m,rx,ry,rz); break;
      case 312: sr_euler_mat_zxy(m,rx,ry,rz); break;
      case 321: sr_euler_mat_zyx(m,rx,ry,rz); break;
      default: m.identity();
    }
 }

void sr_euler_mat_xyz ( SrMat& m, float rx, float ry, float rz )
 {
   GETSINCOS;
   // the following is the same as: R=Rx*Ry*Rz (in column-major format)
   m[0]=float(cy*cz);           m[1]=float(cy*sz);           m[2]=float(-sy);
   m[4]=float(-cx*sz+sx*sy*cz); m[5]=float(cx*cz+sx*sy*sz);  m[6]=float(sx*cy);
   m[8]=float(sx*sz+cx*sy*cz);  m[9]=float(-sx*cz+cx*sy*sz); m[10]=float(cx*cy);
 }

void sr_euler_mat_xzy ( SrMat& m, float rx, float ry, float rz )
 {
   GETSINCOS;
   // the following is the same as: R=Rx*Rz*Ry (in column-major format)
   m[0]=float(cy*cz);           m[1]=float(sz);     m[2]=float(-cz*sy);
   m[4]=float(-cx*sz*cy+sx*sy); m[5]=float(cx*cz);  m[6]=float(cx*sy*sz+sx*cy);
   m[8]=float(sx*sz*cy+cx*sy);  m[9]=float(-sx*cz); m[10]=float(-sx*sy*sz+cx*cy);
 }

void sr_euler_mat_yxz ( SrMat& m, float rx, float ry, float rz )
 {
   GETSINCOS;
   // the following is the same as: R=Ry*Rx*Rz (in column-major format)
   m[0]=float(cy*cz-sx*sy*sz); m[1]=float(cy*sz+sx*sy*cz); m[2]=float(-cx*sy);
   m[4]=float(-cx*sz);         m[5]=float(cx*cz);          m[6]=float(sx);
   m[8]=float(cz*sy+sx*cy*sz); m[9]=float(sy*sz-sx*cy*cz); m[10]=float(cx*cy);
 }

void sr_euler_mat_yzx ( SrMat& m, float rx, float ry, float rz )
 {
   GETSINCOS;
   // the following is the same as: R=Ry*Rz*Rx (in column-major format)
   m[0]=float(cy*cz); m[1]=float(cx*cy*sz+sx*sy); m[2]=float(cy*sz*sx-cx*sy);
   m[4]=float(-sz);   m[5]=float(cx*cz);          m[6]=float(cz*sx);
   m[8]=float(sy*cz); m[9]=float(cx*sy*sz-cy*sx); m[10]=float(sx*sy*sz+cx*cy);
 }

void sr_euler_mat_zxy ( SrMat& m, float rx, float ry, float rz )
 {
   GETSINCOS;
   // the following is the same as: R=Rz*Rx*Ry (in column-major format)
   m[0]=float(cy*cz+sx*sy*sz);  m[1]=float(cx*sz); m[2]=float(-sy*cz+sx*cy*sz);
   m[4]=float(-sz*cy+sx*sy*cz); m[5]=float(cx*cz); m[6]=float(sy*sz+sx*cy*cz);
   m[8]=float(cx*sy);           m[9]=float(-sx);   m[10]=float(cx*cy);
 }

void sr_euler_mat_zyx ( SrMat& m, float rx, float ry, float rz )
 {
   GETSINCOS;
   // the following is the same as: R=Rz*Ry*Rx (in column-major format)
   m[0]=float(cy*cz);  m[1]=float(cx*sz+cz*sy*sx); m[2]=float(sx*sz-cx*cz*sy);
   m[4]=float(-sz*cy); m[5]=float(cx*cz-sx*sy*sz); m[6]=float(sx*cz+cx*sy*sz);
   m[8]=float(sy);     m[9]=float(-sx*cy);         m[10]=float(cx*cy);
 }

//================================ EOF ===================================

/* Math:
      |1   0   0|    |cy  0 -sy|    | cz  sz  0|
   Rx=|0  cx  sx| Ry=| 0  1   0| Rz=|-sz  cz  0|
      |0 -sx  cx|    |sy  0  cy|    |  0   0  1|

          |  cy   0  -sy|         |cycz cysz -sy|         | cycz sz -czsy|
   RxRy = |sxsy  cx sxcy|  RyRz = | -sz   cz   0|  RzRy = |-szcy cz  sysz|
          |cxsy -sx cxcy|         |sycz sysz  cy|         |   sy  0    cy|

         |    cycz         cysz      -sy   |   | m0 m1 m2 |
RxRyRz = |-cxsz+sxsycz  cxcz+sxsysz  sx*cy | = | m4 m5 m6 |
         | sxsz+cxsycz -sxcz+cxsysz  cx*cy |   | m8 m9 m10|
X: sx/cx = m6/m10
Z: sz/cz = m1/m0
Y: cy^2*cz^2 + cy^2*sz^2 = m0^2+m1^2 => cy = norm(m0,m1) => sy/cy = -m2/norm(m0,m1)

         |      cycz       sz      -czsy     |   | m0 m1 m2 |
RxRzRy = | -cxszcy+sxsy   cxcz   cxsysz+sxcy | = | m4 m5 m6 |
         |  sxszcy+cxsy  -sxcz  -sxsysz+cxcy |   | m8 m9 m10|
X: -sx/cx = m9/m5
Y: -sy/cy = m2/m0
Z: cx^2*cz^2 + cz^2*sx^2 = m5^2+m9^2 => cz = norm(m5,m9) => sz/cz = m1/norm(m5,m9)

         |  cycz+sxsysz  cxsz  -sycz+sxcysz |   | m0 m1 m2 |
RzRxRy = | -szcy+sxsycz  cxcz   sysz+sxcycz | = | m4 m5 m6 |
         |     cxsy      -sx       cxcy     |   | m8 m9 m10|
Z: sz/cz = m1/m5
Y: sy/cy = m8/m10
X: cx^2*sz^2 + cx^2*cz^2 = m1^2+m5^2 => cx = norm(m1,m5) => sx/cx = -m9/norm(m1,m5)

         |  cycz  cxsz+czsysx sxsz-cxczsy |   | m0 m1 m2 |
RzRyRx = | -szcy  cxcz-sxsysz sxcz+cxsysz | = | m4 m5 m6 |
         |   sy     -sxcy        cxcy     |   | m8 m9 m10|
X: -sx/cx = m9/m10
Z: -sz/cz = m4/m0
Y: cy^2*cz^2 + sz^2*cy^2 = m0^2+m4^2 => cy = norm(m0,m4) => sy/cy = m8/norm(m0,m4)

(Euler is fixed axis, the "inver order" becomes moving axis)
*/

//================================ EOF ===================================

