#include "precompiled.h"
#include "0ad_warning_disable.h"
# include <math.h>

# include "sr_sn_manipulator.h"

# include "sr_quat.h"
# include "sr_box.h"
# include "sr_lines.h"
# include "sr_sa_bbox.h"

//# define SR_USE_TRACE1 // events
# include "sr_trace.h"

//============================ SrSnManipulator ====================================

const char* SrSnManipulator::class_name = "Manipulator";

SrSnManipulator::SrSnManipulator ()
             :SrSnEditor ( SrSnManipulator::class_name )
 {
   _box = new SrSnBox;
   _box->render_mode ( srRenderModeLines );
   _box->can_override_render_mode ( false );
   _box->visible ( false );

   _dragsel = new SrSnLines;
   _dragsel->color ( SrColor::cyan );

   helpers()->add ( _box );
   helpers()->add ( _dragsel );

   _precision_in_pixels = 6;

   _rx = _ry = _rz = 0;

   _user_cb = 0;
   _user_cb_data = 0;
   
   _translationray = false;

   init ();
 }

SrSnManipulator::~SrSnManipulator ()
 {
 }

void SrSnManipulator::child ( SrSn *sn )
 {
   SrSnEditor::child ( sn );
   update ();
 }

void SrSnManipulator::initial_mat ( const SrMat& m )
 {
   SrSnEditor::mat ( m );
   _translation = SrVec::null;
   _rotation = SrQuat::null;
   _initmat = m;
   _mode=ModeWaiting;
   _firstp = SrPnt::null;
   _transform ( SrPnt::null, SrVec::null );
 }

void SrSnManipulator::init ()
 {
   _dragsel->shape().init();
   _mode = ModeWaiting;
   _corner = -1;

   if ( SrSnEditor::child() )
    { _box->visible ( true );
    }
   else
    { _box->visible ( false );
    }
 }

void SrSnManipulator::update ()
 {
   SrSaBBox bbox_action;
   bbox_action.apply ( SrSnEditor::child() );
   _box->shape() = bbox_action.get();
   init ();
 }

void SrSnManipulator::translation ( const SrVec& t )
 {
   _translation = t;

   _mode=ModeWaiting;
   _firstp = SrPnt::null;
   _transform ( SrPnt::null, SrVec::null );
 }

void SrSnManipulator::_transform ( const SrPnt& p, const SrVec& r )
 {
   SrVec dt;

   if ( _mode==ModeRotating )
    { }
   else
    { dt = p-_firstp; }

   SrMat R;
   SrQuat q;
   if ( r.x ) q.set ( SrVec::i, r.x );
   if ( r.y ) q.set ( SrVec::j, r.y );
   if ( r.z ) q.set ( SrVec::k, r.z );
   _rotation = _rotation * q;

   _rotation.get_mat ( R );
   _translation += dt * R;

   R[12] = _translation.x;
   R[13] = _translation.y;
   R[14] = _translation.z;

   SrSnEditor::mat ( R * _initmat );
 }

void SrSnManipulator::_set_drag_selection ( const SrPnt& p )
 {
   float factor = 8;

   if ( _mode==ModeRotating ) factor/=2.0f;

   SrLines& l = _dragsel->shape();
   l.init();

   if ( _mode==ModeRotating )
    { l.push_line ( _center, p );
    }
   else
    { SrVec v1 = _bside[1]-_bside[0];
      SrVec v2 = _bside[3]-_bside[0];
      v1 /= 2;
      v2 /= 2;
      if ( _translationray )
       { float l1 = v1.len()/4;
         float l2 = v2.len()/4;
         if ( l1>l2 ) l1=l2;
         v1.len(l1); v2.len(l1);
         l.push_circle_approximation ( p, v1, cross(v1,v2), 12 );
       }
      else
       { l.push_line ( p-v1, p+v1 );
         l.push_line ( p-v2, p+v2 );
       }
    }
 }

int SrSnManipulator::handle_event ( const SrEvent &e )
 {
   if ( _mode!=ModeWaiting && e.type==SrEvent::Release )
    { SR_TRACE1 ( "RELEASE" );
      init ();
      if ( _user_cb ) _user_cb ( this, e, _user_cb_data );
      return 1;
    }

   if ( _mode==ModeWaiting && e.type==SrEvent::Push )
    { SR_TRACE1 ( "PUSH" );
      float t1, t2;
      int k = e.ray.intersects_box ( _box->const_shape(), t1, t2, _bside );
      if ( k>0 )
       { _firstp = SR_LERP(e.ray.p1,e.ray.p2,t1);
         _plane.set ( _bside[0], _bside[1], _bside[2] );
         _mode = ModeTranslating;
         _set_drag_selection ( _firstp );
         if ( _user_cb ) _user_cb ( this, e, _user_cb_data );
         return 1;
       }
    }

   if ( _mode==ModeTranslating && e.type==SrEvent::Drag )
    { SR_TRACE1 ( "TRANSLATING" );
      SrPnt p;
      if ( _translationray )
       p = e.ray.closestpt ( _firstp );
      else
       p = _plane.intersect ( e.ray.p1, e.ray.p2 );
      _transform ( p, SrVec::null );
      _set_drag_selection ( p );
      if ( _user_cb ) _user_cb ( this, e, _user_cb_data );
      return 1;
    }

   if ( _mode==ModeTranslating && e.type==SrEvent::Keyboard )
    { SR_TRACE1 ( "ROTATING" );
      if ( e.key=='p' ) sr_out<<mat()<<srnl;
      if ( e.key=='x' ) 
       { SR_SWAPB(_translationray);
         _set_drag_selection ( _firstp );
       }
      float da = SR_TORAD(5.0f);
      if ( e.shift ) da = SR_TORAD(1.0f);
       else if ( e.alt ) da = SR_TORAD(0.1f);
      SrVec r;
      switch ( e.key )
       { case 'q' : r.x+=da; break;
         case 'a' : r.x-=da; break;
         case 'w' : r.y+=da; break;
         case 's' : r.y-=da; break;
         case 'e' : r.z+=da; break;
         case 'd' : r.z-=da; break;
         case SrEvent::KeyEsc: _rotation=SrQuat::null; _translation=SrVec::null; break;
       }
      _mode = ModeRotating;
      _transform ( _firstp, r );
      _mode = ModeTranslating;
      if ( _user_cb ) _user_cb ( this, e, _user_cb_data );
      return 1;
    }

/*   if ( _mode==ModeRotating && e.type==SrEvent::Drag )
    { SR_TRACE1 ( "ROTATING" );
      SrPnt ip[2];
      int i = e.ray.intersects_sphere ( _center, _radius, ip );
      if ( i>0 )
       { transform ( ip[0] );
         set_drag_selection ( _bside[_corner] );
       }
      return 1;
    }
*/
   _mode = ModeWaiting;

   return 0; // event not used
 }

//================================ End of File =================================================

