
# ifndef SR_ARRAY_PT_H
# define SR_ARRAY_PT_H

/** \file sr_array_pt.h 
 * resizeable array of class pointers */

# include "sr_class_manager.h"
# include "sr_array.h"

/*! \class SrArrayPtBase sr_array_pt.h
    \brief resizeable array of class pointers

    SrArrayPtBase implements methods for managing a resizeable array
    of pointers. The user should however use the template
    class SrArrayPt for an implementation that includes automatic
    type casts for the user types. A manager to the user
    data is required, see sr_class_manager.h */
class SrArrayPtBase : private SrArray<void*>
 { private :
    SrClassManagerBase* _man;

   public :

    /*! Initiates an empty array. The class manager is required. */
    SrArrayPtBase ( SrClassManagerBase* m );

    /*! Copy constructor.  The class manager of a is shared. */
    SrArrayPtBase ( const SrArrayPtBase& a );

    /*! Destructor */
   ~SrArrayPtBase ();

    /*! Returns true if the array has no elements, and false otherwise. */
    bool empty () const { return SrArray<void*>::empty(); }

    /*! Returns the capacity of the array. */
    int capacity () const { return SrArray<void*>::capacity(); }

    /*! Returns the current size of the array. */
    int size () const { return SrArray<void*>::size(); }

    /*! Makes the array empty; equivalent to size(0) */
    void init ();

    /*! Changes the size of the array, filling new objects in the new positions. */
    void size ( int ns );

    /*! Changes the capacity of the array. */
    void capacity ( int nc );

    /*! Makes capacity to be equal to size. */
    void compress ();

    /*! Swaps the pointers of position i and j, that must be valid positions. */
    void swap ( int i, int j );
 
    /*! Returns a valid index as if the given index references a circular
        array, ie, it returns index%size() for positive numbers. Negative
        numbers are also correctly mapped. */
    int validate ( int index ) const { return SrArray<void*>::validate(index); }

    /*! deletes element i and reallocates a new one as a copy of pt */
    void set ( int i, const void* pt );

    /*! returns a pointer to the object in position i. */
    void* get ( int i ) const;

    /*! Returns a const pointer to the object in position i. */
    const void* const_get ( int i ) const;

    /*! Returns a pointer to the last element or 0 if the array is empty*/
    void* top () const;

    /*! Pop and frees element size-1 if the array is not empty */
    void pop ();

    /*! Allocates and appends one empty element */
    void push ();

    /*! Inserts n positions, starting at pos i, and putting a new element in
        each new position created. */
    void insert ( int i, int n=1 );

    /*! Removes n positions starting from pos i */
    void remove ( int i, int n=1 );

    /*! Extract (without deletion) and returns the pointer at position i */
    void* extract ( int i );

    /*! Copy operator */
    void operator = ( const SrArrayPtBase& a );

    /*! Inserts one object, considering the array is sorted. Returns the
        inserted position, or -1 if duplication occurs and allowdup is false.
        (Note: all methods using sr_compare functions are not thread safe, as
         they use a static pointer to set the current comparison function) */
    int insort ( const void* pt, bool allowdup=true );

    /*! Sort array */
    void sort ();

    /*! Linear search. Returns index of the element found, 
        or -1 if not found. */
    int lsearch ( const void* pt ) const;

    /*! Binary search for sorted arrays. Returns index of the element found, 
        or -1 if not found. If not found and pos is not 0, pos will have the 
        position to insert the element keeping the array sorted. */
    int bsearch ( const void* pt, int *pos=NULL );

    /*! Frees all data, and then makes SrArrayPtBase be the given array a.
        After this, a is set to be a valid empty array. The data is moved without
        reallocation. */
    void take_data ( SrArrayPtBase& a );

    /*! Outputs all elements of the array in format is [e0 e1 ... en]. */
    friend SrOutput& operator<< ( SrOutput& o, const SrArrayPtBase& a );

    /*! Inputs elements in format is [e0 e1 ... en]. */
    friend SrInput& operator>> ( SrInput& in, SrArrayPtBase& a );
 };

/*! \class SrArrayPt sr_array_pt.h
    \brief resizeable array of class pointers

    SrArrayPtBase implements methods for managing a resizeable array
    of pointers, which are managed by SrClassManager object. */
template <class X>
class SrArrayPt : public SrArrayPtBase
 { public :
    /*! Default constructor that automatically creates a SrClassManager<X>. */
    SrArrayPt () : SrArrayPtBase ( new SrClassManager<X> ) {}

    /*! Constructor with a given class manager. */
    SrArrayPt ( SrClassManagerBase* m ) : SrArrayPtBase ( m ) {}

    /*! Copy constructor sharing class manager. */
    SrArrayPt ( const SrArrayPt& a ) : SrArrayPtBase ( a ) {}

    void set ( int i, const X& x ) { SrArrayPtBase::set(i,(const void*)&x); }

    X* get ( int i ) { return (X*)SrArrayPtBase::get(i); }
    const X* const_get ( int i ) const { return (const X*)SrArrayPtBase::const_get(i); }

    X* operator[] ( int i ) { return get(i); }

    /*! Returns a pointer to the last element or 0 if the array is empty*/
    X* top () const { return (X*)SrArrayPtBase::top(); }

    /*! Pop and frees element size-1 if the array is not empty */
    void pop () { SrArrayPtBase::pop(); }

    /*! Allocates and appends one empty element */
    void push () { SrArrayPtBase::push(); }

    /*! Allocates and appends one element using copy operator */
    void push ( const X& x ) { push(); *top()=x; }

    /*! Allocates and insert one element using copy operator */
    void insert ( int i, const X& x ) { insert(i,1); *get(i)=x; }

    /*! Extract (without deletion) and returns the pointer at position i */
    X* extract ( int i ) { return (X*) SrArrayPtBase::extract(i); }
 };

//============================== end of file ===============================

#endif // SR_ARRAY_PT_H
