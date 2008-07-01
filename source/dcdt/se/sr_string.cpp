#include "precompiled.h" 
#include "0ad_warning_disable.h"
# include <ctype.h>
# include <string.h>
# include <stdlib.h>
# include <stdarg.h> 
# include <stdio.h>

# include "sr_string.h"
# include "sr_input.h"

//# define SR_USE_TRACE1 // constructors / destructors
//# define SR_USE_TRACE2 // member functions
//# define SR_USE_TRACE3 // set() member function
# include "sr_trace.h"

char *SrString::_empty = (char*)"";

//============================= SrString ==========================================

SrString::SrString () : _capacity(0), _data(SrString::_empty)
 {
   SR_TRACE1 ( "Default Constructor" );
 }

SrString::SrString ( char c, int len ) : _capacity(0), _data(SrString::_empty)
 {
   if ( len<=0 ) return;
   _capacity = len+1;
   _data = new char[_capacity];
   memset ( _data, c, len );
   _data[len] = 0;
   SR_TRACE1 ( "Char Constructor: "<<c );
 }

SrString::SrString ( const char *fmt, ... ) : _capacity(0), _data(SrString::_empty)
 {
   if ( !fmt ) return;

   char buf[256];
   va_list args;

   buf[0] = 0;

   va_start ( args, fmt );
   vsprintf ( buf, fmt, args );
   va_end   ( args );

   set ( buf );
 }

SrString::SrString ( const SrString& st ) : _capacity(0), _data(_empty)
 {
   set(st._data);
 }

SrString::~SrString ()
 { 
   if ( _data!=_empty ) delete[] _data;
 }

SrString& SrString::set ( char c, int len )
 {
   if ( len<0 ) len=0;
   capacity ( len+1 );
   memset ( _data, c, len );
   return *this;
 }

SrString& SrString::set ( const char *st )
 {
   if ( st==_data ) return *this;

   if ( !st )
    { if ( _data!=_empty ) { delete[] _data; _capacity=0; _data=_empty; }
    }
   else if ( !st[0] ) 
    { if ( _capacity ) _data[0]=0;
    }
   else 
    { int size = (int)strlen(st)+1;
      if ( _capacity<size )
       { if ( _data!=_empty ) delete[] _data;
         _capacity = size;
         _data = new char[_capacity];
       }
      strcpy ( _data, st );
    }

   SR_TRACE3 ( "set:[" << _data << "] capacity:" << _capacity << " pt:" << (int)_data );
   return *this;
 }

SrString& SrString::setf ( const char *fmt, ... )
 {
   char buf[256];
   va_list args;

   buf[0] = 0;

   va_start ( args, fmt );
   vsprintf ( buf, fmt, args );
   va_end   ( args );

   return set ( buf );
 }

int SrString::len () const
 {
   return (int) strlen (_data);
 }

void SrString::len ( int l )
 {
   if ( _capacity < l+1 ) capacity ( l+1 );
   _data[l] = 0;
 }

int SrString::capacity () const
 {
   return _capacity;
 }

void SrString::capacity ( int c )
 {
   if ( c<0 ) c=0;
   if ( _capacity==c ) return;
   _capacity = c;

   if ( c==0 )
    { if ( _data!=_empty ) { delete[] _data; _data=_empty; }
    }
   else 
    { if ( _data==_empty ) { c=1; _data=0; }
      _data = sr_string_realloc ( _data, _capacity );
      _data[c-1] = 0;
    } 
 }

void SrString::compress ()
 {
   if ( !_capacity ) return;

   int len = (int)strlen(_data);

   if ( len+1==_capacity ) return; // is compressed

   if ( !len ) 
    { delete[] _data; 
      _data = _empty;
      _capacity = 0;
    }
   else
    { _capacity = len+1;
      _data = sr_string_realloc ( _data, _capacity );
    }

   SR_TRACE2 ( "compressed to:[" << _data << "] capacity:" << _capacity  << " pt:" << (int)_data );
 }

void SrString::trim ()
 {
   int inf, sup;
   bounds    ( inf, sup );
   substring ( inf, sup );
   SR_TRACE2 ( "trim:[" << _data << "]" );
 }

void SrString::ltrim ()
 {
   int inf, sup;
   bounds    ( inf, sup );
   substring ( inf, -1 );
   SR_TRACE2 ( "ltrim:[" << _data << "]" );
 }

void SrString::rtrim ()
 {
   int inf, sup;
   bounds    ( inf, sup );
   substring ( 0,   sup );
   SR_TRACE2 ( "rtrim:[" << _data << "]" );
 }

void SrString::bounds ( int &xi, int &xf ) const
 {
   xi = 0;
   xf = (int)strlen(_data)-1;
   if ( xf<0 ) { xi=-1; return; }

   while ( xi<=xf && isspace(_data[xi]) ) xi++;
   while ( xf>=xi && isspace(_data[xf]) ) xf--;
 }

void SrString::substring ( int inf, int sup )
 {
   int n=0;
   int max=(int)strlen(_data)-1;
   
   if ( max<0 ) return;
   if ( inf<0 ) inf=0;
   if ( sup<0 || sup>max ) sup=max;
   
   while (inf<=sup) _data[n++] = _data[inf++];
   _data[n] = 0;
 }

char SrString::last_char () const
 {
   int len = (int)strlen(_data);
   if ( len==0 ) return 0;
   return _data[len-1];
 }

void SrString::last_char ( char c )
 {
   int len = (int)strlen(_data);
   if ( len==0 ) return;
   _data[len-1] = c;
 }

void SrString::get_substring ( SrString& s, int inf, int sup ) const
 {
   int max=(int)strlen(_data)-1;
   if ( max<0 ) return;
   if ( sup<0 || sup>max ) sup=max;
   if ( inf<0 ) inf=0;
   if ( inf>sup ) return;
   s.len ( sup-inf+1 );
   int i=0;
   while ( inf<=sup ) s[i++]=_data[inf++];
   s[i]=0;
 }

int SrString::get_next_string ( SrString& s, int i ) const
 {
   int i1, i2;
   int max=(int)strlen(_data)-1;

   if ( max<0 ) return -1;
   if ( i<0 ) i=0;

   while ( i<=max && isspace(_data[i]) ) i++; // cut leading spaces
   i1 = i;
   while ( i<=max && !isspace(_data[i]) ) i++; // advance untill finding other white characters
   i2 = i;
   
   if ( i2>i1 ) 
    { get_substring ( s, i1, i2-1 );
      return i2;
    }
   else
    { return -1;
    }
 }

void SrString::lower ()
 {
   char *pt = _data;
   while ( *pt ) { *pt=(char)SR_LOWER(*pt); pt++; }
 }

void SrString::upper ()
 {
   char *pt = _data;
   while ( *pt ) { *pt=(char)SR_UPPER(*pt); pt++; }
 }

int SrString::search ( char c ) const
 {
   int len = (int)strlen(_data);
   for ( int i=0; i<len; i++ ) if (_data[i]==c) return i;
   return -1;
 }

int SrString::search ( const char *st, bool ci ) const
 {
   int cmp;
   int len = (int)strlen(st);
   if ( len==0 ) return -1;

   int last_index = (int)strlen(_data)-len;
   for ( int i=0; i<=last_index; i++ )
    { cmp = ci? sr_compare ( _data+i, st, len ) :
                sr_compare_cs ( _data+i, st, len );
      if ( cmp==0) return i;
    }

   return -1;
 }

int SrString::remove_file_name ()
 {
   int i;

   for ( i=len()-1; i>=0; i-- )
    { if ( _data[i]=='/' || _data[i]=='\\' )
       { _data[i+1]=0;
         break;
       }
    }

   return i;
 }

int SrString::remove_path ()
 {
   int i;

   for ( i=len()-1; i>=0; i-- )
    { if ( _data[i]=='/' || _data[i]=='\\' )
       { substring(i+1,-1);
         break;
       }
    }

   return i;
 }

int SrString::get_file_name ( SrString& filename ) const
 {
   filename.set ( _data );
   return filename.remove_path ();
 }

int SrString::extract_file_name ( SrString& filename )
 {
   int i = get_file_name ( filename );
   if ( i>=0 ) _data[i+1]=0;
   return i;
 }

int SrString::remove_file_extension ()
 {
   int i;

   for ( i=len()-1; i>=0; i-- )
    { if ( _data[i]=='.' ) _data[i]=0;
      if ( _data[i]=='/' || _data[i]=='\\' ) return -1; // no extension
    }

   return i;
 }

int SrString::get_file_extension ( SrString& ext ) const
 {
   int i;

   ext.set ( _data );

   for ( i=len()-1; i>=0; i-- )
    { if ( _data[i]=='.' )
       { ext.substring(i+1,-1);
         break;
       }
      if ( _data[i]=='/' || _data[i]=='\\' ) return -1; // no extension
    }

   return i;
 }

int SrString::extract_file_extension ( SrString& ext )
 {
   int i = get_file_extension ( ext );
   if ( i>=0 ) _data[i]=0;
   return i;
 }

bool SrString::has_file_extension ( const char* ext ) const
 {
   int i;

   for ( i=len()-1; i>=0; i-- )
    { if ( _data[i]=='.' )
       { if ( !ext ) return true;
         return sr_compare_cs ( _data+i+1, ext )==0? true:false;
       }
      if ( _data[i]=='/' || _data[i]=='\\' ) break; // early termination
    }

   return false;
 }

bool SrString::make_valid_path ()
 {
   replace_all ( "\\", "/" );

   if ( _data[0]==0 ) return false;

   if ( last_char()!='/' ) append ( "/" );

   return true;
 }

bool SrString::make_valid_path ( const char* s )
 {
   if ( !s ) return false;
   set ( s );
   return make_valid_path();
 }

bool SrString::has_absolute_path ( const char* s ) // static function
 {
   if ( !s ) return false;
   if ( !s[0] ) return false;
   if ( s[0]=='\\' || s[0]=='/' ) return true; // /dir, \\machine\dir
   if ( !s[1] ) return false; // eg a 1-letter filename
   if ( s[1]==':' ) return true; // c:dir is considered absolute
   return false;
 }

bool SrString::make_valid_string ( const char* s )
 {
   bool need_quotes = false;
   bool need_slash = false;

   int i;
   int len = (int)strlen ( s );

   if ( len==0 )
     need_quotes = true;
   else
    { for ( i=0; i<len; i++ )
       { if ( i==0 && isdigit(s[i]) ) { need_quotes=true; break; }
         if ( isspace(s[i]) ) { need_quotes=true; break; }
         if ( s[i]=='"' ) { need_slash=true; break; }
         if ( strchr(SR_INPUT_DELIMITERS,s[i]) ) { need_quotes=true; break; }
       }
    }

   if ( !need_quotes && !need_slash )
    { set(s);
      return false;
    }

   SrString::len ( 0 );
   capacity ( len*2+3 ); // sure to hold all cases

   int p=0;
   if ( need_quotes ) _data[p++]='"';

   for ( i=0; i<len; i++ )
    { if ( s[i]=='"' ) _data[p++]='\\';
      _data[p++] = s[i];
    }

   if ( need_quotes ) _data[p++]='"';
   _data[p] = 0;

   return true;
 }

int SrString::atoi () const
 {
   return ::atoi(_data);
 }

float SrString::atof () const
 {
   return (float)::atof(_data);
 }

double SrString::atod () const
 {
   return ::atof(_data);
 }

void SrString::append ( const char* st )
 {
   if ( !st || !st[0] ) return;

   char *tmp = 0;
   if ( st==_data ) { tmp=sr_string_new(st); st=tmp; }

   int newlen = (int)strlen(_data)+(int)strlen(st);

   if ( newlen<_capacity )
    { strcat(_data,st);
    }
   else
    { if ( _data==_empty ) _data=0; // so to let realloc work
      _capacity = (newlen+1)*2;
      _data = sr_string_realloc ( _data, _capacity );
      strcat ( _data, st );
    }

   delete[] tmp;
 }

void SrString::insert ( int i, const char *st )
 { 
   if ( !st ) return;
   if ( i<0 ) i=0;
   
   int len = (int)strlen(_data);
   if ( i>=len ) { append(st); return; }

   int dp = (int)strlen ( st );
   if ( !dp ) return;

   int ns = len+dp+1;

   // at this point _data!=_empty, because of the i>=len test.
   
   if ( ns>_capacity ) 
    { _capacity = ns*2;
      _data = sr_string_realloc ( _data, _capacity );
    }

   // memmove deals with overlapped regions
   memmove ( _data+i+dp, _data+i, sizeof(char)*(len-i+1) ); // copies also the ending 0

   // now we fill the created space :
   memcpy ( _data+i, st, sizeof(char)*dp );
 }

void SrString::remove ( int i, int dp )
 {
   if ( _data==_empty || dp<=0 ) return;
   int len = (int)strlen(_data);
   if ( i>=len || i<0 ) return;
   if ( i+dp>=len ) { _data[i]=0; return; }

   // memmove deals with overlapped regions :
   memmove ( _data+i, _data+i+dp, sizeof(char)*(len-i-dp+1) ); // copies also the ending 0
 }

int SrString::replace ( const char* oldst, const char* newst, bool ci )
 {
   int i = search ( oldst, ci );
   //   printf("%d\n",i);

   if ( i<0 ) return i; // not found

   int oldlen = (int)strlen(oldst);
   int newlen = newst? (int)strlen(newst):0;
   
   if ( oldlen<newlen ) // open space
    { SrString dif ( ' ', newlen-oldlen );
      insert ( i, dif );
    }
   else if ( oldlen>newlen ) // remove space
    { remove ( i, oldlen-newlen );
    }

   // void *memmove( void *dest, const void *src, size_t count );
   // The memmove function copies count bytes of characters from src to dest.
   // If some regions of the source area and the destination overlap,
   // memmove ensures that the original source bytes in the overlapping
   // region are copied before being overwritten.
   if ( newlen>1 ) memmove ( _data+i, newst, sizeof(char)*newlen );
    else if ( newlen==1 ) _data[i]=newst[0];

   return i;
 }

int SrString::replace_all ( const char* oldst, const char* newst, bool ci )
 {
   int count=0;
   while ( replace(oldst,newst,ci)!=-1 ) count++;
   return count;
 }

void SrString::take_data ( SrString &s )
 {
   if ( _data!=_empty ) delete[] ( _data );

   _data       = s._data;
   _capacity   = s._capacity;
   s._data     = _empty;
   s._capacity = 0;

   SR_TRACE2 ( "take_data!" ); 
 }

char* SrString::leave_data ( char*& s )
 {
   s = _data;
   if ( s==_empty ) { s=new char[1]; s[0]=0; }

   _data = _empty;
   _capacity = 0;

   SR_TRACE2 ( "leave_data!" ); 
   return s;
 }

SrString& SrString::operator << ( const char* st )
 {
   append(st);
   return *this;
 }
 
SrString& SrString::operator << ( int i )
 {
   char buf[64];
   sprintf ( buf, "%d", i );
   append(buf);
   return *this; 
 }

SrString& SrString::operator << ( float f )
 {
   char buf[64];
   sprintf ( buf, "%f", f );
   append(buf);
   return *this; 
 }

SrString& SrString::operator << ( double d )
 {
   char buf[64];
   sprintf ( buf, "%f", d );
   append(buf);
   return *this; 
 }

SrString& SrString::operator << ( char c )
 {
   char buf[2];
   buf[0]=c; buf[1]=0;
   append(buf);
   return *this; 
 }

SrString& SrString::operator = ( const SrString &s )
 {
   set(s._data);
   return *this;
 }

SrString& SrString::operator = ( const char* st )
 {
   set(st);
   return *this;
 }

SrString& SrString::operator = ( int i )
 {
   set("");
   return *this<<i;
 }

SrString& SrString::operator = ( char c )
 {
   set("");
   return *this<<c;
 }

SrString& SrString::operator = ( float f )
 {
   set("");
   return *this<<f;
 }

SrString& SrString::operator = ( double d )
 {
   set("");
   return *this<<d;
 }

//================================= End of File ======================================


