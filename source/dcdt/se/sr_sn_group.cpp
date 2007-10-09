#include "precompiled.h"
# include "sr_sn_group.h"

//# define SR_USE_TRACE1  // Const/Dest
# include "sr_trace.h"

//======================================= SrSnGroup ====================================

const char* SrSnGroup::class_name = "Group";

SrSnGroup::SrSnGroup ()
          :SrSn ( SrSn::TypeGroup, SrSnGroup::class_name )
 {
   SR_TRACE1 ( "Constructor" );
   _separator = false;
 }

SrSnGroup::~SrSnGroup ()
 {
   SR_TRACE1 ( "Destructor" );
   remove_all ();
 }

void SrSnGroup::capacity ( int c ) 
 { 
   if ( c<=_children.size() ) return;
   _children.capacity ( c ); 
 }

SrSn* SrSnGroup::get ( int pos ) const
 { 
   if ( _children.size()==0 ) return 0;
   if ( pos<0 || pos>=_children.size() ) return _children.get(_children.size()-1);
   return _children.get(pos); 
 }

int SrSnGroup::search ( SrSn *n ) const
 {
   for ( int i=0; i<_children.size(); i++ )
    { if ( _children[i]==n ) return i; }

   return -1;
 }

void SrSnGroup::add ( SrSn *sn, int pos )
 {
   sn->ref(); // Increment reference counter

   if ( pos<0 || pos>=_children.size() ) // Append
    { _children.push() = sn;
    }
   else // Insert
    { _children.insert(pos) = sn;
    }
 }

SrSn *SrSnGroup::remove ( int pos )
 {
   SR_TRACE3 ( "remove "<<pos );

   SrSn *sn;

   if ( _children.empty() ) // empty
    { return 0;
    }
   else if ( pos<0 || pos>=_children.size() ) // get last child
    { sn = _children.pop();
    }
   else // remove item in the middle
    { sn = _children[pos];
      _children.remove(pos);
    }

   int oldref = sn->getref();
   sn->unref();
   return oldref>1? sn:0;
 }

SrSn *SrSnGroup::remove ( SrSn *n )
 {
   int pos = search ( n );
   if ( pos<0 ) return 0;
   return remove ( pos );
 }

SrSn *SrSnGroup::replace ( int pos, SrSn *sn )
 {
   SR_TRACE3 ( "replace "<<pos );

   if ( _children.empty() || pos<0 || pos>=_children.size() ) return 0; // invalid pos

   sn->ref();
   SrSn *old = _children[pos];
   _children[pos] = sn;

   int oldref = old->getref();
   old->unref();
   return oldref>1? old:0;
 }

void SrSnGroup::remove_all ()
 {
   SR_TRACE3 ( "remove_all" );
   while ( !_children.empty() ) _children.pop()->unref();
 }

//======================================= EOF ====================================

