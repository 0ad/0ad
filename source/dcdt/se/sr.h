 
# ifndef SR_H
# define SR_H

/** \file sr.h 
 * Main header file of the SR toolkit
 *
 \code
 *************************************************************************
 *
 *             SR - Simulation and Representation Toolkit
 *            (also called the small scene graph toolkit)
 *                    Marcelo Kallmann 1995-2004
 *
 *   This toolkit was develloped as support for research on modelling,
 *   computational geometry, animation and robotics.
 *
 *   Main features are classes for:
 *   data structures, math, scene graph, meshes,
 *   dynamic constrained delaunay triangulations and articulated characters.
 *
 *   Support for OpenGL and FLTK libraries is provided in libs srgl and srfl.
 *
 *   Main design goals are to be simple and flexible when linking with other tools.
 *   Design choices:
 *   - Small number of classes, with clear and independent functionality
 *   - Uses old c standard library dependency, for most portability and efficiency
 *   - Simple scene graph nodes for small scenes, this is not a scene library
 *   - Transformation matrices follow column major OpenGL format
 *   - Angle parameters are in radians, but degrees are used in data files
 *   - float types are normally preferred than double types, but both are used
 *   - Multiple inheritance is avoided when possible
 *   - Templates are used mainly as type-casting to generic classes, when possible
 *   - Most geometric classes are 3D; otherwise a letter is used to indicate
 *      the dimension, e.g.: SrVec2, SrMatn, etc
 *
 *************************************************************************
 *
 * Nomenclature conventions/examples :
 *
 * global functions          : sr_time()
 * global variables          : sr_out
 * global defines            : SR_BIT0, SR_USE_TRACE1
 * global consts             : sre, srpi
 * global typedefs           : srbyte, sruint
 * global enums              : srMyEnum
 * global classes            : SrList, SrVec, SrVec2          
 * class public members      : a.size(), a.size(3), v.normalize()
 * class private members     : _num_vertices, _size
 * class enums               : class SrClass { enum Msg { MsgOk, MsgError } };
 * class friends (no prefix) : dist ( const SrVec& x, const SrVec& y )
 *
 * If compiling in windows, make sure the _WIN32 macro is defined.
 *
 * Macros for the library compilation (which are not defined by default):
 *  - SR_DEF_BOOL : If the compiler does not have 'bool true false' as keywords,
 *                  defining this macro will make sr to define them.
 *  - SR_BV_MATH_FLOAT : To change default type to float in sr_bv_math.h
 *
 *************************************************************************
 \endcode */

// ==================================== Types =================================

# ifdef _WIN32
# define SR_TARGET_WINDOWS  //!< Defined if compiled for windows
# else
# define SR_TARGET_LINUX  //!< Defined if not compiled in windows
# endif

# ifdef SR_DEF_BOOL
enum bool { false, true }; //!< for old compilers without bool/true/false keywords
# endif

// The following types should be adjusted according to the used system
typedef void*          srvoidpt; //!< a pointer to a char
typedef char*          srcharpt; //!< a pointer to a char
typedef signed char    srchar;   //!< 1 byte signed int, from -127 to 128
typedef unsigned char  srbyte;   //!< 1 byte unsigned int, from 0 to 255
typedef unsigned short srword;   //!< 2 bytes unsigned int, from 0 to 65,535
typedef short int      srsint;   //!< 2 bytes integer, from -32,768 to 32,767
typedef unsigned int   sruint;   //!< 4 bytes unsigned int, from 0 to 4294967295
typedef signed int     srint;    //!< 4 bytes signed integer, from -2147483648 to 2147483647

/*! Defines a typedef for a generic comparison function in the form: 
    int srcompare ( const void*, const void* ), that is used by data structure classes. */
typedef int (*srcompare) ( const void*, const void* );

/*! Defines a generic comparison function for the type X, to be used as argument
    for template based classes. Same as: int (*sr_compare) (const X*,const X*) */
# define SR_COMPARE_FUNC int (*sr_compare_func) (const X*,const X*)

// ================================ Some Constants ======================

const char srnl  = '\n'; //!< Contains the newline '\n' char
const char srtab = '\t'; //!< Contains the tab '\t' char
const char srspc = ' ';  //!< Contains the space char

const float srtiny   = 1.0E-6f;     //!< 1E-6
const float sre      = 2.71828182f; //!< 2.7182818
const float srpi     = 3.14159265f; //!< 3.1415926
const float srpidiv2 = 1.57079632f; //!< 1.57079632
const float sr2pi    = 6.28318530f; //!< 2*pi
const float srsqrt2  = 1.41421356f; //!< sqrt(2) = 1.4142135
const float srsqrt3  = 1.73205080f; //!< sqrt(3) = 1.7320508
const float srsqrt6  = 2.44948974f; //!< sqrt(6) = 2.4494897

const sruint sruintmax = ((sruint)0)-1; //!< the unsigned int maximum value

/* floats and doubles have precision of 7 and 15 decimal digits */
# define SR_E      2.7182818284590452 //!< 2.71828...
# define SR_PI     3.1415926535897932 //!< 3.141592...
# define SR_PIDIV2 1.5707963267948966 //!< 1.570796...
# define SR_2PI    6.2831853071795864 //!< 2*pi
# define SR_SQRT2  1.4142135623730950 //!< sqrt(2) = 1.4142...
# define SR_SQRT3  1.7320508075688772 //!< sqrt(3) = 1.7320...
# define SR_SQRT6  2.4494897427831780 //!< sqrt(6) = 2.4494...

// ================================= Macros ==================================

/*! \def SR_ASSERT
    The macro SR_ASSERT(exp) is expanded to a code that sends an error message
    to sr_out and exist the application, using sr_out.fatal_error()). */
# define SR_ASSERT(exp) if ( !(exp) ) sr_out.fatal_error("SR_ASSERT failure in %s::%d !\n",__FILE__,__LINE__);

/*! Macro that puts c in lower case if c is a valid upper case letter. */
# define SR_LOWER(c) ( (c)>='A'&&(c)<='Z'? (c)-'A'+'a':(c) )

/*! Macro that puts c in upper case if c is a valid lower case letter. */
# define SR_UPPER(c) ( (c)>='a'&&(c)<='z'? (c)-'a'+'A':(c) )

/*! Macro that returns 0 if a is equal to b, 1 if a>b, and -1 otherwise. */
# define SR_COMPARE(a,b) (a==b)? 0: (a>b)? 1: -1

/*! Macro that swaps the value of a boolean type. */
# define SR_SWAPB(b) b = !b // B from bool

/*! Macro that swaps the values of a and b using three xor logical operations. */
# define SR_SWAPX(a,b) { a^=b; b^=a; a^=b; } // x from xor

/*! Macro that swaps the values of a and b, using tmp as temporary variable. */
# define SR_SWAPT(a,b,tmp) { tmp=a; a=b; b=tmp; }

/*! Macro that swaps the values of a and b, given that a temporary 
    variable named tmp, of the same type as a and b exists. */
# define SR_SWAP(a,b) { tmp=a; a=b; b=tmp; }

/*! Macro that returns a number multiple of gap, but that is greater or equal to size. */
# define SR_SIZE_WITH_GAP(size,gap) ( (gap) * ( ((size)/(gap)) + ((size)%(gap)==0?0:1) ) )

/*! Macro that makes m to be x, if x is greater than m. */
# define SR_UPDMAX(m,x) if((x)>(m)) m=x

/*! Macro that makes m to be x, if x is smaller than m. */
# define SR_UPDMIN(m,x) if((x)<(m)) m=x

/*! Macro that tests if x is inside the interval [i,s]. */
# define SR_BOUNDED(x,i,s) ((i)<=(x) && (x)<=(s))

/*! Macro that returns x clipped by the interval [i,s]. */
# define SR_BOUND(x,i,s) (x)<(i)? (i): (x)>(s)? (s): (x)

/*! Macro that forces a to be positive by negating it if it is negative. */
# define SR_POS(a) if((a)<0) a=-(a)

/*! Macro that forces a to be negative by negating it if it is positive. */
# define SR_NEG(a) if((a)>0) a=-(a)

/*! Macro that returns x, so that x = a(1-t) + bt. */
# define SR_LERP(a,b,t) ((a)*(1-(t))+(b)*(t)) // return x = a(1-t) + bt

/*! Macro that returns t, so that x = a(1-t) + bt. */
# define SR_PARAM(a,b,x) ((x)-(a))/((b)-(a)) // return t : x = a(1-t) + bt

/*! Macro that truncates x, with an int typecast. */
# define SR_TRUNC(x) ( (int) (x) )

/*! Macro that returns x rounded to the nearest integer. */
# define SR_ROUND(x) ( (int) ((x>0)? (x+0.5f):(x-0.5f)) )

/*! Macro that rounds x to the nearest integer, but to be 
    applied only when x is positive. This macro adds 0.5 
    and does an int typecast. */
# define SR_ROUNDPOS(x) ( (int) (x+0.5) )

/*! Macro that returns the lowest integer of x. */
# define SR_FLOOR(x) ( int( ((x)>0)? (x):((x)-1) ) )

/*! Macro that returns the highest integer of x. */
# define SR_CEIL(x) ( int( ((x)>0)? ((x)+1):(x) ) )

/*! Macro that returns the maximum value of the two arguments. */
# define SR_MAX(a,b) ((a)>(b)? (a):(b))

/*! Macro that returns the maximum value of the three arguments. */
# define SR_MAX3(a,b,c) ((a)>(b)? (a>c?(a):(c)):((b)>(c)?(b):(c)))

/*! Macro that returns the minimum value of the two arguments. */
# define SR_MIN(a,b) ((a)<(b)? (a):(b))

/*! Macro that returns the minimum value of the three arguments. */
# define SR_MIN3(a,b,c) ((a)<(b)? ((a)<(c)?(a):(c)):((b)<(c)?(b):(c)))

/*! Macro that returns the absolute value of x. */
# define SR_ABS(x) ((x)>0? (x):-(x))

/*! Macro that returns |a-b|, that is the distance of two points in the line. */
# define SR_DIST(a,b) ( (a)>(b)? ((a)-(b)):((b)-(a)) )

/*! Macro that tests if the distance between a and b is closer or equal to ds. */
# define SR_NEXT(a,b,ds) ( ( (a)>(b)? ((a)-(b)):((b)-(a)) )<=(ds) )

/*! Macro that tests if the distance between a and 0 is closer or equal to ds. */ 
# define SR_NEXTZ(a,eps) ( (a)>-(eps) && (a)<(eps) ) // z from zero

/*! Macro that returns -1 if x is negative, 1 if x is positive and 0 if x is zero. */
# define SR_SIGN(x) ((x)<0)? -1: ((x)>0)? 1: 0

/*! Returns the converted angle, from radians to degrees (float version). */
# define SR_TODEG(r) (180.0f*float(r)/srpi)

/*! Returns the converted angle, from degrees to radians (float version). */
# define SR_TORAD(d) (srpi*float(d)/180.0f)

/*! Returns the converted angle, from radians to degrees (double version). */
# define SR_TODEGd(r) (180.0*double(r)/SR_PI)

/*! Returns the converted angle, from degrees to radians (double version). */
# define SR_TORADd(d) (SR_PI*double(d)/180.0)

// ============================== Math Utilities ===========================

/*! Returns the convertion from radians to degrees (float version). */
float sr_todeg ( float radians );

/*! Returns the convertion from radians to degrees (double version). */
double sr_todeg ( double radians );

/*! Returns the convertion from degrees to radians (float version). */
float sr_torad ( float degrees );

/*! Returns the convertion from degrees to radians (double version). */
double sr_torad ( double degrees );

/*! Returns the integer part of x by using a sequence of type casts (float version). */
float sr_trunc ( float x );

/*! Returns the integer part of x by using a sequence of type casts (double version). */
double sr_trunc ( double x );

/*! Returns the closest integer of x (float version). */
float sr_round ( float x );

/*! Returns the closest integer of x (double version). */
double sr_round ( double x );

/*! Returns the lowest rounded value of x (float version). */
float sr_floor ( float x );

/*! Returns the lowest rounded value of x (double version). */
double sr_floor ( double x );

/*! Returns the highest rounded value of x (float version). */
float sr_ceil ( float x );

/*! Returns the highest rounded value of x (double version). */
double sr_ceil ( double x );

/*! Returns the square root for integer values, with no use of floating point. */
int sr_sqrt ( int x );

/*! sr_fact returns the factorial of x. */
int sr_fact ( int x );

/*! returns "b^e", e must be >=0 */
int sr_pow ( int b, int e );

// ============================= Compare Functions ============================

/*! Case insensitive comparison of strings in the C style
    This function follows the C style of compare functions where 0 is returned if
    s1==s2, <0 if s1<s2, and >0 otherwise. Comparisons are case-insensitive.
    s1 and s2 must be non-null pointers, otherwise unpredictable results will arise. 
    If two strings have the first n characters equal, where one has lenght n, and
    the other has length >n, the smaller one is considered to come first. */
int sr_compare ( const char *s1, const char *s2 );

/*! Case sensitive comparison of strings in the C style */
int sr_compare_cs ( const char *s1, const char *s2 );

/*! Case insensitive compare strings, but compares a maximum of n characters. */
int sr_compare ( const char *s1, const char *s2, int n );

/*! Case sensitive compare strings, but compares a maximum of n characters. */
int sr_compare_cs ( const char *s1, const char *s2, int n );

/*! Compares two integers, returning 0 if they're equal, <0 if i1<i2, and >0 otherwise. */
int sr_compare ( const int *i1, const int *i2 );

/*! Compares two floats, returning 0 if they're equal, <0 if f1<f2, and >0 otherwise. */
int sr_compare ( const float *f1, const float *f2 );

/*! Compares two doubles, returning 0 if they're equal, <0 if d1<d2, and >0 otherwise. */
int sr_compare ( const double *d1, const double *d2 );

// ============================== C String Utilities ============================

/*! Allocates a string with sufficient size to copy 'tocopy' in it.
    The allocation is simply done with operator new, and the allocated
    memory pointer is returned. If tocopy==0, the value 0 is returned. */
char* sr_string_new ( const char* tocopy );

/*! Deletes s, and reallocates s with sufficient size to copy 'tocopy'
    in it. If tocopy==0, s will be a null pointer, and 0 is returned. 
    Otherwise the allocation is simply done with operator new, the 
    allocated memory pointer is returned and s is changed to point to
    this new memory allocated so that the returned value will be the 
    same as s. */
char* sr_string_set ( char*& s, const char *tocopy );
const char* sr_string_set ( const char*& s, const char *tocopy );

/*! Deletes s, and reallocates it with the given size also copying its
    contents to the new allocated position. If size<=0, s is simply
    deleted. If the new size is smaller then the original s length,
    the contents will be truncated. In all cases, the new s is returned
    and will be a valid string, having the ending null char. This
    function is similar to the C standard realloc function, but using
    C++ operators new/delete. */
char* sr_string_realloc ( char*& s, int size );

// ============================== Standard IO ============================

/*! Redirects the C streams stdout and stderr to the text files 
    stdout.txt and stderr.txt in the current folder */
void sr_stdout_to_file ();

// ============================== Bit Operation ============================

/*! Tests if flg has the given bit set. */
# define SR_FLAG_TEST(flg,bit) ((flg)&(bit))

/*! Sets on the given bit of the flag. */
# define SR_FLAG_ON(flg,bit) flg|=bit

/*! Swaps the given bit of the flag. */
# define SR_FLAG_SWAP(flg,bit) flg^=bit

/*! Sets off the given bit of the flag. This requires two instructions, so 
    that SR_FLAG_SWAP is faster. */ 
# define SR_FLAG_OFF(flg,bit) if((flg)&(bit)) flg^=bit

/*! Sets the given bit of the flag to be on or off, according to the boolean value of val. */
# define SR_FLAG_SET(flg,bit,val) flg = (val)? (flg)|(bit) : ((flg)&(bit)? (flg)^(bit):flg)

# define SR_BIT0        1 //!< Same as 1
# define SR_BIT1        2 //!< Same as 2
# define SR_BIT2        4 //!< Same as 4
# define SR_BIT3        8 //!< Same as 8
# define SR_BIT4       16 //!< Same as 16
# define SR_BIT5       32 //!< Same as 32
# define SR_BIT6       64 //!< Same as 64
# define SR_BIT7      128 //!< Same as 128
# define SR_ALLBITS   255 //!< Same as 255
# define SR_NOBITS      0 //!< Same as 0

//============================== end of file ===============================

# endif  // SR_H
