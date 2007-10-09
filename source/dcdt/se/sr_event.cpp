#include "precompiled.h" 
# include "sr_event.h"

//# define SR_USE_TRACE1  
# include "sr_trace.h"

//===================================== SrEvent =================================

SrEvent::SrEvent ()
 {
   init ();
 }

void SrEvent::init ()
 {
   type = None;
   key = 0;
   button = 0;
   button1 = button2 = button3 = 0;
   ctrl = shift = alt = key = 0;
   width = heigth = 0;
   pixel_size = 0.05f;
 }

void SrEvent::init_lmouse ()
 {
   type = None;
   key = 0;
   button = 0;
   button1 = button2 = button3 = 0;
   ctrl = shift = alt = key = 0;
   lmouse = mouse;
   lmouse = mouse;
   mouse.x = mouse.y = 0;
 }

const char *SrEvent::type_name () const
 {
   switch ( type )
    { case None : return "none";
      case Push : return "push";
      case Drag : return "drag";
      case Release : return "release";
      case Keyboard : return "keyboard";
    }
   return "undefined?!";
 }

SrOutput& operator<< ( SrOutput& out, const SrEvent& e )
 {
   out << e.type_name();

   if ( e.type==SrEvent::Keyboard ) 
    out << " [" << (e.key? e.key:' ') << ':' << (int)e.key << ']';

   out << " POS:" << e.mouse <<
          " BUTS:" << (int)e.button1<<(int)e.button2<<(int)e.button3<<
          " ACS:" << (int)e.alt<<(int)e.ctrl<<(int)e.shift;

   return out;
 }

