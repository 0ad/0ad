#include "precompiled.h"
# include <math.h>
 
# include "sr_trackball.h"
# include "sr_mat.h"

//# define SR_USE_TRACE1  
# include "sr_trace.h"

//=================================== SrTrackball ===================================

SrTrackball::SrTrackball ()
 {
   init ();
 }

SrTrackball::SrTrackball ( const SrTrackball& t )
            :rotation(t.rotation), last_spin(t.last_spin)
 {
 }

void SrTrackball::init () 
 { 
   rotation = SrQuat::null;
 }

SrMat& SrTrackball::get_mat ( SrMat &m ) const
 {
   rotation.get_mat ( m );
   return m;
 }

/* Project an x,y pair onto a sphere of radius r OR a hyperbolic sheet
   if we are away from the center of the sphere. */
static float tb_project_to_sphere ( float r, float x, float y )
 {
   float d, t, z;
   d = sqrtf(x*x + y*y);
   if ( d < r*srsqrt2/2.0f ) z=sqrtf(r*r-d*d); // Inside sphere 
    else { t=r/srsqrt2; z=t*t/d; } // On hyperbola
   return z;
 }

void SrTrackball::get_spin_from_mouse_motion ( float p1x, float p1y, float p2x, float p2y, SrQuat& spin )
 {
    SrVec p1, p2, d, a; // a==Axes of rotation
    const float size=0.8f;
    float t;
    if ( p1x==p2x && p1y==p2y ) { spin=SrQuat::null; return; }
    // figure out z-coordinates for projection of P1 and P2 to deformed sphere

    p1.set ( p1x, p1y, tb_project_to_sphere(size,p1x,p1y) );
    p2.set ( p2x, p2y, tb_project_to_sphere(size,p2x,p2y) );
    a = cross ( p1, p2 );             // axis

    d = p1-p2;                        // how much to rotate around that axis.
    t = d.norm() / (2.0f*size);
    t = SR_BOUND ( t, -1.0f, 1.0f );  // avoid problems with out-of-control values...
    float ang = 2.0f * asinf(t);

    spin.set ( a, ang );      // a is normalized inside rot() :
 }

void SrTrackball::increment_from_mouse_motion ( float lwinx, float lwiny, float winx, float winy )
 {
   get_spin_from_mouse_motion ( lwinx, lwiny, winx, winy, last_spin );
   rotation = last_spin * rotation;
 }

void SrTrackball::increment_rotation ( const SrQuat& spin )
 {
   last_spin = spin;
   rotation = last_spin * rotation;
 }

//=============================== friends ==========================================

SrOutput& operator<< ( SrOutput& out, const SrTrackball& tb )
 {
   out << "rotat: " << tb.rotation << srnl <<
          "spin: " << tb.last_spin << srnl;

   return out;
 }

//================================ End of File =========================================
