#include "precompiled.h"
# include <stdlib.h>
# include "sr_set.h"

//============================= SrSetBasic ================================

int SrSetBasic::insert ( void* pt )
 {
   int i;

   if ( _freepos.size()>0 )
    { i=_freepos.pop(); _data[i]=pt; }
   else
    { i=_data.size(); _data.push()=pt; }

   return i;
 }

void* SrSetBasic::extract ( int i )
 {
   if ( i<0 || i>=_data.size() ) return 0;
   if ( !_data[i] ) return 0;

   void* pt = _data[i];
   _data[i]=0;

   if ( i==_data.size()-1 )
    { _data.pop(); }
   else
    { _freepos.push()=i; }

   return pt;
 }

void SrSetBasic::compress ()
 {
   if ( _data.size()>0 )
    { int i, n;
      while ( _data.top()==0 )
       { _data.pop();
         n = _data.size();
         for ( i=0; i<_freepos.size(); i++ )
           if ( _freepos[i]==n ) { _freepos[i]=_freepos.pop(); break; }
       }
    }
   _data.compress();
   _freepos.compress();
 }

bool SrSetBasic::remove_gaps ( SrArray<int>& newindices )
 {
   int i, j, gap=0;

   newindices.size ( _data.size() );

   // advance untill first null
   for ( i=0; i<_data.size(); i++ )
    { if ( _data[i] ) newindices[i]=i;
       else break;
    }
   if ( i==_data.size() ) return false; // nothing done

   j = i;
   while ( i<_data.size() )
    { if ( !_data[i] )
       { newindices[i] = -1; }
      else
       { _data[j]=_data[i];
         _data[i]=0;
         newindices[i] = j;
         j++;
       }
      i++;
    }

   _data.size ( j );
   _freepos.size ( 0 );
   return true;
 }

//============================== SrSetBase ================================

SrSetBase::SrSetBase ( SrClassManagerBase* m )
 {
   _man = m;
   _man->ref();
 }

SrSetBase::SrSetBase ( const SrSetBase& s )
 {
   _man = s._man;
   _man->ref();
   copy ( s );
 }

SrSetBase::~SrSetBase ()
 {
   init ();
   _man->unref();
 }

void SrSetBase::init ()
 {
   void* pt;
   while ( _data.size() )
    { pt = _data.pop();
      if ( pt ) _man->free ( pt );
    }
   _freepos.size(0); 
 }

void SrSetBase::copy ( const SrSetBase& s )
 {
   init ();
   _freepos = s._freepos;
   _data = s._data;

   int i;
   for ( i=0; i<_data.size(); i++ )
    if ( _data[i] ) _data[i]=_man->alloc(s._data[i]);
 }

void SrSetBase::remove ( int i )
 {
   void* pt = extract ( i );
   if ( pt ) _man->free ( pt );
 }

SrOutput& operator<< ( SrOutput& out, const SrSetBase& s )
 {
   int i, lasti, n=0;

   lasti = s.maxid();
   while ( lasti>=0 && s._data[lasti]==0 ) lasti--;

   out<<'[';

   for ( i=0; i<s._data.size(); i++ )
    { if ( s._data[i]==0 ) continue;
      s._man->output(out,s._data[i]);
      if ( n++<i ) out<<':'<<i;
      if ( i<lasti) out<<srspc;
    }

   out<<']';
   return out;
 }

SrInput& operator>> ( SrInput& inp, SrSetBase& s )
 {
   s.init ();
   int id, mi;
   SrArray<int> ids;
 
   inp.get_token(); // '['
   while (true)
    { inp.get_token();
      if ( inp.last_token()[0]==']' )
       { break; }
      else inp.unget_token();

      s._data.push() = s._man->alloc();
      s._man->input ( inp, s._data.top() );

      inp.get_token();
      if ( inp.last_token()[0]==':' )
       { id = atoi(inp.getn());
         mi = s.maxid();
         while ( mi<id )
          { s._freepos.push()=mi;
            s._data.push() = s._data[mi];
            s._data[mi]=0;
            mi = s.maxid();
          }
       }
      else inp.unget_token();
    }

   return inp;
 }

//=========================== SrSetIteratorBase ================================

SrSetIteratorBase::SrSetIteratorBase ( const SrSetBase& s ) : _set(s)
 { 
   reset ();
 }

void SrSetIteratorBase::reset ()
 {
   _minid = 0;
   _maxid = _set.maxid();
   while ( _minid<=_maxid && !_set.get(_minid) ) _minid++;
   if ( _minid<_maxid ) while ( !_set.get(_maxid) ) _maxid--; 
   _curid = _minid;
 }

void SrSetIteratorBase::next ()
 {
   _curid++;
   while ( _curid<=_maxid && !_set.get(_curid) ) _curid++;
 } 

void SrSetIteratorBase::prior ()
 {
   _curid--;
   while ( _curid>=_minid && !_set.get(_curid) ) _curid--;
 }

//============================== EOF ================================
