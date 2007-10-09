#include "precompiled.h"
# include "sr_sa_model_export.h"
# include "sr_model.h"
# include "sr_sn_shape.h"

//# define SR_USE_TRACE1 // Const / Dest 
//# define SR_USE_TRACE2 // 
# include "sr_trace.h"

//=============================== SrSaModelExport ====================================

SrSaModelExport::SrSaModelExport ( const SrString& dir )
 { 
   SR_TRACE1 ( "Constructor" );
   directory ( dir );
   _num = 0;
   _prefix = "model";
 }

SrSaModelExport::~SrSaModelExport ()
 {
   SR_TRACE1 ( "Destructor" );
 }

void SrSaModelExport::directory ( const SrString& dir )
 {
   _dir = dir;
   char c = _dir.last_char();
   if ( c==0 || c=='/' || c=='\\' ) return;
   _dir << '/';
 }

bool SrSaModelExport::apply ( SrSn* n )
 {
   _num = 0;
   bool result = SrSa::apply ( n );
   return result;
 }

//==================================== virtuals ====================================

bool SrSaModelExport::shape_apply ( SrSnShapeBase* s )
 {
   if ( sr_compare(s->inst_class_name(),"model")!=0 ) return true;
   if ( !s->visible() ) return true;

   SrMat mat;
   get_top_matrix ( mat );

   SrModel model;
   model = ((SrSnModel*)s)->shape();
   model.apply_transformation ( mat );

   SrString fname;
   fname.setf ( "%s%s%04d.srm", (const char*)_dir,(const char*)_prefix, _num++ );
   SrOutput out ( fopen(fname,"wt") );
   if ( !out.valid() ) return false;

   model.save ( out );

   return true;
 }

//======================================= EOF ====================================

