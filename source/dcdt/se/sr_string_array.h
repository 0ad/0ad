
# ifndef SR_STRING_ARRAY_H
# define SR_STRING_ARRAY_H

/** \file sr_string_array.h 
 * resizeable array of strings */

# include "sr_array.h" 

/*! \class SrStringArray sr_string_array.h
    \brief resizeable array of strings

    SrStringArray implements methods for managing a resizeable array
    of strings. It derives SrArray, rewriting all methods to correctly
    manage the allocation and deallocation of the strings. */
class SrStringArray : private SrArray<char*>
 { public :
    /*! Default constructor */
    SrStringArray ( int s=0, int c=0 );

    /*! Copy constructor */
    SrStringArray ( const SrStringArray& a );

    /*! Destructor */
   ~SrStringArray ();

    /*! Returns true if the array has no elements, and false otherwise. */
    bool empty () const { return SrArray<char*>::empty(); }

    /*! Returns the capacity of the array. */
    int capacity () const { return SrArray<char*>::capacity(); }

    /*! Returns the current size of the array. */
    int size () const { return SrArray<char*>::size(); }

    /*! Changes the size of the array. */
    void size ( int ns );

    /*! Changes the capacity of the array. */
    void capacity ( int nc );

    /*! Makes capacity to be equal to size. */
    void compress ();

    /*! Returns a valid index as if the given index references a circular
        array, ie, it returns index%size() for positive numbers. Negative
        numbers are also correctly mapped. */
    int validate ( int index ) const { return SrArray<char*>::validate(index); }

    /*! Sets all elements to s */
    void setall ( const char* s );

    /*! Sets element i to become a copy of s. Index i must be a valid entry. */
    void set ( int i, const char* s );

    /*! Gets a const pointer to the string index i. If that string was not
        defined it returns a pointer to a static empty string "", so that
        always a valid string is returned. */
    const char* get ( int i ) const;

    /*! Operator version of get() */
    const char* operator[] ( int i ) const { return get(i); }

    /*! Returns a const pointer to the last element or 0 if the array is empty*/
    const char* top () const;

    /*! Pop and frees element size-1 if the array is not empty */
    void pop ();

    /*! Appends one element */
    void push ( const char* s );

    /*! Inserts dp positions, starting at pos i, and seting string s in
        each new position created. */
    void insert ( int i, const char* s, int dp=1 );

    /*! Removes dp positions starting from pos i */
    void remove ( int i, int dp=1 );

    /*! Copy operator */
    void operator = ( const SrStringArray& a );

    /*! Inserts one string, considering the array is sorted. Returns the inserted position, 
        or -1 if duplication occurs and allowdup is false. */
    int insort ( const char* s, bool allowdup=true );

    /*! Sort array */
    void sort ();

    /*! Linear search */
    int lsearch ( const char* s ) const;

    /*! Binary search for sorted arrays. Returns index of the element found, 
        or -1 if not found. If not found and pos is not 0, pos will have the 
        position to insert the element keeping the array sorted. */
    int bsearch ( const char* s, int* pos=0 );

    /*! Validate path to be a valid path and push it to the string array.
        If the path is not valid (eg null), or is already in the array, nothing
        is done and -1 is returned. Case sensitive comparison is used.
        In case of success the position of the added path is returned. */
    int push_path ( const char* path );

    /*! Considers that the string array contains a list of paths and tries to locate
        and open a file by searching in these paths.
        First, if the given filename has an absolute path in it, the method simply
        tries to open it and returns right after. Otherwise:
        - First search in the paths stored in the string array.
          If basedir is given (not null), relative paths in the string array
          are made relative (concatenated) to basedir.
        - Second, try to open filename using only the basedir path (if given).
        - Finally tries to simply open filename.
        Path basedir, if given, has to be a valid path name (with a slash in the end).
        Returns true if the file could be open. In such case, the successfull full file
        name can be found in inp.filename(). False is returned in case of failure.
        Parameter mode is the fopen() mode: "rt", etc.  */
    bool open_file ( SrInput& inp, const char* filename, const char* mode, const char* basedir );

    /*! Frees the data of SrStringArray, and then makes SrStringArray be the
        given array a. After this, a is set to be a valid empty array. The data
        is moved without reallocation. */
    void take_data ( SrStringArray& a );

    /*! Outputs all elements of the array in format ["e0" "e1" ... "en"]. */
    friend SrOutput& operator<< ( SrOutput& o, const SrStringArray& a );

    /*! Inputs elements in format ["e0" "e1" ... "en"]. */
    friend SrInput& operator>> ( SrInput& in, SrStringArray& a );
 };

#endif // SR_STRING_ARRAY_H

//============================== end of file ===============================
