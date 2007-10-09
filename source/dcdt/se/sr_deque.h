
# ifndef SR_DEQUE_H
# define SR_DEQUE_H

/** \file sr_deque.h 
 * double-ended queue */

# include "sr_array.h"

/*! \class SrDeque sr_deque.h
    \brief double-ended queue
    A double-ended queue templete based on SrArray. */
template <class X>
class SrDeque // used by the funnel algorithm
 { private :
    SrArray<X> _array;
    int _base;
    bool _topmode;

   public :
    SrDeque () { _base=0; _topmode=true; }

    int size () { return _array.size()-_base; }
    void init ( int cap ) { _array.ensure_capacity(cap); _array.size(cap/2); _base=_array.size(); }
    void init () { _array.size(_array.capacity()/2); _base=_array.size(); }
    void compress () { _array.remove(0,_base); _base=0; _array.compress(); }

    X& top ( int i ) { return _array[_array.size()-i-1]; }
    X& top () { return _array.top(); }
    X& popt () { return _array.pop(); }
    X& pusht () { return _array.push(); }

    X& bottom ( int i ) { return _array[_base+i]; }
    X& bottom () { return _array[_base]; }
    X& popb () { return _array[_base++]; }
    X& pushb ()
       { if (_base==0) { _base=_array.size(); _array.insert(0,_base); }
         return _array[--_base];
       }

    X& operator[] ( int i ) { return _array[_base+i]; }

   public :
    bool top_mode () const { return _topmode; }
    void top_mode ( bool b ) { _topmode=b; }
    X& get ( int i ) { return _topmode? top(i):bottom(i); }
    X& get () { return _topmode? top():bottom(); }
    X& pop () { return _topmode? popt():popb(); }
    X& push () { return _topmode? pusht():pushb(); }
 };

//============================== end of file ===============================

#endif // SR_DEQUE_H

