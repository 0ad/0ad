#include "precompiled.h"
# include "sr_cfg_manager.h"

//=========================== SrCfgManagerBase ========================================

// ps: could try each "level check" in a new thread
bool SrCfgManagerBase::visible ( const srcfg* ct0, const srcfg* ct1, srcfg* ct, float prec )
 {
   float segs, dseg, dt, t;
   float len = dist ( ct0, ct1 );
   int k = 1; // k is the current level being tested

   while ( true )
    { // test the 2^(k-1) tests of level k:
      segs = (float) sr_pow ( 2, k );
      dseg = 1.0f / segs;

      dt = dseg*2.0f;
      
      //sr_out<<"level="<<k<<srnl;
      //sr_out<<"segs="<<segs<<srnl;

      for ( t=dseg; t<1.0f; t+=dt )
       { interp ( ct0, ct1, t, ct );
         //sr_out<<"t="<<t<<srnl; 
         if ( !valid(ct) ) return false;
       }
      //sr_out<<"len/segs="<<(len/segs)<<" prec="<<prec<<srnl;
      if ( len/segs<=prec ) return true; // safe edge up to prec
      k++; // increment level
    }
 }

//============================= End of File ===========================================
