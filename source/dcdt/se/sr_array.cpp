#include "precompiled.h" 
# include <stdlib.h>
# include <string.h>

# include "sr_array.h"

//# define SR_USE_MEM_CONTROL
# include "sr_mem_control.h"

# define DATA(i)    ((char*)_data)+(sizeofx*(i))
# define NEWDATA(i) ((char*)newdata)+(sizeofx*(i))

//================================= methods =======================================================

SrArrayBase::SrArrayBase ( sruint sizeofx, int s, int c ) 
            : _size(s), _capacity(c)
 { 
   if ( _capacity<_size ) _capacity=_size;
   _data = _capacity>0? SR_MALLOC(sizeofx*_capacity) : 0;
 }

SrArrayBase::SrArrayBase ( sruint sizeofx, const SrArrayBase& a ) 
            : _size(a._size), _capacity(a._size)
 { 
   if ( _capacity>0 ) 
    { _data = SR_MALLOC ( sizeofx*_capacity );
      if ( _size>0 ) memcpy ( _data, a._data, sizeofx*_size );
    }
   else _data = 0;
 }

SrArrayBase::SrArrayBase ( void* pt, int s, int c ) 
            : _data(pt), _size(s), _capacity(c)
 {
 }

void SrArrayBase::free_data ()
 { 
   if (_data) SR_FREE(_data); 
 }

void SrArrayBase::size ( unsigned sizeofx, int ns )
 {
   _size = ns;
   if ( _size<0 ) _size=0;

   if ( _size>_capacity )
    { _capacity = _size;
      _data = realloc ( _data, sizeofx*_capacity ); // if _data==0, realloc reacts as malloc.
    }
 }

void SrArrayBase::capacity ( unsigned sizeofx, int nc )
 {
   if ( nc<0 ) nc = 0;
   if ( nc==_capacity ) return;
   if ( nc==0 ) { if(_data)free(_data); _data=0; _capacity=_size=0; return; }

   _capacity = nc;
   if ( _size>_capacity ) _size=_capacity;

   _data = realloc ( _data, sizeofx*_capacity ); // if _data==0, realloc reacts as malloc.
 }

int SrArrayBase::validate ( int index ) const
 {
   if ( index<0 ) index += _size * ( -index/_size + 1 );
   SR_ASSERT ( index>=0 );
   return index%_size;
 }

void SrArrayBase::compress ( sruint sizeofx )
 {
   if ( _size==_capacity ) return;

   if ( !_size ) { SR_FREE ( _data ); _data=0; }
    else _data = SR_REALLOC ( _data, sizeofx*_size );

   _capacity = _size;
 }

void SrArrayBase::remove ( sruint sizeofx, int i, int dp )
 { 
   if ( i<_size-dp ) memmove ( DATA(i), DATA(i+dp), sizeofx*(_size-(i+dp)) );
   _size-=dp;
 }

void SrArrayBase::insert ( sruint sizeofx, int i, int dp )
 { 
   SR_ASSERT ( i>=0 && i<=_size);
   SR_ASSERT ( dp>0 );

   _size += dp;

   if ( _size>_capacity ) 
    { _capacity = _size*2; 
      _data = SR_REALLOC ( _data, sizeofx*_capacity );
    }

   if ( i<_size-dp )
    { memmove ( DATA(i+dp), DATA(i), sizeofx*(_size-dp-i) ); // ok with overlap
    }
 }

void SrArrayBase::move ( sruint sizeofx, int dest, int src, int n )
 { 
   memmove ( DATA(dest), DATA(src), sizeofx*(n) );
 }

void SrArrayBase::sort ( sruint sizeofx, srcompare cmp ) 
 { 
   ::qsort ( _data, (size_t)_size, (size_t)sizeofx, cmp ); 
 }

int SrArrayBase::lsearch ( sruint sizeofx, const void *x, srcompare cmp ) const
 { 
   char *pt = (char*)_data;

   for ( int i=0; i<_size; i++ ) { if ( cmp(pt,x)==0 ) return i; pt+=sizeofx; }

   return -1;
 }

int SrArrayBase::bsearch ( sruint sizeofx, const void *x, srcompare cmp, int *pos ) const
 {
   int comp;
   int i, e, p;

   comp=1; i=0; e=_size; p=(e-i)/2;
   
   while ( comp!=0 && i!=e )
    { comp = cmp ( x, DATA(p) );
      if ( comp<0 ) e = p;
       else if ( comp>0 ) i = p+1;
      p = i + (e-i)/2;
    }

   if (pos) *pos=p;

   return comp==0? p : -1;
 }

int SrArrayBase::insort ( sruint sizeofx, const void *x, srcompare cmp, bool allowdup )
 { 
   int result, pos;
   result = bsearch ( sizeofx, x, cmp, &pos );
   if ( result!=-1 && !allowdup ) return -1;
   insert ( sizeofx, pos, 1 );
   return pos;
 }

void SrArrayBase::copy ( sruint sizeofx, const SrArrayBase& a )
 { 
   if ( _data==a._data ) return;
   if ( _data ) { SR_FREE(_data); _data=0; }
   _capacity = a._capacity;
   _size = a._size;
   if ( _capacity>0  ) 
    { _data = SR_MALLOC ( sizeofx*_capacity );
      if ( _size>0 ) memcpy ( _data, a._data, sizeofx*_size );
    }
 }

void* SrArrayBase::leave_data ()
 {
   void *pt = _data;     
   _data=0;
   _size=0;
   _capacity=0;
   return pt;
 }

void SrArrayBase::take_data ( SrArrayBase& a )
 {
   if ( _data ) SR_FREE ( _data );
   _data     = a._data;     a._data=0;
   _size     = a._size;     a._size=0;
   _capacity = a._capacity; a._capacity=0;
 }

void SrArrayBase::take_data (  void* pt, int s, int c )
 {
   if ( _data ) SR_FREE ( _data );
   _data     = pt;
   _size     = s;
   _capacity = c;
 }

//============================== end of file ===============================
