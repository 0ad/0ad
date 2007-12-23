#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_sn.h"

//# define SR_USE_TRACE1  // SrSn Const/Dest
# include "sr_trace.h"

//======================================= SrSn ====================================

SrSn::SrSn ( SrSn::Type t, const char* class_name ) 
 { 
   SR_TRACE1 ( "Constructor" );
   _ref = 0;
   _visible = 1;
   _type = t;
   _label = 0;
   _inst_class_name = class_name;
 }

SrSn::~SrSn ()
 {
   SR_TRACE1 ( "Destructor" );
   delete _label;
 }

void SrSn::label ( const char* s )
 {
   sr_string_set ( _label, s );
 }

const char* SrSn::label () const
 {
   return _label? _label:"";
 }


//======================================= EOF ====================================

