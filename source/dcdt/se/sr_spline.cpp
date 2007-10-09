#include "precompiled.h"
//# include <math.h>
# include "sr_spline.h"

//============================== SrSpline ===============================

SrSpline::SrSpline ( const SrSpline& c )
 {
 }

void SrSpline::init ( int d, int k )
 {
   _pieces = k-1;
   _dim = d;
   if ( _pieces<0 ) _pieces=0;
   if ( _dim<0 ) _dim=0;
   _spline.size ( _pieces*(_dim*3) + _dim );
   _spline.setall ( 0.0f );
 }
 

//============================== end of file ===============================

