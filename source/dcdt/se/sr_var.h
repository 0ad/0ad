
# ifndef SR_VAR_H
# define SR_VAR_H

/** \file sr_var.h 
 * a multi type variable
 */

# include "sr.h"
# include "sr_array.h"

/*! \class SrVar sr_var.h
    \brief generic type variable

    SrVar keeps generic type and multi dimension variable values. */
class SrVar
 { private :
    union Var { bool b; int i; float f; char* s; };

    SrArray<Var> _data; // keeps elements
    char* _name;        // contains name, if used
    char _type;         // type 'b', 'i', 'f', or 's'

   public :    

    /*! Constructs an empty SrVar as type 'i' and name "" */
    SrVar ();

    /*! Constructs an empty SrVar as the given type and name "".
        If the given type is unknown, type 'i' is set. */
    SrVar ( char type );

    /*! Constructs an empty SrVar as the given type and name.
        If the given type is unknown, type 'i' is set. */
    SrVar ( const char* name, char type );

    /*! Constructs a type 'b' SrVar with the given name and value */
    SrVar ( const char* name, bool value );

    /*! Constructs a type 'i' SrVar with the given name and value */
    SrVar ( const char* name, int value );

    /*! Constructs a type 'f' SrVar with the given name and float value */
    SrVar ( const char* name, float value );

    /*! Constructs a type 'f' SrVar with the given name and double value */
    SrVar ( const char* name, double value ) { SrVar::SrVar ( name, float(value) ); }

    /*! Constructs a type 's' SrVar with the given name and value */
    SrVar ( const char* name, const char* value );

    /*! Copy constructor */
    SrVar ( const SrVar& v );

    /*! Deletes all used internal data */
    ~SrVar ();

    /*! Sets a name to be kept by SrVar. Null can be passed to clear the name. */
    void name ( const char* n );

    /*! Returns the associated name. "" is returned if no name was set. */
    const char* name () const;

    /*! Inits SrVar with the given type. Types are specified by the first
        letter of: bool, int, float, or string.
        This method will not free extra spaces existing in internal arrays.
        For such purpose, compress() should be used.
        SrVar values can then be defined with one of the set methods.
        Note: if a non recognized type is given SrVar is set to type 'i'. */
    void init ( char type );

    /*! Makes SrVar be identical to v, same as operator= */
    void init ( const SrVar& v );

    /*! Frees any extra spaces existing in internal arrays */
    void compress () { _data.compress(); }

    /*! Returns the actual array size of the variable */
    int size () const { return _data.size(); }

    /*! Set a new array size */
    void size ( int ns );

    /*! Returns the letter corresponding to the type of the variable, which
        is one of: 'b', 'i', 'f' or 's' */
    char type () const { return _type; }

    /*! Methods set() are used to update a value or to push a new value in SrVar.
        Parameter index gives the position to update the value and has
        the default value of 0.
        If the index is out of range, the value is pushed after
        the last current value, increasing the array size of SrVar.
        Automatic type casts are performed in case of type mixing */
    void set ( bool b, int index=0 );
    void set ( int i, int index=0 );
    void set ( float f, int index=0 );
    void set ( double d, int index=0 ) { set(float(d),index); }
    void set ( const char* s, int index=0 );

    /*! Methods push() are used to push a new value in SrVar. */
    void push ( bool b );
    void push ( int i );
    void push ( float f );
    void push ( double d ) { push(float(d)); }
    void push ( const char* s );

    /*! Methods get() are used to get a value from SrVar.
        Parameter index gives the position to retrieve the value and has
        the default value of 0.
        If the index is out of range, zero is returned ("" for gets() ) */
    bool getb ( int index=0 ) const;
    int geti ( int index=0 ) const;
    float getf ( int index=0 ) const;
    const char* gets ( int index=0 ) const;

    /*! Starting at position i (included), this method removes n positions.
        Parameter n has default value of 1 */
    void remove ( int i, int n=1 );

    /*! Insert at position i, n positions.
        Parameter n has default value of 1 */
    void insert ( int i, int n=1 );

    /*! Makes SrVar be identical to v, same as init(v) */
    SrVar& operator= ( const SrVar& v );

    /*! Outputs SrVar.
        If SrVar has no name, name "var" is used. If SrVar is empty,
        a 0 value ("" for string type) is used. */
    friend SrOutput& operator<< ( SrOutput& o, const SrVar& v );

    /*! Input SrVar. The type of v will change according to the first element
        found in the input after the '=' sign. For example, the input "point=1.0 2;"
        will set v with name 'point', type float, and values 1.0f and 2.0f */
    friend SrInput& operator>> ( SrInput& in, SrVar& v );

    /*! C style comparison function compares the names of variables,
        returning <0, 0, or >0. */
    friend int sr_compare ( const SrVar* v1, const SrVar* v2 );
 };

//================================ End of File =================================================

# endif  // SR_VAR_H

