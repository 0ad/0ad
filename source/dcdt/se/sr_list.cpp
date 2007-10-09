#include "precompiled.h"
# include "sr_list.h"

//============================ SrListBase =================================

SrListBase::SrListBase ( SrClassManagerBase* m )
 {
   _first = 0;
   _cur = 0;
   _man = m;
   _man->ref();
   _elements = 0;
 } 

SrListBase::SrListBase ( const SrListBase& l )
 {
   _first = 0;
   _cur = 0;
   _man = l._man;
   _man->ref();
   _elements = 0;
   insert_list ( l );
 }

SrListBase::SrListBase ( SrListNode* n, SrClassManagerBase* m )
 {
   _first = _cur = n;
   _man = m;
   _man->ref();
   _elements = 0;
 }

SrListBase::~SrListBase ()
 {
   init ();
   _man->unref();
 } 

void SrListBase::init ()
 {
   while (_cur) remove ();
 }

void SrListBase::elements ( int e )
 {
   if ( e>=0 ) { _elements=e; return; }

   _elements = 0;
   if ( !_first ) return;
   _cur = _first;
   do { _elements++; 
        _cur = _cur->next(); 
      } while ( _cur!=_first );
 }

void SrListBase::insert_next ( SrListNode *n ) 
 {
   if (!n) return;

   _elements++;
   if ( _cur )
    { _cur->insert_next(n);
      _cur = n;
    }
   else 
    { n->init(); _first = _cur = n; }
 }

SrListNode* SrListBase::insert_next ()
 {
   insert_next ( (SrListNode*)_man->alloc() );
   return _cur;
 }

void SrListBase::insert_prior ( SrListNode* n )
 {
   if (!n) return;

   _elements++;
   if ( _cur )
    { _cur->insert_prior(n);
      _cur = n;
    }
   else 
    { n->init(); _first = _cur = n; }
 }

SrListNode* SrListBase::insert_prior ()
 {
   insert_prior ( (SrListNode*)_man->alloc() );
   return _cur;
 }

void SrListBase::insert_list ( const SrListNode* l )
 { 
   if ( !l ) return;
 
   SrListBase nl (_man);
   const SrListNode* lcur = l;

   do { nl.insert_next ( (SrListNode*)_man->alloc(lcur) );
        lcur = lcur->next();
      } while ( lcur!=l );

   if ( _cur )
    _cur->splice ( nl.last() ); // join the two lists
   else
    _cur = _first = nl._cur->next();

   nl.leave_data(); // so that elements are not deleted by nl destructor

   _elements += nl._elements;
 }

SrListNode *SrListBase::replace ( SrListNode *n )
 {
   if (!_cur) return 0;
   if ( _first==_cur ) _first=n;
   SrListNode *ret = _cur->replace(n);
   _cur = n;
   return ret;
 }

SrListNode *SrListBase::extract ()
 {
   if (!_cur) return 0;

   SrListNode *origcur = _cur;
   _elements--;

   if ( _first->next()!=_first )
    { _cur = _cur->remove()->next();
      if ( _first==origcur ) _first=_cur;
    }
   else _first = _cur = 0;

   return origcur;
 }

void SrListBase::remove ()
 {
   void* pt = extract();
   if ( pt ) _man->free(pt);
 }

void SrListBase::sort ()
 {
   _cur = _first;
   if ( !_cur || _cur->next()==_cur ) return;

   SrListNode *pos, *min;
   while ( _cur->next() != _first )
    { min = pos = _cur;

      do { _cur = _cur->next();
           if ( _man->compare(_cur,min)<0 ) min=_cur;
         } while ( _cur!=_first->prior() );

      if (min!=pos)
       { if (pos==_first) _first=min;
         pos->swap(min);
       }

      _cur = min->next();
    }

   _cur = _first;
 }

int SrListBase::insort ( SrListNode *n )
 {
   int c=1;

   if ( _cur )
    { _cur = _first;
      do { c = _man->compare ( n, _cur );
           if ( c<=0 ) break;
           _cur = _cur->next();
         } while ( _cur!=_first );
    }

   if ( _cur==_first && c<=0 ) _first=n;

   insert_prior(n);

   return c;
 }

bool SrListBase::search ( const SrListNode *n )
 {
   _cur = _first;
   if ( !_cur || !n ) return false;
   int c;

   do { c = _man->compare ( n, _cur );
        if ( c==0 ) return true;
        if ( c<0 ) return false;
        _cur = _cur->next();
      } while ( _cur!=_first );

   return false;
 }

bool SrListBase::search_all ( const SrListNode *n )
 {
   _cur = _first;
   if ( !_cur || !n ) return false;

   do { if ( _man->compare(n,_cur)==0 ) return true;
        _cur = _cur->next();
      } while ( _cur!=_first );

   return false;
 }

void SrListBase::take_data ( SrListBase& l )
 {
   if ( this == &l ) return;
   init ();
   _first    = l._first;    l._first    = 0;
   _cur      = l._cur;      l._cur      = 0;
   _elements = l._elements; l._elements = 0;
 }

void SrListBase::take_data ( SrListNode* n )
 {
   if ( _cur==n || _first==n ) return;
   init ();
   _first = _cur = n;
 }

SrListNode* SrListBase::leave_data ()
 {
   SrListNode* n = _first;
   _first = _cur = 0;
   _elements = 0;
   return n;
 }

SrOutput& operator<< ( SrOutput& o, const SrListBase& l )
 { 
   o<<'[';
   SrListIteratorBase it(l);
   for ( it.first(); it.inrange(); it.next() )
    { l._man->output ( o, it.get() );
      if ( !it.inlast() ) o << ' ';
    }
   return o<<']';
 }

//=========================== SrListIteratorBase ================================

SrListIteratorBase::SrListIteratorBase ( const SrListBase& l )
 { 
   _node = 0;
   _list = &l;
   reset ();
 }

SrListIteratorBase::SrListIteratorBase ( SrListNode* n )
 { 
   _list = 0;
   _node = n;
   reset ();
 }

void SrListIteratorBase::reset ()
 {
   if ( _list )
    { _cur = _list->cur();
      _first = _list->first();
      _last = _list->last();
    }
   else
    { _cur = _first = _last = _node;
      if ( _first ) _last = _first->prior();
    }
   _rcode = 0;
 }

bool SrListIteratorBase::inrange ()
 {
   if ( _rcode==3 || !_first ) return false;

   if ( _rcode==1 ) { _rcode=2; if ( _first==_last ) _rcode=3; }
    else
   if ( _rcode==2 && (inlast()||infirst()) ) _rcode=3;

   return true;
 }

//============================== end of file ===============================
