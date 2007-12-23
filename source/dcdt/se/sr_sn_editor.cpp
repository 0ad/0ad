#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_sn_editor.h"

//# define SR_USE_TRACE1  // SrSn Const/Dest
//# define SR_USE_TRACE2  // SrSceneGroup Const/Dest
//# define SR_USE_TRACE3  // SrSceneGroup children management
//# define SR_USE_TRACE4  // SrSceneMatrix Const/Dest
//# define SR_USE_TRACE5  // SrSceneShapeBase Const/Dest
# include "sr_trace.h"


//======================================= SrSnEditor ====================================

SrSnEditor::SrSnEditor ( const char* name )
                       :SrSn ( SrSn::TypeEditor, name )
 {
   SR_TRACE2 ( "Constructor" );
   _child = 0;
   _helpers = new SrSnGroup;
   _helpers->ref ();
 }

SrSnEditor::~SrSnEditor ()
 {
   SR_TRACE2 ( "Destructor" );
   _helpers->unref ();
   child ( 0 );
 }

// protected :

void SrSnEditor::child ( SrSn *sn )
 {
   if ( _child ) _child->unref();
   if ( sn ) sn->ref(); // Increment reference counter
   _child = sn;
 }

int SrSnEditor::handle_event ( const SrEvent &e )
 {
   return 0;
 }

//======================================= EOF ====================================

