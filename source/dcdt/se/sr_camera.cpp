#include "precompiled.h" 
#include "0ad_warning_disable.h"
# include <math.h>
# include "sr_box.h"
# include "sr_plane.h"
# include "sr_camera.h"

//# define SR_USE_TRACE1  // ray
# include "sr_trace.h"

//=================================== SrCamera ===================================

SrCamera::SrCamera ()
 {
   init ();
 }

SrCamera::SrCamera ( const SrCamera &c )
         :eye(c.eye), center(c.center), up(c.up)
 {
   fovy  = c.fovy;
   znear = c.znear;
   zfar  = c.zfar;
   aspect = c.aspect;
   scale = c.scale;
 }

SrCamera::SrCamera ( const SrPnt& e, const SrPnt& c, const SrVec& u )
         : eye(e), center(c), up(u)
 {
   fovy  = SR_TORAD(60);
   znear = 0.1f; 
   zfar  = 1000.0f; 
   aspect = 1.0f;
 }

void SrCamera::init () 
 { 
   eye.set ( 0, 0, 2.0f ); 
   center = SrVec::null;
   up = SrVec::j;
   fovy  = SR_TORAD(60);
   znear = 0.1f; 
   zfar  = 1000.0f; 
   aspect = 1.0f;
   scale = 1.0f;
 }

SrMat& SrCamera::get_view_mat ( SrMat &m ) const
 {
   m.look_at ( eye, center, up );
   return m;
 }

SrMat& SrCamera::get_perspective_mat ( SrMat &m ) const
 {
   m.perspective ( fovy, aspect, znear, zfar );
   return m;
 }

// screenpt coords range in [-1,1]
void SrCamera::get_ray ( float winx, float winy, SrVec &p1, SrVec &p2 ) const
 {
   p1.set ( winx, winy, znear ); // p1 is in the near clip plane

   SrMat M(SrMat::NotInitialized), V(SrMat::NotInitialized), P(SrMat::NotInitialized);

   V.look_at ( eye, center, up );
   P.perspective ( fovy, aspect, znear, zfar );

   M.mult ( V, P ); // equiv to M = V * P

   M.invert();

   p1 = p1 * M; 
   p2 = p1-eye; // ray is in object coordinates, but before the scaling

   p2.normalize();
   p2 *= (zfar-znear);
   p2 += p1;

   float inv_scale = 1.0f/scale;
   p1*= inv_scale;
   p2*= inv_scale;

   SR_TRACE1 ( "Ray: "<< p1 <<" : "<< p2 );
 }

/* - --------            \
   | |      |             \
   h | bbox |--------------.eye
   | |      |   dist      /
   - --------            /    tan(viewang/2)=(h/2)/dist
*/
void SrCamera::view_all ( const SrBox &box, float fovy_radians )
 {
   SrVec size = box.size();
   float h = SR_MAX(size.x,size.y);

   fovy = fovy_radians;
   up = SrVec::j;
   center = box.center();
   eye = center;
  
   float dist = (h/2)/tanf(fovy/2);
   eye.z = box.b.z + dist;

   float delta = box.max_size() + 0.0001f;
   zfar = SR_ABS(eye.z)+delta;

   scale = 1.0f;
 }

void SrCamera::apply_translation_from_mouse_motion ( float lwinx, float lwiny, float winx, float winy )
 {
   SrVec p1, p2, x, inc;

   SrPlane plane ( center, eye-center );

   get_ray ( lwinx, lwiny, p1, x );
   p1 = plane.intersect ( p1, x );
   get_ray ( winx, winy, p2, x );
   p2 = plane.intersect ( p2, x );

   inc = p1-p2;

   inc *= scale;

   *this += inc;
 }

void SrCamera::operator*= ( const SrQuat& q )
 {
   eye -= center;
   eye = eye * q;
   eye += center;
   up -= center;
   up = up * q;
   up += center;
 }

void SrCamera::operator+= ( const SrVec& v )
 {
   eye += v;
   center += v;
 }

void SrCamera::operator-= ( const SrVec& v )
 {
   eye -= v;
   center -= v;
 }

//=============================== friends ==========================================

SrCamera operator* ( const SrCamera& c, const SrQuat& q )
 {
   SrCamera cam(c);
   cam *= q;
   return cam;
 }

SrCamera operator+ ( const SrCamera& c, const SrVec& v )
 {
   SrCamera cam(c);
   cam += v;
   return cam;
 }

SrOutput& operator<< ( SrOutput& out, const SrCamera& c )
 {
//   out << "eye:" << c.eye << " center:" << c.center << " up:" << c.up << srnl;

   out << "eye    " << c.eye << srnl <<
          "center " << c.center << srnl <<
          "up     " << c.up << srnl <<
          "fovy   " << c.fovy << srnl <<
          "znear  " << c.znear << srnl <<
          "zfar   " << c.zfar << srnl <<
          "aspect " << c.aspect << srnl <<
          "scale  " << c.scale << srnl;

   return out;
 }

SrInput& operator>> ( SrInput& inp, SrCamera& c )
 {
   while ( 1 )
    { if ( inp.get_token()==SrInput::EndOfFile ) break;
      if ( inp.last_token()=="eye" ) inp>>c.eye;
      else if ( inp.last_token()=="center" ) inp>>c.center;
      else if ( inp.last_token()=="up" ) inp>>c.up;
      else if ( inp.last_token()=="fovy" ) inp>>c.fovy;
      else if ( inp.last_token()=="znear" ) inp>>c.znear;
      else if ( inp.last_token()=="zfar" ) inp>>c.zfar;
      else if ( inp.last_token()=="aspect" ) inp>>c.aspect;
      else if ( inp.last_token()=="scale" ) inp>>c.scale;
      else { inp.unget_token(); break; }
    }
   return inp;
 }

//================================ End of File =========================================
