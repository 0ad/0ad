#include "precompiled.h"
# include <stdlib.h>
# include "sr_var_table.h"

//====================================== SrVarTable =======================================

SrVarTable::SrVarTable ( const SrVarTable& vt )
 {
   _table.size ( vt.size() );
   int i;
   for ( i=0; i<_table.size(); i++ )
    _table[i] = new SrVar ( vt[i] );

   _name = 0;
   sr_string_set ( _name, vt._name );
 }

SrVarTable::~SrVarTable ()
 {
   delete _name;
   init();
 }

void SrVarTable::name ( const char* n )
 {
   sr_string_set ( _name, n );
 }

const char* SrVarTable::name () const
 {
   return _name? _name:"";
 }

void SrVarTable::init ()
 {
   while ( _table.size()>0 )
    delete _table.pop();
 }

SrVar* SrVarTable::get ( const char* name ) const
 {
   int i = search ( name ); 
   return i<0? 0 : _table[i];
 }

SrVar* SrVarTable::set ( const char* name, int value, int index )
 {
   int i = search ( name ); 
   if ( i<0 ) return 0;
   _table[i]->set(value,index);
   return _table[i];
 }

SrVar* SrVarTable::set ( const char* name, bool value, int index )
 {
   int i = search ( name ); 
   if ( i<0 ) return 0;
   _table[i]->set(value,index);
   return _table[i];
 }

SrVar* SrVarTable::set ( const char* name, float value, int index )
 {
   int i = search ( name ); 
   if ( i<0 ) return 0;
   _table[i]->set(value,index);
   return _table[i];
 }

SrVar* SrVarTable::set ( const char* name, const char* value, int index )
 {
   int i = search ( name ); 
   if ( i<0 ) return 0;
   _table[i]->set(value,index);
   return _table[i];
 }

static int fcmp ( SrVar*const* pt1, SrVar*const* pt2 )
 {
   return sr_compare ( *pt1, *pt2 );
 }

int SrVarTable::search ( const char* name ) const
 {
   SrVar v;
   v.name ( name );
   return _table.bsearch ( &v, fcmp ); 
 }

int SrVarTable::insert ( SrVar* v )
 {
   int pos;
   pos = _table.insort ( v, fcmp, true );
   return pos;
 }

void SrVarTable::remove ( int i )
 {
   if ( i<0 || i>=_table.size() ) return;
   delete _table[i];
   _table.remove ( i );
 }

void SrVarTable::merge ( SrVarTable& vt )
 {
   int i, id;
   
   for ( i=0; i<vt.size(); i++ )
    { id = search(vt.get(i).name());
      if ( id>=0 ) // found with same name
       { get(id).init ( vt.get(i) ); // get the value
         delete vt._table[i];
         vt._table[i] = 0;
       }
    }

   for ( i=0; i<vt.size(); i++ )
    { if ( vt._table[i] )
       _table.push() = vt._table[i];
    }

   _table.sort ( fcmp );
   vt._table.size(0);
 }

SrVar* SrVarTable::extract ( int i )
 {
   if ( i<0 || i>=_table.size() ) return 0;
   SrVar* v = _table[i];
   _table.remove ( i );
   return v;
 }

SrOutput& operator<< ( SrOutput& o, const SrVarTable& vt )
 {
   int i;
   SrString buf;

   if ( vt.name()[0] )
    { buf.make_valid_string ( vt.name() );
      o << buf << srspc;
    }

   o << '[' << srnl;

   for ( i=0; i<vt.size(); i++ ) o << vt[i] << srnl;

   o << ']' << srnl;

   return o;
 }

SrInput& operator>> ( SrInput& in, SrVarTable& vt )
 {
   vt.init ();
   vt.name ( 0 );

   in.get_token(); // name or '['

   if ( in.last_token()[0]!='[' )
    { vt.name ( in.last_token() );
      in.get_token (); // now it should come '['
    }

   while ( true )
    { in.get_token ();
      if ( in.last_token()[0]==']' ) break;
      in.unget_token ();
      vt._table.push () = new SrVar;
      in >> *vt._table.top();
    }

   // sort elements in case data was edited by hand
   vt._table.sort ( fcmp );
   
   return in;
 }


//================================ End of File =================================================


