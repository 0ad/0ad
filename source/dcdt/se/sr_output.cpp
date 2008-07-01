#include "precompiled.h"
#include "0ad_warning_disable.h"
# include <stdarg.h> 
# include <stdlib.h> 
# include <stdio.h>

# include "sr_output.h"
# include "sr_string.h"

//============================== Global data ====================================

SrOutput sr_out;

static const char* DefaultIntFmt = "%d"; 
static const char* DefaultFloatFmt = "g";
static const char* DefaultDoubleFmt = "g";

//=============================== SrOutput ======================================

void SrOutput::_init () // this can be only called from constructors
 { 
   _type = TypeConsole;
   _margin = 0;
   _margin_char = '\t';
   _intfmt = DefaultIntFmt;
   _floatfmt = DefaultFloatFmt;
   _doublefmt = DefaultDoubleFmt;
   _func_udata = 0;
   _filename = 0;
   default_formats(); 
 }

SrOutput::SrOutput ()
 { 
   _init ();
 }
   
SrOutput::SrOutput ( FILE *f )
 { 
   _init ();
   _type = TypeFile;
   _device.file = f; 
 }

SrOutput::SrOutput ( const char* filename, const char* mode )
 { 
   _init ();
   _type = TypeFile;
   _device.file = fopen(filename,mode); 
   sr_string_set(_filename,filename);
 }

SrOutput::SrOutput ( SrString &s )
 { 
   _init ();
   _type = TypeString;
   _device.string=&s; 
 }

SrOutput::SrOutput ( void(*f)(const char*,void*), void* udata )
 {
   _init ();
   _type = TypeFunction;
   _func_udata = udata;
   _device.func = f;
 }

SrOutput::~SrOutput ()
 {
   close ();
   default_formats (); // will free pointers only if needed (no defaults used)
 }

void SrOutput::leave_file ()
 {
   _device.file = 0;
   _type=TypeConsole; 
 }

FILE* SrOutput::filept ()
 {
   return _type==TypeFile? _device.file:0;
 }

void SrOutput::init () 
 {
   close(); 
   _type=TypeConsole; 
 }

void SrOutput::init ( FILE *f ) 
 { 
   close(); 
   _type=TypeFile; 
   _device.file=f; 
 }

void SrOutput::init ( const char* filename, const char* mode )
 { 
   init ( fopen(filename,mode) );
   sr_string_set(_filename,filename);
 }

void SrOutput::init ( SrString &s ) 
 { 
   close(); 
   _type=TypeString; 
   _device.string=&s; 
 }

void SrOutput::init ( void(*f)(const char*,void*), void* udata ) 
 { 
   close(); 
   _type=TypeFunction; 
   _device.func=f;
   _func_udata = udata; 
 }

bool SrOutput::valid ()
 {
   switch ( _type )
    { case TypeConsole  : return true;
	  case TypeFile     : return _device.file ?   true:false;
	  case TypeString   : return _device.string ? true:false;
	  case TypeFunction : return _device.func ?   true:false;
    }
   return false;
 }

void SrOutput::close ()
 { 
   if ( _type==TypeFile )
    { if ( _device.file ) 
       { fclose ( _device.file ); 
         _device.file = 0;
       } 
    }
   sr_string_set ( _filename, 0 );
 }

static void set_fmt ( const char*& fmt, const char* def, const char* newfmt )
 {
   if ( fmt==def ) fmt=0;
    else sr_string_set ( fmt, 0 ); 

   if ( newfmt==def ) fmt=def;
    else sr_string_set ( fmt, newfmt );
 }
 
void SrOutput::fmt_int ( const char* s )
 {
   set_fmt ( _intfmt, DefaultIntFmt, s );
 }

void SrOutput::fmt_float ( const char* s )
 {
   set_fmt ( _floatfmt, DefaultFloatFmt, s );
 }

void SrOutput::fmt_double ( const char* s )
 {
   set_fmt ( _doublefmt, DefaultDoubleFmt, s );
 }

void SrOutput::outm ()
 {
   int i;
   for ( i=0; i<_margin; i++ ) put ( _margin_char ); 
 }

void SrOutput::flush ()
 {
   switch ( _type )
    { case TypeConsole  : fflush(stdout); break;
      case TypeFile     : fflush(_device.file); break;
      case TypeString   : break;
      case TypeFunction : break;
    }
 }

void SrOutput::default_formats () 
 { 
   fmt_int ( DefaultIntFmt );
   fmt_float ( DefaultFloatFmt );
   fmt_double ( DefaultDoubleFmt );
 }

void SrOutput::put ( char c )
 {
   switch ( _type )
    { case TypeConsole  : fputc(c,stdout); break;
      case TypeFile     : fputc(c,_device.file); break;
      case TypeString   : { char st[2]; st[0]=c; st[1]=0; _device.string->append(st); } break;
      case TypeFunction : { char st[2]; st[0]=c; st[1]=0; _device.func(st,_func_udata); } break;
    }
 }

void SrOutput::put ( const char *st )
 {
   switch ( _type )
    { case TypeConsole  : fputs(st,stdout); break;
      case TypeFile     : fputs(st,_device.file); break;
      case TypeString   : _device.string->append(st); break;
      case TypeFunction : _device.func(st,_func_udata); break;
    }
 }

void SrOutput::putf ( const char *fmt, ... )
 {
   char buf[256];
   va_list args;

   buf[0] = 0;

   va_start ( args, fmt );
   vsprintf ( buf, fmt, args );
   va_end   ( args );

   put ( buf );
 }

void SrOutput::fatal_error ( const char *fmt, ... )
 {
   char buf[256];
   va_list args;

   buf[0] = 0;

   va_start ( args, fmt );
   vsprintf ( buf, fmt, args );
   va_end   ( args );

   put ( "\nSR Fatal Error !\n" );
   put ( buf );
   put ( "\nPress any key to exit..." );
   flush ();
   getchar();
   put ( "\n" );

   exit ( 1 );
 }

void SrOutput::warning ( const char *fmt, ... )
 {
   char buf[256];
   va_list args;

   buf[0] = 0;

   va_start ( args, fmt );
   vsprintf ( buf, fmt, args );
   va_end   ( args );

   put ( "\nSR Warning:\n" );
   put ( buf );
   put ( "\n" );
   flush ();
 }

//=============================== friends ===============================

SrOutput& operator<< ( SrOutput& o, const char *st )
 { 
   o.put(st);
   return o;
 }

SrOutput& operator<< ( SrOutput& o, const SrString& st )
 {
   o.put((const char*)st);
   return o;
 }

SrOutput& operator<< ( SrOutput& o, char c )
 {
   o.put(c);
   return o; 
 }

SrOutput& operator<< ( SrOutput& o, srbyte c )
 {
   o.putf(o._intfmt,(int)c);
   return o;
 }

SrOutput& operator<< ( SrOutput& o, int i )
 {
   o.putf(o._intfmt,i);
   return o; 
 }

SrOutput& operator<< ( SrOutput& o, bool b )
 {
   o.put ( b? "true":"false" );
   return o; 
 }

SrOutput& operator<< ( SrOutput& o, short s )
 {
   o.putf(o._intfmt,(int)s);
   return o;
 }

SrOutput& operator<< ( SrOutput& o, sruint i )
 {
   o.putf(o._intfmt,(int)i);
   return o; 
 }

SrOutput& operator<< ( SrOutput& o, float f )
 {
   if ( o._floatfmt[0]=='g' )
    { if ( f==int(f) )
       o.putf("%d.0",int(f));
      else
       o.putf("%g",f);
    }
   else if ( o._floatfmt[0]=='i' )
    { if ( f==int(f) )
       o.putf("%d",int(f));
      else
       o.putf("%f",f);
    }
   else o.putf(o._floatfmt,f);

   return o;
 }

SrOutput& operator<< ( SrOutput& o, double d )
 {
   if ( o._doublefmt[0]=='g' )
    { o.putf("%g",d);
      if ( d==int(d) ) o<<".0";
    }
   else if ( o._doublefmt[0]=='i' )
    { if ( d==int(d) )
       o.putf("%d",int(d));
      else
       o.putf("%f",d);
    }
   else o.putf(o._doublefmt,d);
   return o; 
 }

//============================== end of file ===============================
