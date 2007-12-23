#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_hash_table.h"

//================================ hash function =============================

static int hash ( const char* s, int size )
 {
   int h = 0;
   while ( *s )
    { h = 31*h + SR_LOWER(*s);
      s++;
    }
   h = SR_ABS(h);
   h = h%size;
   return h;
 }

/*static int hash ( const char *string, int limit ) // worse
 {
	unsigned int i = 0;
	unsigned int accum = 0;
		
	while( string[ i ] != 0 )	{
			accum = string[ i ] 
				^ ( accum << ( sizeof( char ) * 8 + 1 ) ) 
				^ ( accum >> ( ( sizeof( accum ) - sizeof( char ) ) * 8 - 1 ) );
			i++;
		}

    return (accum % limit);
 }*/

//================================ SrHashTableBase ===============================

SrHashTableBase::SrHashTableBase ( int hsize )
 {
   init ( hsize );
 }

SrHashTableBase::~SrHashTableBase ()
 {
   init ( 0 );
 }

void SrHashTableBase::init ( int hsize )
 {
   if ( hsize==0 && _hash_size==0 ) return; // already initialized to 0
   
   // destroys actual table:
   while ( _table.size() )
    { if ( _table.top().dynamic ) sr_string_set ( _table.top().st, 0 ); // delete
      _table.pop();
    }

   // builds new table:
   _free.size ( 0 );
   _free.capacity ( 0 );
   _last_id = -1;
   _elements = 0;
   _hash_size = hsize;
   _table.capacity ( hsize );
   _table.size ( hsize );
   int i;
   for ( i=0; i<hsize; i++ )
    _set_entry ( i, 0/*st*/, 0/*data*/, 0/*dynamic*/ );
 }

void SrHashTableBase::rehash ( int new_hsize )
 {
   SrArray<Entry> a;
   a.capacity ( _elements );
   a.size ( 0 );
   
   // 1. store current valid elements:
   int i;
   for ( i=0; i<_table.size(); i++ )
    { if ( _table[i].st )
       { a.push() = _table[i];
         _table[i].st=0; // ensures st pointer will not be deleted
       }
    }
    
   // 2. init table with new hash size:
   init ( new_hsize );

   // 3. put back original data in new table:
   for ( i=0; i<a.size(); i++ )
    { _insert ( a[i].st, a[i].data, a[i].dynamic );
    }
 }

int SrHashTableBase::longest_entry () const
 {
   int i, j, len;
   int longest=0;

   for ( i=0; i<_hash_size; i++ )
    {
      if ( _table[i].st==0 ) continue;

      len = 1;
      j=_table[i].next;
      while ( j>=0 )
       { len++;
         j = _table[j].next;
       }

      if ( len>longest ) longest=len;

    }

   return longest;
 }

int SrHashTableBase::lookup_index ( const char *st ) const
 {
   if ( !st ) return -1;
   
   int id = ::hash ( st, _hash_size );

   if ( _table[id].st==0 ) return -1; // empty entry, not found

   while ( true )
    { if ( sr_compare(_table[id].st,st)==0 ) return id; // already there
    
      // else check next colliding entry:
      if ( _table[id].next<0 ) return -1; // no more entries, not found
      id = _table[id].next;
    }
 }

void* SrHashTableBase::lookup ( const char* st ) const
 {
   int id = lookup_index ( st );
   return id<0? 0: _table[id].data;
 }

bool SrHashTableBase::_insert ( const char *st, void* data, char dynamic )
 {
   if ( !st ) { _last_id=-1; return false; }
   if ( _hash_size<=0 ) init ( 256 ); // automatic initialization to avoid problems
   
   int id = ::hash ( st, _hash_size );

   if ( _table[id].st==0 ) // empty entry, just take it
    { _set_entry ( id, st, data, dynamic );
      _elements++;
      _last_id = id;
      return true;
    }

   while ( true )
    { if ( sr_compare(_table[id].st,st)==0 ) // already there
       { _last_id = id;
         return false;
       }

      // else check next colliding entry:
      if ( _table[id].next<0 ) // no more entries, add one:
       { int newid;
         if ( _free.size()>0 )
          { newid = _free.pop();
          }
         else
          { newid = _table.size();
            _table.push();
          }
         _table[id].next = newid;
         _set_entry ( newid, st, data, dynamic );
         _elements++;
         _last_id = newid;
         return true;
       }
      
      id = _table[id].next;
    }
 }

void* SrHashTableBase::remove ( const char *st )
 {
   if ( !st ) { _last_id=-1; return false; }
   
   int id = ::hash ( st, _hash_size );

   if ( _table[id].st==0 ) return 0; // already empty entry

   int priorid=id;
   while ( true )
    { if ( sr_compare(_table[id].st,st)==0 ) // found: remove it
       { 
         void* data = _table[id].data; 
         int next = _table[id].next;
         _clear_entry ( id );
         _elements--;

         // fix links:
         if ( priorid==id ) // removing first entry
          { if ( next>=0 )
             { _table[id] = _table[next];
               _set_entry ( next, 0 /*st*/, 0 /*data*/, 0 /*dynamic*/ );
               _free.push() = next;
             }
            else
             { } // nothing to do
          }
         else // removing entry in the "linked list"
          { _table[priorid].next = next;
            _free.push() = id;
          }
      
         return data;
       }

      priorid = id;
      id = _table[id].next;
    }
 }

void SrHashTableBase::_set_entry ( int id, const char *st, void* data, char dynamic )
 {
   if ( dynamic )
     _table[id].st = sr_string_new ( st ); // can return null if st==0
   else
     _table[id].st = (char*)st;

   _table[id].data = data;
   _table[id].next = -1;
   _table[id].dynamic = dynamic;
 }

void SrHashTableBase::_clear_entry ( int id )
 {
   if ( _table[id].dynamic ) delete _table[id].st;

   _table[id].st = 0;
   _table[id].data = 0;
   _table[id].next = -1;
   _table[id].dynamic = 0;
 }

//============================== end of file ===============================

