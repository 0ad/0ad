#include "precompiled.h"
# include "sr_array_pt.h"

//====================== SrArrayPtBase ==========================

SrArrayPtBase::SrArrayPtBase ( SrClassManagerBase* m )
 {
   _man = m;
   _man->ref();
 }

SrArrayPtBase::SrArrayPtBase ( const SrArrayPtBase& a )
 {
   _man = a._man;
   _man->ref();
   *this = a; // calls copy operator
 }

SrArrayPtBase::~SrArrayPtBase ()
 {
   init ();
   _man->unref(); // must be called after init()!
 }

void SrArrayPtBase::init ()
 {
   while ( SrArray<void*>::size()>0 ) _man->free ( SrArray<void*>::pop() );
 }

void SrArrayPtBase::size ( int ns )
 {
   int i, s = SrArray<void*>::size();
   if ( ns>s )
    { SrArray<void*>::size(ns);
      for ( i=s; i<ns; i++ ) SrArray<void*>::set ( i, _man->alloc() );
    }
   else if ( ns<s )
    { for ( i=ns; i<s; i++ ) _man->free ( SrArray<void*>::get(i) );
      SrArray<void*>::size(ns);
    }
 }

void SrArrayPtBase::capacity ( int nc )
 {
   int i, s = SrArray<void*>::size();
   if ( nc<0 ) nc=0;
   if ( nc<s )
    { for ( i=nc; i<s; i++ ) _man->free ( SrArray<void*>::get(i) );
    }
   SrArray<void*>::capacity(nc);
 }

void SrArrayPtBase::compress ()
 {
   capacity ( size() );
 }

void SrArrayPtBase::swap ( int i, int j )
 {
   void *pti = SrArray<void*>::get(i);
   void *ptj = SrArray<void*>::get(j);
   SrArray<void*>::set ( i, ptj );
   SrArray<void*>::set ( j, pti );
 }

void SrArrayPtBase::set ( int i, const void* pt )
 {
   SR_ASSERT ( i>=0 && i<size() );
   _man->free ( SrArray<void*>::get(i) );
   SrArray<void*>::set ( i, _man->alloc(pt) );
 }

void* SrArrayPtBase::get ( int i ) const
 {
   SR_ASSERT ( i>=0 && i<size() );
   return SrArray<void*>::get(i);
 }

const void* SrArrayPtBase::const_get ( int i ) const
 {
   SR_ASSERT ( i>=0 && i<size() );
   return SrArray<void*>::const_get(i);
 }

void* SrArrayPtBase::top () const
 {
   if ( size()==0 ) return 0;
   return SrArray<void*>::get ( size()-1 );
 }

void SrArrayPtBase::pop ()
 {
   if ( size()>0 ) _man->free ( SrArray<void*>::pop() );
 }

void SrArrayPtBase::push ()
 {
   SrArray<void*>::push() = _man->alloc ();
 }

void SrArrayPtBase::insert ( int i, int dp )
 {
   SrArray<void*>::insert ( i, dp );
   int j;
   for ( j=0; j<dp; j++ )
    SrArray<void*>::set ( i+j, _man->alloc() );
 }

void SrArrayPtBase::remove ( int i, int dp )
 {
   int j;
   for ( j=0; j<dp; j++ )
    _man->free( SrArray<void*>::get(i+j) );
   SrArray<void*>::remove ( i, dp );
 }

void* SrArrayPtBase::extract ( int i )
 {
   void *pt = SrArray<void*>::get(i);
   SrArray<void*>::remove ( i );
   return pt;
 }

void SrArrayPtBase::operator = ( const SrArrayPtBase& a )
 {
   init (); // deletes all data
   SrArray<void*>::size ( a.size() );
   SrArray<void*>::compress();
   int i;
   for ( i=0; i<a.size(); i++ ) SrArray<void*>::set ( i, _man->alloc(a[i]) );
 }

static SrClassManagerBase* StaticManager = 0; // This is not thread safe...

static int fcmp ( const void* pt1, const void* pt2 )
 {
   typedef const int* cint;
   return StaticManager->compare( (const void*)*cint(pt1), (const void*)*cint(pt2) );
 }

int SrArrayPtBase::insort ( const void* pt, bool allowdup )
 {
   int pos;
   pos = SrArray<void*>::insort ( (void *const&)pt,
                         (int(*)(void *const *,void *const *))fcmp,
                         allowdup );
   if ( pos>=0 ) SrArray<void*>::get(pos) = _man->alloc(pt);
   return pos;
 }

void SrArrayPtBase::sort ()
 {
   StaticManager = _man;
   SrArray<void*>::sort ( (int(*)(void *const *,void *const *))fcmp );
 }

int SrArrayPtBase::lsearch ( const void* pt ) const
 {
   StaticManager = _man;
   return SrArray<void*>::lsearch ( (void *const&)pt, (int(*)(void *const *,void *const *))fcmp );
 }

int SrArrayPtBase::bsearch ( const void* pt, int *pos ) 
 {
   StaticManager = _man;
   return SrArray<void*>::bsearch ( (void *const&)pt, (int(*)(void *const *,void *const *))fcmp, pos );
 }

void SrArrayPtBase::take_data ( SrArrayPtBase& a )
 {
   size(0);
   capacity(0);
   SrArray<void*>::take_data(a);
 }

SrOutput& operator<< ( SrOutput& o, const SrArrayPtBase& a )
 {
   int i, m;
   m = a.size()-1;
   o << '[';
   for ( i=0; i<=m; i++ )
    { a._man->output ( o, a[i] );
      if ( i<m ) o<<srspc;
    }
   return o << ']';
 }

SrInput& operator>> ( SrInput& in, SrArrayPtBase& a )
 {
   a.size(0);
   in.get_token();
   while (true)
    { in.get_token();
      if ( in.last_token()[0]==']' ) break;
      in.unget_token ();
      a.push ();
      a._man->input ( in, a.top() );
    }
   return in;
 }

//=========================== EOF ===============================
