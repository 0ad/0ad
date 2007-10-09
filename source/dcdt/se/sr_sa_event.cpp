#include "precompiled.h"
//# include <stdlib.h>

# include "sr_sn_editor.h"
# include "sr_sa_event.h"

//# define SR_USE_TRACE1 // constructor / destructor
# include "sr_trace.h"

//================================== SrSaEvent ====================================

bool SrSaEvent::editor_apply ( SrSnEditor* m )
 {
   bool b=true;
   if ( !m->visible() ) return b;

   push_matrix ();
   mult_matrix ( m->mat() );

   SrMat mat = _matrix_stack.top();
   mat.invert();
//sr_out<<mat<<srnl;
//sr_out << "pixel_size AA: " << _ev.pixel_size <<srnl;

   SrEvent e = _ev;
//sr_out << "pixel_size BB: " << e.pixel_size <<srnl;
   e.ray.p1 = e.ray.p1*mat;
   e.ray.p2 = e.ray.p2*mat;
   e.lray.p1 = e.lray.p1*mat;
   e.lray.p2 = e.lray.p2*mat;
   e.mousep = e.mousep*mat;
   e.lmousep = e.lmousep*mat;

   _result = m->handle_event ( e );
   if ( _result ) b = false; // event used: stop the scene traversing

   pop_matrix ();
   return b;
 }

//======================================= EOF ====================================
