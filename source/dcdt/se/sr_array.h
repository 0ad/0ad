
# ifndef SR_ARRAY_H
# define SR_ARRAY_H

/** \file sr_array.h 
 * fast resizeable array template */

# include "sr.h"
# include "sr_input.h" 
# include "sr_output.h" 

/*! \class SrArrayBase sr_array.h
    \brief Fast resizeable array base class

    This class is to be derived, and not to be directly used. See SrArray for
    a user-ready class. All memory management functions of SrArrayBase were
    written using quick memory block functions. In this way, malloc() and free()
    functions are used. So that void pointers are used to refer to user's data.
    Most methods need to know the size of each element that is the parameter 
    sizeofx appearing several times. */
class SrArrayBase
 { protected :

    void* _data;     //!< Array pointer used for storage
    int   _size;     //!< Number of elements being used in array
    int   _capacity; //!< Number of allocated elements (>=size)

   protected :

    /*! Init with the sizeof of each element, size, and capacity. If the given
        capacity is smaller than the size, capacity is set to be equal to size */
    SrArrayBase ( sruint sizeofx, int s, int c );

    /*! Copy constructor. Allocates and copies size elements. */
    SrArrayBase ( sruint sizeofx, const SrArrayBase& a );

    /*! Constructor from a given buffer. No checkings are done, 
        its the user responsability to give consistent parameters. */
    SrArrayBase ( void* pt, int s, int c );

    /*! Will free the internal buffer if needed. The internal size and capacity 
        are not adjusted, so that this method should be called only by the 
        destructor of the derived class, as SrArrayBase has not a destructor. */
    void free_data ();

    /*! Changes the size of the array. Reallocation is done only when the size 
        requested is greater than the current capacity, and in this case,
        capacity becomes equal to the size. */
    void size ( unsigned sizeofx, int ns );

    /*! Changes the capacity of the array. Reallocation is done whenever a new
        capacity is requested. Internal memory is freed in case of 0 capacity.
        The size is always kept inside [0,nc]. Parameter nc is considered 0
        if it is negative. */
    void capacity ( unsigned sizeofx, int nc );

    /*! Returns a valid index as if the given index references a circular
        array, ie, it returns index%size() for positive numbers. Negative
        numbers are also correctly mapped. */
    int validate ( int index ) const;
    
    /*! Makes size==capacity, freeing all extra capacity if any. */
    void compress ( sruint sizeofx );

    /*! Removes positions starting with pos i, and length n, moving all data
        correctly. The parameters must be valid, no checkings are done. No 
        reallocation is done. */
    void remove ( sruint sizeofx, int i, int n );

    /*! Inserts dp positions, starting at pos i, moving all data correctly. 
        Parameter i can be between 0 and size(), if i==size(), n positions
        are appended. If reallocation is needed, the array capacity is 
        reallocated to contain two times the new size (after insertion). */
    void insert ( sruint sizeofx, int i, int n );

    /*! Copies n entries from src position to dest position. Regions are
        allowed to overlap. Uses the C function memmove. */
    void move ( sruint sizeofx, int dest, int src, int n );

    /*! Sorts the array, with the compare function cmp, by calling the
        system function qsort(). */
    void sort ( sruint sizeofx, srcompare cmp );

    /*! Linear search, returns index of the element found, or -1 if not found */
    int lsearch ( sruint sizeofx, const void *x, srcompare cmp ) const;

    /*! Binary search for sorted arrays. Returns index of the element found,
        or -1 if not found. If not found and pos is not null, pos will have
        the position to insert the element keeping the array sorted. Faster 
        than the standard C library function bsearch() for large arrays. */
    int bsearch ( sruint sizeofx, const void *x, srcompare cmp, int *pos ) const;

    /*! Returns the position of the insertion, or -1 if not inserted. The
        space of sizeofx is created and the index of the position is returned,
        but get attention to the fact that the contents of x are not moved to
        the inserted position. In this way, x can be only a key to the data 
        stored in the array. In case of duplication, the insertion is not done
        if parameter allowdup is given as false. To insert the element position,
        insert() method is called. */
    int insort ( sruint sizeofx, const void *x, srcompare cmp, bool allowdup );

    /*! Copy from another array. SrArrayBase will be an exact copy of the given array,
        allocating the same capacity, but copying only size elements. */
    void copy ( sruint sizeofx, const SrArrayBase& a );

    /*! Returns the internal buffer pointer that will be null or contain the address of
        the memory used and that was allocated with malloc(). The user will then be
        responsible to free this allocated memory with free(). After this call, the 
        array becomes an empty valid array. */
    void* leave_data ();

    /*! Takes the data of the given array a, that will become an empty array.
        SrArrayBase will have the same data that a had before. This is done 
        without reallocation. */
    void take_data ( SrArrayBase& a );

    /*! Frees the current data of SrArrayBase, and then makes SrArrayBase to control
        the given buffer pt, with size and capacity as given.  */
    void take_data (  void* pt, int s, int c );
 };

/*! \class SrArray sr_array.h
    \brief Fast resizeable dynamic array

    All memory management functions of SrArray use quick memory block functions 
    and so be aware that constructors and destructors of class X are not called. 
    SrArray can be used only with classes or structs that do not have any internal
    allocated data, as SrArray will not respect them when resizing. Internally, 
    malloc(), realloc() and free() functions are used throught SrArrayBase methods. 
    Note that the array size is automatically reallocated when needed (with a double
    size strategey), and so take care to not reference internal memory of SrArray
    that can be reallocated. For example, the following code is wrong: a.push()=a[x],
    because a[x] referentiates a memory space that can be reallocated by push() */
template <class X>
class SrArray : protected SrArrayBase
 { public:

    /*! Constructs with the given size and capacity. If the given capacity 
        is smaller than the size, capacity is set to be equal to size. */
    SrArray ( int s=0, int c=0 ) : SrArrayBase ( sizeof(X), s, c ) {}

    /*! For compatibility with prior versions we provide a constructor from
        3 ints, the last one is simply not considered. */
    SrArray ( int s, int c, int g ) : SrArrayBase ( sizeof(X), s, c ) {}

    /*! Copy constructor. SrArray will be an exact copy of the given array, 
        but allocating as capacity only the size of a. 
        Attention: the operator= that X might have is not called ! */
    SrArray ( const SrArray& a ) : SrArrayBase ( sizeof(X), a ) {}

    /*! Constructor from a given buffer. No checkings are done, its the user 
        responsability to give consistent parameters. */
    SrArray ( X* pt, int s, int c ) : SrArrayBase ( (void*)pt, s, c ) {}

    /*! Destructor frees the array calling the base class free_data() method.
        Attention: elements' destructors are not called ! */
   ~SrArray () { SrArrayBase::free_data(); }

    /*! Returns true if the array has no elements, ie, size()==0; and false otherwise. */
    bool empty () const { return _size==0? true:false; }

    /*! Returns the capacity of the array. Capacity is used to be able to have a 
        larger storage buffer than the current size used. The method capacity() 
        will always return a value not smaller than size(). */
    int capacity () const { return _capacity; }

    /*! Returns the current size of the array. */
    int size () const { return _size; }

    /*! Changes the size of the array. Reallocation is done only when the size 
        requested is greater than the current capacity, and in this case, capacity
        becomes equal to the size. */
    void size ( int ns ) { SrArrayBase::size(sizeof(X),ns); }

    /*! Changes the capacity of the array. Reallocation is done whenever a new
        capacity is requested. Internal memory is freed in case of 0 capacity.
        The size is always kept inside [0,nc]. Parameter nc is considered 0
        if it is negative. */
    void capacity ( int nc ) { SrArrayBase::capacity(sizeof(X),nc); }

    /*! Defines a minimum capacity to use, ie, sets the capacity to be c
        iff the current capacity is lower than c */
    void ensure_capacity ( int c ) { if ( capacity()<c ) capacity(c); }

    /*! Sets all elements as x, copying each element using operator = */
    void setall ( const X& x )
        { int i; for ( i=0; i<_size; i++ ) ((X*)_data)[i]=x; }

    /*! Makes capacity to be equal to size, freeing all extra capacity if any. */
    void compress () { SrArrayBase::compress ( sizeof(X) ); }

    /*! Returns a valid index as if the given index references a circular
        array, ie, it returns index%size() for positive numbers. Negative
        numbers are also correctly mapped. */
    int validate ( int index ) const { return SrArrayBase::validate(index); }

    /*! Gets a const reference to the element of index i. Indices start from 0 and must 
        be smaller than size(). No checkings are done to ensure that i is valid. */
    const X& const_get ( int i ) const { return ((X*)_data)[i]; }

    /*! Gets a reference to the element of index i. Indices start from 0 and must 
        be smaller than size(). No checkings are done to ensure that i is valid. */
    X& get ( int i ) const { return ((X*)_data)[i]; }

    /*! Sets an element. Operator = is used here. Indices start from 0 and must 
        be smaller than size(). No checkings are done to ensure that i is valid. */
    void set ( int i, const X& x ) { ((X*)_data)[i]=x; }

    /*! Operator version of X& get(int i), but returning a non const reference.
        No checkings are done to ensure that i is valid. */
    //X& operator[] ( int i ) { return ((X*)_data)[i]; }

    /*! Returns a const pointer of the internal buffer. The internal buffer 
        will always contain a contigous storage space of capacity() elements. 
        See also take_data() and leave_data() methods. */
    operator const X* () const { return (X*)_data; }
	operator X* () { return (X*)_data; }

    /*! Returns a reference to the last element, ie, with index size()-1.
        The array must not be empty when calling this method. */
    X& top () { return ((X*)_data)[_size-1]; }

    /*! Returns a reference to the last element, ie, with index size()-1, and
        then reduces the size of the array by one with no reallocation.
        The array must not be empty when calling this method. */
    X& pop () { return ((X*)_data)[--_size]; }

    /*! Method to append positions. If reallocation is needed, capacity is set
        to two times the new size. The first new element appended is returned
        as a reference. */
    X& push () { SrArrayBase::insert(sizeof(X),_size,1); return top(); }

    /*! Pushes one position at the end of the array using the insert() method, and
        then copies the content of x using operator=(). */
    void push ( const X& x ) { SrArrayBase::insert(sizeof(X),_size,1); top()=x; }

    /*! Inserts dp positions, starting at pos i, moving all data correctly. 
        Parameter i can be between 0 and size(), if i==size(), dp positions are
        appended. If reallocation is required, capacity is set to two times the
        new size. The first new element inserted (i) is returned as a reference. 
        The quantity of appended positions (dp) has a default value of 1. */
    X& insert ( int i, int dp=1 ) { SrArrayBase::insert(sizeof(X),i,dp); return ((X*)_data)[i]; }

    /*! Removes dp positions starting from pos i, moving all data correctly;
        dp has a default value of 1. Attention: elements' destructors are not called! */
    void remove ( int i, int dp=1 ) { SrArrayBase::remove(sizeof(X),i,dp); }

    /*! Copies n entries from src position to dest position. Regions are
        allowed to overlap. Uses the C function memmove. */
    void move ( int dest, int src, int n ) { SrArrayBase::move(sizeof(X),dest,src,n); }

    /*! Copies all internal data of a to SrArray, with fast memcpy() functions,
        so that the operator=() that X might have is not used. This method has
        no effect if a "self copy" is called. */
    void operator = ( const SrArray<X>& a )
     { SrArrayBase::copy ( sizeof(X), a ); }

    /*! Revert the order of the elements in the array. Copy operator of X is used. */
    void revert ()
     { int i, max=size()-1, mid=size()/2; X tmp;
       for ( i=0; i<mid; i++ ) { SR_SWAP(get(i),get(max-i)); }
     }

    /*! Inserts the element x in the sorted array, moving all data correctly, and 
        returning the position of the element inserted. When allowdup is false, 
        the element will not be inserted in case of duplication, and in this case, 
        -1 is returned. A compare function int sr_compare(const X*,const X*) is
        required as argument. The method insert() is called to open the required
        space, but the operator=() of X will be used to copy element contents in
        the open position. Parameter allowdup has a default value of true. */
    int insort ( const X& x, SR_COMPARE_FUNC, bool allowdup=true )
     { int pos = SrArrayBase::insort ( sizeof(X), (void*)&x, (srcompare)sr_compare_func, allowdup );
       if ( pos>=0 ) ((X*)_data)[pos]=x;
       return pos;
     }

    /*! Standard library qsort() wrapper call. The compare function is required
        as argument: int sr_compare(const X*,const X*) */
    void sort ( SR_COMPARE_FUNC ) { SrArrayBase::sort ( sizeof(X), (srcompare)sr_compare_func ); }

    /*! Linear search, returns the index of the element found, or -1 if not
        found. A compare function is required as argument:
        int sr_compare(const X*,const X*) */
    int lsearch ( const X& x, SR_COMPARE_FUNC ) const { return SrArrayBase::lsearch ( sizeof(X), (void*)&x, (srcompare)sr_compare_func ); }

    /*! Binary search for sorted arrays. Returns index of the element found, 
        or -1 if not found. If not found and pos is not null, pos will have the 
        position to insert the element keeping the array sorted. Faster than
        the standard C library bsearch() for large arrays. A compare function
        is required as argument: int sr_compare(const X*,const X*) */
    int bsearch ( const X& x, SR_COMPARE_FUNC, int *pos=NULL ) const
     { return SrArrayBase::bsearch ( sizeof(X), (void*)&x, (srcompare)sr_compare_func, pos ); }

    /*! Returns the internal buffer pointer that will be null or contain the address of
        the memory used and that was allocated with malloc(). The user will then be
        responsible to free this allocated memory with free(). After this call, the 
        array becomes an empty valid array. */
    X* leave_data () { return (X*) SrArrayBase::leave_data(); }

    /*! Frees the data of SrArray, and then makes SrArray be the given array a.
        After this, a is set to be a valid empty array. The data is moved without
        reallocation. */
    void take_data ( SrArray<X>& a ) { SrArrayBase::take_data ( (SrArrayBase&)a ); }

    /*! Frees the data of SrArray, and then makes SrArray to control the given 
        buffer pt, with size and capacity as given. Its the user reponsibility to
        pass correct values. Note also that the memory menagement of SrArray is
        done with malloc/realloc/free functions. */
    void take_data (  X* pt, int s, int c ) { SrArrayBase::take_data ( pt, s, c ); }

    /*! Output all elements of the array. Element type X must have its ouput operator <<
        available. The output format is [e0 e1 ... en]. */
    friend SrOutput& operator<< ( SrOutput& o, const SrArray<X>& a )
     { int i;
       o << '[';
       for ( i=0; i<a.size(); i++ )
        { o << a[i];
          if ( i<a.size()-1 ) o<<srspc;
        }
       return o << ']';
     }

    /*! Input all elements of the array. Element type X must have its input operator <<
        available. */
    friend SrInput& operator>> ( SrInput& in, SrArray<X>& a )
     { a.size(0);
       in.get_token();
       while (true)
        { in.get_token();
          if ( in.last_token()[0]==']' ) break;
          in.unget_token();
          a.push();
          in >> a.top();
        }
       return in;
     }
 };

//============================== end of file ===============================

#endif // SR_ARRAY_H


