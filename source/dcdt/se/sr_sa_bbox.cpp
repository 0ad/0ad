#include "precompiled.h"
//# include <stdlib.h>

# include "sr_sa_bbox.h"
# include "sr_sn_shape.h"

//# define SR_USE_TRACE1 // constructor / destructor
# include "sr_trace.h"

//================================== SrSaBBox ====================================

bool SrSaBBox::shape_apply ( SrSnShapeBase* s )
 {
   if ( !s->visible() ) return true;
   SrBox b;
   SrMat m;
   s->get_bounding_box ( b );
   get_top_matrix ( m );
   _box.extend ( b * m );
   return true;
 }
/*
bool SrSaBBox::manipulator_apply ( SrSceneShapeBase* s )
 {
   SrBox b;
   s->get_bounding_box ( b );
   _box.extend ( b * top_matrix() );
   return true;
 }
*/

//======================================= EOF ====================================
