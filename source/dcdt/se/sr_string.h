
/** \file sr_string.h 
 * Dynamic string */

# ifndef SR_STRING_H
# define SR_STRING_H

# include "sr.h" 

/*! \class SrString sr_string.h
    \brief Resizable dynamic string

    SrString keeps a dynamic allocated buffer containing a string. This 
    buffer can have a capacity greater than the current string being stored,
    to speedup some string operations. Whenever some string operation results
    with a string with bigger size than the capacity of the original string,
    the buffer is reallocated. But when the result is smaller, no reallocation
    is done. The method compress() can be used to free the extra internal
    buffer space. Important: when the string is empty, its buffer will point
    to a static empty string (""), so that SrString will always contain a 
    valid string when accessing its internal buffer pointer with the type cast
    operator (const char*). With this type cast defined, all functions having
    a const char* parameter can receive a SrString parameter. */
class SrString
 { private:
    char* _data;         // pointer to the allocated buffer, will never be 0
    int   _capacity;     // capacity of the allocated buffer, can be 0
    static char *_empty; // _data will point to empty instead of having a 0 value
   public:

    /*! Default constructor: creates empty string pointing to SrString::_empty. */
    SrString ();

    /*! Constructor from a character. Constructs a string with len()==len
        and capacity()==len+1, containing the given character inside all the
        string. If len<=0, this constructor has the same effect as the default
        constructor, ie, capacity()==0 and SrString==_empty. */
    SrString ( char c, int len=1 );

    /*! Constructor from a formatted string. Note that the formatted string is 
        composed using a buffer of 256 bytes, so that the generated string
        must not exceed this size in any case. The C-library function 
        sprintf(...) is used, and then, set() is called. */
    SrString ( const char *fmt, ... );
 
    /*! Copy constructor. Copies the given string by calling the member
        function set(). */
    SrString ( const SrString& st );

    /*! Destructs the string by freeing the internal used memory if needed. */
   ~SrString ();

    /*! Obtains a string with length len and filled with c. */
    SrString& set ( char c, int len );

    /*! Sets SrString to contain the given string. If the given string pointer
        is 0, SrString will be set to _empty, having the internal buffer freed.
        Otherwise, the internal buffer is only reallocated when more space is
        needed. Passing an empty string ("") as argument will keep the actual
        capacity of SrString, just setting the first char to 0. */
    SrString& set ( const char *st );

    /*! Sets SrString to contain the given formatted string. Note that the
        formatted string is composed using a buffer of 256 bytes, so that
        the result must not exceed this size in any case. The C-library 
        function sprintf(...) is used and then, set() is called. */
    SrString& setf ( const char *fmt, ... );

    /*! Returns the length of the string by calling strlen() */
    int len () const;

    /*! Ensures the string has a sufficient capacity to hold a string of len l
        (ie, capacity>=l+1), calling the method capacity(l+1) if needed. Then
        a \0 character is placed at position l. */
    void len ( int l );

    /*! Returns the current capacity of the internal buffer being used. */
    int capacity () const;

    /*! Sets the desired capacity for the internal buffer. If c<=0, the 
        internal string is deallocated if needed and then it is pointed
        to _empty. Otherwise, the internal buffer size is adjusted with 
        realloc(c). */
    void capacity ( int c );

    /*! Frees any not used space in the internal buffer. If SrString is empty,
        nothing is done. If the internal buffer points to an empty string 
        different than SrString::_empty, it is freed and MgString becomes empty.
        Otherwise, the internal buffer uses realloc() to achieve the optimal 
        memory case where capacity()==len()+1. */
    void compress ();

    /*! Take out leading and ending spaces of SrString. No reallocation is
        done, so that SrString keeps its capacity(), regardless of its len(). */
    void trim ();

    /*! Take out the leading spaces of SrString. No reallocation is done, so
        that SrString keeps its capacity(), regardless of its len(). */
    void ltrim ();

    /*! Take out ending spaces of SrString. No reallocation is used, so that
       SrString keeps its capacity(), regardless of its len(). */
    void rtrim ();

    /*! Puts in xi and xf the indices of the first and last non-white 
        characters. The first argument will contain the position of the first
        non-white char of the string, or -1 if the string is empty. The second
        argument will contain the position of the last non-white char, or -1 
        if the string is empty. If SrString contains only white-space 
        characters, we will have xi>xf, where xi==len() and xf==len()-1. The
        C function isspace() is used to determine if a character is a white-space
        char or not. */
    void bounds ( int &xi, int &xf ) const;

    /*! Makes SrString to becomes its substring enclosed in the coordinates
        [inf,sup]. If sup<0, sup is considerd to be len()-1, ie, the maximum
        valid coordinate. If sup is greater than this maximum, sup becomes 
        this maximum value. If inf<0, inf is considered to be 0. And if 
        inf>sup, SrString becomes empty; already taking into account all 
        checkings for the sup parameter. If SrString is empty, nothing is done.
        In any case, no reallocation is used, so that SrString keeps its 
        same original capacity(). */
    void substring ( int inf, int sup );

    /*! Returns the last char in the string, or 0 if string is empty. */
    char last_char () const;

    /*! Replaces the last char in the string by c if the string is not empty */
    void last_char ( char c );

   /*! Put in s the substring defined by positions inf and sup.
       If inf>sup or s is empty nothing is done and the method returns.
       If sup is out of range it is set to the end of the string, and
       if inf<0 inf is set to 0. */
    void get_substring ( SrString& s, int inf, int sup ) const;

   /*! Put in s the next sequence of characters (a string) found in SrString.
       The search starts at the position i. In case a string is not found,
       -1 is returned, otherwise the position just after the end of the 
       found string is returned. In this way it is possible to parse efficiently
       all names separated by white spaces. */
    int get_next_string ( SrString& s, int i ) const;

    /*! Changes each character to lower case. */
    void lower ();

    /*! Changes each character to upper case. */
    void upper ();

    /*! Returns the index of the first occurrence of c, or -1 if not found. */
    int search ( char c ) const;

    /*! Returns the index of the first occurrence of str, or -1 if not found,
        or if any of the strings are empty or if the length of st is greater
        than that of SrString.
        If ci is true (the default), the used compare function is case-insensitive. */
    int search ( const char* st, bool ci=true ) const;

    /*! Keeps only the path of a filename. The cut point is determined by
        the last slash character (/ or \) found, which stays in the string.
        Returns the cut point, which can be -1 if no slash is found. */
    int remove_file_name ();

    /*! Removes the path of a filename. The cut point (returned) is determined
        by the last slash character (/ or \) found. -1 is returned if
        no slash is found. */
    int remove_path ();

    /*! Search for a file name at the end of the string and put it in filename.
        The cut point is determined by the last slash character present in the
        string and its position is returned. */
    int get_file_name ( SrString& filename ) const;

    /*! Same as get filename, but SrString is modified to not contain the
        file name. The slash character stays in the string (not in filename).
        -1 is returned if not found, and in this case nothing is changed. */
    int extract_file_name ( SrString& filename );

    /*! Search for an extension at the end of the string and remove it.
        The cut point is the last point character found, and its position
        is returned (or -1 in case of no extension found). */
    int remove_file_extension ();

    /*! Search for an extension at the end of the string and put it in ext.
        The cut point is determined by the last point character present in
        the string and its position is returned (or -1 in case of failure).
        The returned extension does not contain the point character. */
    int get_file_extension ( SrString& ext ) const;

    /*! Search for an extension at the end of the string, put it in ext, and
        erase it from the string. The cut point is the last point character
        found, its position is returned (or -1 in case of failure).
        The point character will be no more present in the string and neither
        in ext. -1 is returned if not found, and in this case nothing is
        changed. */
    int extract_file_extension ( SrString& ext );

    /*! Checks if ".ext" exists at the end of the string (case sensitive).
        If ext is a null pointer, only checks if a '.' exists.
        Note: 'ext' is not supposed to contain the '.' character. */
    bool has_file_extension ( const char* ext ) const;

    /*! Ensures a slash exists at the end, and replaces all '\' with '/'.
        Returns true if a non empty path (>=2 chars) is found and false otherwise. */
    bool make_valid_path ();

    /*! Copies s (s can be a type-casted SrString), ensures a slash exists at
        the end, and replaces all '\' with '/'. Returns true if a non empty path 
        (>=2 chars) is found and false otherwise (false is returned if s is null). */
    bool make_valid_path ( const char* s );

    /*! Returns true if the string is an absolute path and false otherwise.
        It is considered an absolute path if it starts with a slash character,
        or if it starts with a drive letter in the format "X:" */
    static bool has_absolute_path ( const char* s );

    /*! Member version of the has_absolute_path() static method */
    bool has_absolute_path () const { return has_absolute_path(_data); }

    /*! Copy s to SrString, adding to any found double quotes a back slash. If
        s is empty, or double quotes or any delimiters are found, the whole
        string is enclosed with double quotes and true is returned. This ensures
        a valid string name for I/O. If no modifications are required false is
        returned and s is simply copied. 
        Example: [My "book" is <red>] will become ["My \"book\" is <red>"] */
    bool make_valid_string ( const char* s );

    /*! Converts to an integer, same as the C library function */
    int atoi () const;

    /*! Converts to a float, same as the C library function. */
    float atof () const;

    /*! Converts to a double, using the C library atof function. */
    double atod () const;

    /*! Type cast operator to access the internal string buffer.
        The internal buffer is always a non-null pointer, but it can point
        to an empty string, ie, SrString::_empty. Implemented inline. */
    operator const char* () const { return _data; }

    /*! Accesses the character/element number i of the string.
        SrString is indexed as the standard c string, so that valid indices
        are in the range [0,len()-1]. There are no checkings if the index is
        inside the range. Implemented inline. */
    char& operator[] ( int i ) { return _data[i]; }

    /*! Gives a const access of the element number i of the string. SrString
        is indexed as the standard c string, so that valid indices are in the
        range [0,len()-1]. There are no checkings if the index is inside the
        range. Implemented inline. */
    const char& get ( int i ) const { return _data[i]; }
 
    /*! Appends st to the string. If more space is required, the string capacity
        is increased by 2 times the required capacity in order to speed up many
        consecutive string concatenations. Use compress() for reducing the used
        memory later if required. */
    void append ( const char* st );

    /*! Inserts st in the middle of SrString, starting at position i. 
        If i<0, 0 is considered. If reallocation is required, the string capacity
        is increased by 2 times the required capacity. If i>=len(), append() is
        called. If st==0, or st=="", nothing is done. */
    void insert ( int i, const char* st );

    /*! Removes dp characters of the string, starting at position i.
        If SrString is empty, or the indices are out of range, nothing is done.
        If the space to remove goes outside the string, the space is clipped. */
    void remove ( int i, int dp );

    /*! Replaces the first occurence of oldst with newst,
        and returns the position where newst was inserted.
        If oldst was not found, -1 is returned and nothing is done.
        oldst and newst can overlap.
        If ci is true (the default), the used compare function is case-insensitive. */
    int replace ( const char* oldst, const char* newst, bool ci=true );

    /*! Calls the method replace until -1 is returned.
        Returns the number of replacements done. */
    int replace_all ( const char* oldst, const char* newst, bool ci=true );

    /*! Makes SrString be exactly the same object as b, and then makes b an 
        empty string. This is done without reallocation, and is used to speed
        up many algorithms. */
    void take_data ( SrString &s );

    /*! Makes SrString be empty, and assigns the internal buffer to s. Returns
        the string s, that will never be 0, and that will never point to the
        internal SrString::_empty. The returned pointer will need to be deleted
        (with delete) by the user somewhere after. */
    char* leave_data ( char*& s );

    /*! Appends st to the SrString by calling append(). */
    SrString& operator << ( const char* st );
    SrString& operator << ( int i );
    SrString& operator << ( float f );
    SrString& operator << ( double d );
    SrString& operator << ( char c );

    /*! Assignment operator by calling set(). */
    SrString& operator = ( const SrString &s );

    /*! Copies st into SrString by calling set(). */
    SrString& operator = ( const char* st );
    SrString& operator = ( int i );
    SrString& operator = ( char c );
    SrString& operator = ( float f );
    SrString& operator = ( double d );

    /*! Compares strings using sr_compare(const char*,const char*), that does 
        a case-insensitive comparison. Returns 0 if they are equal, <0 if s1<s2,
        and >0 if s1>s2. */
    friend inline int sr_compare ( const SrString* s1, const SrString* s2 ) { return ::sr_compare(s1->_data,s2->_data); }

    // Notice: The compiler requires one class type argument for operators.

    /*! Case-insensitive comparison operator that returns a boolean value. */
    friend inline bool operator == ( const SrString& s1, const char* s2 ) { return sr_compare(s1._data,s2)==0? true:false; }
    friend inline bool operator == ( const char* s1, const SrString& s2 ) { return sr_compare(s1,s2._data)==0? true:false; }
    friend inline bool operator == ( const SrString& s1, const SrString& s2 ) { return sr_compare(s1._data,s2._data)==0? true:false; }

    /*! Case-insensitive comparison operator that returns a boolean value. */
    friend inline bool operator != ( const SrString& s1, const char* s2 ) { return sr_compare(s1._data,s2)!=0? true:false; }
    friend inline bool operator != ( const char* s1, const SrString& s2 ) { return sr_compare(s1,s2._data)!=0? true:false; }
    friend inline bool operator != ( const SrString& s1, const SrString& s2 ) { return sr_compare(s1._data,s2._data)!=0? true:false; }

    /*! Case-insensitive comparison operator that returns a boolean value. */
    friend inline bool operator < ( const SrString& s1, const char* s2 ) { return sr_compare(s1._data,s2)<0? true:false; }
    friend inline bool operator < ( const char* s1, const SrString& s2 ) { return sr_compare(s1,s2._data)<0? true:false; }
    friend inline bool operator < ( const SrString& s1, const SrString& s2 ) { return sr_compare(s1._data,s2._data)<0? true:false; }

    /*! Case-insensitive comparison operator that returns a boolean value. */
    friend inline bool operator <= ( const SrString& s1, const char* s2 ) { return sr_compare(s1._data,s2)<=0? true:false; }
    friend inline bool operator <= ( const char* s1, const SrString& s2 ) { return sr_compare(s1,s2._data)<=0? true:false; }
    friend inline bool operator <= ( const SrString& s1, const SrString& s2 ) { return sr_compare(s1._data,s2._data)<=0? true:false; }

    /*! Case-insensitive comparison operator that returns a boolean value. */
    friend inline bool operator > ( const SrString& s1, const char* s2 ) { return sr_compare(s1._data,s2)>0? true:false; }
    friend inline bool operator > ( const char* s1, const SrString& s2 ) { return sr_compare(s1,s2._data)>0? true:false; }
    friend inline bool operator > ( const SrString& s1, const SrString& s2 ) { return sr_compare(s1._data,s2._data)>0? true:false; }

    /*! Case-insensitive comparison operator that returns a boolean value. */
    friend inline bool operator >= ( const SrString& s1, const char* s2 ) { return sr_compare(s1._data,s2)>=0? true:false; }
    friend inline bool operator >= ( const char* s1, const SrString& s2 ) { return sr_compare(s1,s2._data)>=0? true:false; }
    friend inline bool operator >= ( const SrString& s1, const SrString& s2 ) { return sr_compare(s1._data,s2._data)>=0? true:false; }
 };

//============================== end of file ===============================

# endif  // SR_STRING_H
