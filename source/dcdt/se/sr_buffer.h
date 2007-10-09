
# ifndef SR_BUFFER_H
# define SR_BUFFER_H

/** \file sr_buffer.h 
 * fast buffer memory management template */

# include "sr.h"
# include "sr_input.h" 
# include "sr_output.h" 

/*! Allocates memory with sizeofx*newsize bytes using the C function realloc. 
    If newsize is zero, buffer is freed and both buffer and size becomes zero.
    After the function call, size has the value of newsize. No effect if 
    size==newsize. */
void* sr_buffer_size ( void* buffer, int sizeofx, int& size, int newsize );

/*! Inserts dp*sizeofx bytes at i position, moving correctly the buffer contents. */
void* sr_buffer_insert ( void* buffer, int sizeofx, int& size, int i, int dp );

/*! Inserts dp*sizeofx bytes at i position, moving correctly the buffer contents. */
void* sr_buffer_remove ( void* buffer, int sizeofx, int& size, int i, int dp );

/*! Put buffers with same size and copy them. No effect if buffers pointers are equal. */
void* sr_buffer_copy ( void* buffer, int sizeofx, int& size, const void* buffertocp, int sizetocp );

/*! \class SrBuffer sr_buffer.h
    \brief fast buffer memory management template

    All memory management functions of SrBuffer are written using the four buffer
    functions available in this header. Be aware that constructors and destructors
    of class X are not called. SrBuffer can thus be used only with classes or structs
    that do not have any internal allocated data.
    Internally, SrBuffer keeps only a pointer to the allocated
    buffer memory and the buffer size. For a more featured class see SrArray. */
template <class X>
class SrBuffer
 { private:
    void* _data;
    int   _size;
   public:

    /*! Default constructor. */
    SrBuffer () : _data(0), _size(0) {}

    /*! Copy constructor. */
    SrBuffer ( const SrBuffer& b ) : _data(0), _size(0)
     { _data=sr_buffer_copy(_data,sizeof(X),_size,b._data,b._size); }

    /*! Constructor with a given size. */
    SrBuffer ( int s ) : _data(0), _size(0) { _data=sr_buffer_size(_data,sizeof(X),_size,s); }

    /*! Constructor from a user allocated buffer. See also leave_data(). */
    SrBuffer ( X* pt, int s ) : _data(pt), _size(s) {}

    /*! Destructor frees the buffer. Elements' destructors are not called ! */
   ~SrBuffer () { sr_buffer_size ( _data, sizeof(X), _size, 0 ); }

    /*! Returns true if size()==0; and false otherwise. */
    bool empty () const { return _size==0? true:false; }

    /*! Returns the current size of the Buffer. */
    int size () const { return _size; }

    /*! Allows to change the size of the buffer. */
    void size ( int ns ) 
     { _data=sr_buffer_size(_data,sizeof(X),_size,ns); }

    /*! Sets all elements as x, copying each element using operator = */
    void setall ( const X& x )
     { for ( int i=0; i<_size; i++ ) ((X*)_data)[i]=x; }

    /*! Get a const reference to the element of index i. Indices start from 0
        and must be smaller than size(). No checkings are done to ensure that 
        i is in a valid range. */
    const X& const_get ( int i ) const { return ((X*)_data)[i]; }

    /*! Get a reference to the element of index i. Indices start from 0
        and must be smaller than size(). No checkings are done to ensure that 
        i is in a valid range. */
    X& get ( int i ) { return ((X*)_data)[i]; }

    /*! Sets an element. Operator = is used here. Indices start from 0 and must 
        be smaller than size(). No checkings are done to ensure that i is valid. */
    void set ( int i, const X& x ) { ((X*)_data)[i]=x; }

    /*! Operator version of X& get(int i), but returning a non const reference.
        No checkings are done to ensure that i is valid. */
    X& operator[] ( int i ) { return ((X*)_data)[i]; }

    /*! Returns a const pointer of the internal buffer. The internal buffer 
        will always contain a contigous storage space of size() elements. 
        See also take_data() and leave_data() methods. */
    operator const X* () const { return (X*)_data; }

    /*! Returns a reference to the last element, ie, with index size()-1.
        The Buffer must not be empty when calling this method. */
    X& top () { return ((X*)_data)[_size-1]; }

    /* Reduces the size of the buffer by one. */
    void pop () { _data=sr_buffer_size(_data,sizeof(X),_size,_size-1); }

    /* Increases the size of the buffer by one. */
    void push ( int dp=1 ) { _data=sr_buffer_size(_data,sizeof(X),_size,_size+1); }

    /*! Inserts dp positions, starting at pos i, moving all data correctly. 
        Parameter i can be between 0 and size(), if i==size(), dp positions are
        appended. */
    void insert ( int i, int dp=1 ) { _data=sr_buffer_insert(_data,sizeof(X),_size,i,dp); }

    /*! Removes dp positions starting from pos i, moving all data correctly;
        dp has a default value of 1. Attention: elements' destructors are not
        called ! */
    void remove ( int i, int dp=1 ) { _data=sr_buffer_remove(_data,sizeof(X),_size,i,dp); }

    /*! Copies all internal data of a to SrBuffer, with fast memcpy() functions,
        so that the operator=() that X might have is not used. This method has no
        effect if a "self copy" is called. */
    void operator = ( const SrBuffer<X>& b )
     { _data=sr_buffer_copy(_data,sizeof(X),_size,b._data,b._size); }
    
    /*! Makes the given xpt pointer to point to the internal buffer, without
        reallocation; xpt will then be null or contain the address of the memory
        used and that was allocated with malloc(). The user will then be responsible
        to free this allocated memory with free(). After this call, SrBuffer will
        become an empty but valid buffer. */
    void leave_data ( X*& xpt, int& size )
     { xpt=_data; size=_size; _data=0; _size=0; }

    /*! Frees the data of SrBuffer, and then makes SrBuffer be the given buffer b.
        After this, b is set to be a valid empty buffer. The data is moved without
        reallocation. */
    void take_data ( SrBuffer<X>& b ) { size(0); b.leave_data(_data,_size); }

    /*! Outputs all elements of the Buffer. Element type X must have its ouput operator <<
        available. This is the only method of SrBuffer that is not inline. */
    friend SrOutput& operator<< ( SrOutput& o, const SrBuffer<X>& b )
     { o << '[';
       int i;
       for ( i=0; i<b.size()-1; i++ ) o << b.const_get(i) << srspc;
       if ( i<b.size() ) o << b.const_get(i);
       return o << ']';
     }

    /*! Inputs all elements of the buffer. Element type X must have its input operator <<
        available. */
    friend SrInput& operator>> ( SrInput& in, SrBuffer<X>& b )
     { int s=0;
       b.size(128);
       in.get_token();

       while (true)
        { in.get_token();
          if ( in.last_token()[0]==']' ) break;
          in.unget_token();
          in >> b[s];
          s++;
          if ( s>=b.size() ) b.size ( b.size()+128 );
        }

       b.size ( s );

       return in;
     }
 };

//============================== end of file ===============================

#endif // SR_BUFFER_H

