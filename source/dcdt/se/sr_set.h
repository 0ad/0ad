# ifndef SR_SET_H
# define SR_SET_H

/** \file sr_set.h 
 * indexed set of pointers */

# include "sr_array.h"
# include "sr_class_manager.h" 

/*! The simplest way to use a 'Set' by means of void pointers.
    This is a low level management class and there is no
    destructor to delete stored data. The user is completely
    responsible of the data allocation and deletion.
    Here is an example of how to delete all data in the set:
      for ( int id=0; id<=set.maxid(); id++ )
        delete ((UserClassTypeCast*)set.get(id)); // ok with null pointers
      set.init_arrays(); */
class SrSetBasic
 { protected :
    SrArray<void*> _data;   // data list
    SrArray<int> _freepos;  // keeps freed positions in the array
    
   public :
    /*! Will set the sizes of the internal arrays to 0. This is
        to be called only in case it is guaranteed that all referenced
        objects are already deleted. Use with care! */
    void init_arrays () { _data.size(0); _freepos.size(0); }

    /*! Inserts pt in one empty position and return its index. If a class
        manager is used (by means of SrSetBase), pt must be an allocated 
        pointer of the same element being managed by the class manager. */
    int insert ( void* pt );

    /*! Removes and returns the element stored at position i. */
    void* extract ( int i );

    /*! Compress internal arrays, removing possible nonused positions. */
    void compress ();

    /*! Removes all internal gaps changing current existing indices.
        Old indices can be updated with their new values, which are
        stored in the given Array newindices in the following way:
        newindices[oldindex] == newindex */
    bool remove_gaps ( SrArray<int>& newindices );

    /*! Returns the number of elements in the Set */
    int elements () const { return _data.size()-_freepos.size(); }

    /*! Returns true if the number of elements is 0, or false otherwise */
    bool empty() const { return elements()==0? true:false; }

    /*! maxid() returns the maximum existing id or -1 if the set is empty */
    int maxid () const { return _data.size()-1; }

    /*! Returns the element pointer stored at position i.
        It will return null for previously deleted positions; and
        it is the user responsability to ensure that i is not out of range. */
    void* get ( int i ) const { return _data[i]; }

    /*! Const version of get() */
    const void* const_get ( int i ) const { return _data[i]; }
 };
 
/*! \class SrSetBase sr_set.h
    \brief Base class for SrSet

    SrSet saves in an array, pointers for user data, which can be later
    retrieved with indices. Indices are unique and are not changed
    during insertions and removals. Indices are always >=0.
    As no rearrangements are performed when removals are done,
    internal free positions (gaps) are created. These gaps can be
    removed, but then indices to access data will change. */
class SrSetBase : public SrSetBasic
 { private:
    SrClassManagerBase* _man;
   public:

    /*! Creates a new Set sharing the given class manager */
    SrSetBase ( SrClassManagerBase* m );

    /*! Creates a new Set containing a copy of all elements in s, and
        sharing the same class manager */
    SrSetBase ( const SrSetBase& s );

    /*! Destructor deletes all data internally referenced */
   ~SrSetBase ();

    /*! Deletes all data internally referenced, and make an empty Set */
    void init ();

    /*! Init Set and make it become a copy of s, with the same indices
        and possible internal gaps. */
    void copy ( const SrSetBase& s );

    /*! Allocates one element using the associated class manager, insert
        it in one empty position and return its index. */
    int insert () { return SrSetBasic::insert(_man->alloc()); }

    /*! Deletes and removes the element stored at position i. */
    void remove ( int i );

    friend SrOutput& operator<< ( SrOutput& out, const SrSetBase& s );

    friend SrInput& operator>> ( SrInput& inp, SrSetBase& s );
 };

template <class X>
class SrSet : public SrSetBase
 { public:
    SrSet () : SrSetBase ( new SrClassManager<X> ) {}
    SrSet ( SrClassManagerBase* m ) : SrSetBase ( m ) {}
    SrSet ( const SrSet& s ) : SrSetBase ( s ) {}
   ~SrSet () {}
    int insert ( X* x ) { return SrSetBasic::insert( (void*)x ); }
    int insert () { return SrSetBase::insert(); }
    X* extract ( int i ) { return (X*)SrSetBase::extract(i); }
    X* get ( int i ) const { return (X*)SrSetBase::get(i); }
    const X* const_get ( int i ) const { return (const X*)SrSetBase::const_get(i); }
    X* operator[] ( int i ) const { return (X*)SrSetBase::get(i); }
    void operator = ( const SrSet& s )
     { SrArrayBase::copy ( s ); }
 };

/*! Base class for iterating over sets. */
class SrSetIteratorBase
 { private :
    int _maxid;
    int _minid;
    int _curid;
    const SrSetBase& _set;

   public :
    /*! Constructor */
    SrSetIteratorBase ( const SrSetBase& s );

    /*! Returns the current index being pointed by the iterator */
    int curid () const { return _curid; }

    /*! Returns the max index being pointed by the iterator */
    int maxid () const { return _maxid; }

    /*! Returns the min index being pointed by the iterator */
    int minid () const { return _minid; }

    /*! Must be called each time the associate set is changed */
    void reset ();

    /*! Points the iterator to the first element. */
    void first () { _curid=_minid; }

    /*! Points the iterator to the last element. */
    void last () { _curid=_maxid; }

    /*! Advances the current position of the iterator of one position */
    void next ();

    /*! Walk back the current position of the iterator of one position */
    void prior ();

    /*! Returns true if get() points to a valid position */
    bool inrange () { return _curid>=_minid && _curid<=_maxid? true:false; }

    /*! Returns the current element */
    void* get () { return _set.get(_curid); }

    /*! Returns true if the current position is pointing to the last element. */
    bool inlast () const { return _curid==_maxid? true:false; }

    /*! Returns true if the current position is pointing to the first element, 
        or if the list is empty. */
    bool infirst () const { return _curid==_minid? true:false; }
 };

/*! Derives SrSetIteratorBase providing correct type casts for the user type */
template <class X>
class SrSetIterator : public SrSetIteratorBase
 { public :
    SrSetIterator ( const SrSet<X>& s ) : SrSetIteratorBase(s) {}
    X* get () { return (X*)SrSetIteratorBase::get(); }
    X* operator-> () { return (X*)SrSetIteratorBase::get(); }
 };

//============================== end of file ===============================

#endif // SR_SET_H

