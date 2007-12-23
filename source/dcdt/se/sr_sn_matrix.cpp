#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_sn_matrix.h"

//# define SR_USE_TRACE1  // Const/Dest
# include "sr_trace.h"

//======================================= SrSnMatrix ====================================

const char* SrSnMatrix::class_name = "Matrix";

SrSnMatrix::SrSnMatrix ()
           :SrSn ( SrSn::TypeMatrix, SrSnMatrix::class_name )
 {
   SR_TRACE1 ( "Constructor" );
 }

SrSnMatrix::SrSnMatrix ( const SrMat& m )
           :SrSn ( SrSn::TypeMatrix, SrSnMatrix::class_name )
 {
   SR_TRACE1 ( "Constructor from SrMat" );
   _mat = m;
 }

SrSnMatrix::~SrSnMatrix ()
 {
   SR_TRACE1 ( "Destructor" );
 }


//======================================= EOF ====================================

