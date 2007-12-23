#include "precompiled.h"
#include "0ad_warning_disable.h"
//# include <stdlib.h>

# include "sr_sa_render_mode.h"

# include "sr_sn_shape.h"

//# define SR_USE_TRACE1 // constructor / destructor
# include "sr_trace.h"

//=============================== SrSaRenderMode ====================================

bool SrSaRenderMode::shape_apply ( SrSnShapeBase* s )
 {
   if ( _override )
     s->override_render_mode ( _render_mode );
   else
     s->restore_render_mode ();

   return true;
 }

//======================================= EOF ====================================
