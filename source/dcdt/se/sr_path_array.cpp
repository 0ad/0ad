#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_path_array.h"

//# define SR_USE_TRACE1
# include "sr_trace.h"

//====================== SrPathArray ==========================

void SrPathArray::basedir_from_filename ( const char* file )
 {
   _basedir = file;
   _basedir.remove_file_name();
   _basedir.make_valid_path();
 }

bool SrPathArray::adjust_path ( SrString& file )
 {
   SrInput in;
   if ( open(in,file) )
    { file = in.filename();
      return true;
    }
   else
    { return false;
    }
 }

//=========================== EOF ===============================
