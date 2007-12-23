#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_time.h"
# include "sr_string.h"

# include "sr_output.h"

//======================= SrTime =====================================

SrTime::SrTime ()
 { 
   _h = _m = 0;
   _s = 0;
 }

void SrTime::set ( int h, int m, float s )
 { 
   _h = h;
   _m = m;
   _s = s;

   //sr_out<<h<<srspc<<m<<srspc<<s<<", ";

   if ( _s>=60.0f )
    { m = (int) (_s/60.0f);
      _s -= (float) (m*60);
      _m += m;
    }
   //sr_out<<h<<srspc<<m<<srspc<<s<<", ";

   if ( _m>=60 )
    { h = _m/60;
      _m -= h*60;
      _h += h;
    }
   //sr_out<<h<<srspc<<m<<srspc<<s<<srnl;
 }

/* parameters must be positive. */
void SrTime::set ( float h, float m, float s )
 { 
   // First convert to int,int,float :
   _h = (int)h;     // take the hours
   h -= (float)_h;  // extract hours fraction
   m = m + (h*60);  // accumulate in minutes
   _m = (int)m;     // take minutes
   m -= (float)_m;  // extract minutes fraction
   _s = s + (m*60); // accumulate in seconds

   // Then, normalize values:
   set ( _h, _m, _s );
 }

void SrTime::add ( const SrTime& t )
 {
   set ( _h+t._h, _m+t._m, _s+t._s );
 }

void SrTime::add_hours ( float h )
 {
   set ( h+(float)_h, (float)_m, _s );
 }

void SrTime::add_minutes ( float m )
 {
   set ( (float)_h, m+(float)_m, _s );
 }

void SrTime::add_seconds ( float s )
 {
   set ( _h, _m, _s+s );
 }

SrTime& SrTime::operator += ( const SrTime& t )
 {
   set ( _h+t._h, _m+t._m, _s+t._s );
   return *this;
 }

SrString& SrTime::print ( SrString& s, bool milisecs ) const
 {
   if ( !milisecs )
    s.setf ( "%02d:%02d:%02d", _h, _m, int(_s) );
   else
    { float ms = (_s - float(int(_s))) * 1000.0f;
      s.setf ( "%02d:%02d:%02d.%03d", _h, _m, int(_s), int(ms) );
    }
   return s;
 }

//============================ friends ==========================

SrOutput& operator<< ( SrOutput& o, const SrTime& t )
 {
   SrString s;
   return o << t.print(s);
 }

//============================= end of file ==========================
