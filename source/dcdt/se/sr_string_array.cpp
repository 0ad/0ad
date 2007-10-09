#include "precompiled.h"
# include "sr_string_array.h"

//# define SR_USE_TRACE1
# include "sr_trace.h"

//====================== SrStringArray ==========================

# define SETZERO(i,ini,size) for(i=ini;i<size;i++) SrArray<char*>::set(i,0)
# define DELETE(i,ini,size) for(i=ini;i<size;i++) delete SrArray<char*>::get(i)

SrStringArray::SrStringArray ( int s, int c )
              :SrArray<char*>(s,c)
 {
   int i;
   SETZERO(i,0,s);
 }

SrStringArray::SrStringArray ( const SrStringArray& a )
              :SrArray<char*>(0,0)
 {
   *this = a; // calls copy operator
 }

SrStringArray::~SrStringArray ()
 {
   size(0);
 }

void SrStringArray::size ( int ns )
 {
   int i, s = size();
   if ( ns>s )
    { SrArray<char*>::size(ns);
      SETZERO(i,s,ns);
    }
   else if ( ns<s )
    { DELETE(i,ns,s);
      SrArray<char*>::size(ns);
    }
 }

void SrStringArray::capacity ( int nc )
 {
   int i, s = size();
   if ( nc<s )
    { DELETE(i,nc,s);
    }
   SrArray<char*>::capacity(nc);
 }

void SrStringArray::compress ()
 {
   capacity ( size() );
 }

void SrStringArray::setall ( const char* s )
 {
   int i;
   for ( i=0; i<size(); i++ ) set(i,s);
 }

void SrStringArray::set ( int i, const char* s )
 {
   SR_ASSERT ( i>=0 && i<size() );
   delete SrArray<char*>::get(i);
   SrArray<char*>::set ( i, sr_string_new(s) );
 }

const char* SrStringArray::get ( int i ) const
 {
   SR_ASSERT ( i>=0 && i<size() );
   const char* st = SrArray<char*>::get ( i );
   return st? st:"";
 }

const char* SrStringArray::top () const
 {
   if ( size()==0 ) return 0;
   return SrArray<char*>::get ( size()-1 );
 }

void SrStringArray::pop ()
 {
   if ( size()>0 ) delete SrArray<char*>::pop();
 }

void SrStringArray::push ( const char* s )
 {
   SrArray<char*>::push() = sr_string_new(s);
 }

void SrStringArray::insert ( int i, const char* s, int dp )
 {
   SrArray<char*>::insert ( i, dp );
   int j;
   for ( j=0; j<dp; j++ )
   SrArray<char*>::get(i+j) = sr_string_new(s);
 }

void SrStringArray::remove ( int i, int dp )
 {
   int j;
   for ( j=0; j<dp; j++ )
    delete SrArray<char*>::get(i+j);
   SrArray<char*>::remove ( i, dp );
 }

void SrStringArray::operator = ( const SrStringArray& a )
 {
   size ( 0 ); // deletes all data
   SrArray<char*>::size ( a.size() );
   SrArray<char*>::compress();
   int i;
   for ( i=0; i<a.size(); i++ ) SrArray<char*>::set ( i, sr_string_new(a[i]) );
 }

static int fcmpst ( const void* pt1, const void* pt2 )
 {
   typedef const char* cchar;
   return sr_compare( *((cchar*)pt1), (cchar)pt2 );
 }

static int fcmppt ( const void* pt1, const void* pt2 )
 {
   typedef const char* cchar;
   return sr_compare( *((cchar*)pt1), *((cchar*)pt2) );
 }

int SrStringArray::insort ( const char* s, bool allowdup )
 {
   int pos;
   pos = SrArrayBase::insort ( sizeof(char*), s, fcmpst, allowdup );
   if ( pos>=0 ) SrArray<char*>::get(pos) = sr_string_new(s);
   return pos;
 }

void SrStringArray::sort ()
 {
   SrArrayBase::sort ( sizeof(char*), fcmppt );
 }

int SrStringArray::lsearch ( const char* s ) const
 {
   return SrArrayBase::lsearch ( sizeof(char*), s, fcmpst );
 }

int SrStringArray::bsearch ( const char* s, int *pos ) 
 {
   return SrArrayBase::bsearch ( sizeof(char*), s, fcmpst, pos );
 }

int SrStringArray::push_path ( const char* path )
 {
   // validate path:
   SrString spath;
   if ( !spath.make_valid_path(path) ) return -1;

   // check if already there:
   int i;
   for ( i=0; i<size(); i++ )
    { if ( sr_compare_cs(get(i),spath)==0 ) return -1; 
    }
    
   // ok, push it:
   push ( spath );
   return size()-1;
 }

bool SrStringArray::open_file ( SrInput& inp, const char* filename, const char* mode, const char* basedir )
 {
   // If absolute path, just try to open it:
   if ( SrString::has_absolute_path(filename) )
    { inp.init(filename,mode);
      return inp.valid();
    }
 
   // Otherwise search in the paths:
   int i;
   SrString fullfile;

   // first search in the declared paths:
   for ( i=0; i<size(); i++ )
    { fullfile = "";
      if ( basedir && !SrString::has_absolute_path(get(i)) ) fullfile << basedir;
      fullfile << get(i);
      fullfile << filename;
      SR_TRACE1("Trying: "<<fullfile);
      inp.init ( fopen(fullfile,mode) );
      if ( inp.valid() ) break; // found
    }
    
   // try only in basedir path:
   if ( !inp.valid() && basedir )
    { fullfile = basedir;
      fullfile << filename;
      SR_TRACE1("Trying: "<<fullfile);
      inp.init ( fopen(fullfile,mode) );
    }
    
   // try only filename, ie in cur folder:
   if ( !inp.valid() )
    { fullfile = filename;
      SR_TRACE1("Trying: "<<fullfile);
      inp.init ( fopen(fullfile,mode) );
    }
   
   // if could not open, return:
   if ( !inp.valid() ) return false;
   
   // found it:
   inp.filename ( fullfile );
   return true;
 }

void SrStringArray::take_data ( SrStringArray& a )
 {
   size(0);
   capacity(0);
   SrArray<char*>::take_data(a);
 }

SrOutput& operator<< ( SrOutput& o, const SrStringArray& a )
 {
   o << '[';
   for ( int i=0; i<a.size(); i++ )
    { o << '"' << a[i] << '"' ;
      if ( i<a.size()-1 ) o<<srspc;
    }
   return o << ']';
 }

SrInput& operator>> ( SrInput& in, SrStringArray& a )
 {
   a.size(0);
   in.get_token();
   while (true)
    { in.get_token();
      if ( in.last_token()[0]==']' ) break;
      a.push ( in.last_token() );
    }
   return in;
 }

//=========================== EOF ===============================
