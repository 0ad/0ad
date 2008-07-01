
# ifndef SR_OUTPUT_H
# define SR_OUTPUT_H

/** \file sr_output.h 
 * Output to file, function, string or stdout */

# include <stdio.h>
# include "sr.h"

class SrString;

/*! \class SrOutput sr_output.h
    \brief Generic output class

    Outputs to a file, function, string or stdout. SrOutput has all basic
    functionalities of ostream, but with the ability to easily redirect the
    output and using the old C standard library ouput functions. */
class SrOutput
 { public :
    /*! The type of the ouput, that can be connected to a file, function, string or stdout. */
    enum Type { TypeConsole, //!< Output is directed to the console
                TypeFile,    //!< Output is directed to a file
                TypeString,  //!< Output is directed to a SrString
                TypeFunction //!< Output is directed to a function
              };

   private :
    Type _type;
    int  _margin;
    char _margin_char;
    union { SrString *string;
            void (*func) (const char *,void*);
            FILE *file;
          } _device;

    void* _func_udata;  // used only when the output is a function
    const char* _intfmt;      // format used to print integers
    const char* _floatfmt;    // format used to print floats
    const char* _doublefmt;   // format used to print doubles
    char* _filename;    // optional file name of the open file
    void _init ();

   public : 
    /*! Default constructor: inits the output to the console. */
    SrOutput ();

    /*! Constructor that inits the output to a file. */
    SrOutput ( FILE *f );

    /*! Constructor to a file, which is open with fopen(filename,mode).
        The filename is stored. */
    SrOutput ( const char* filename, const char* mode );

    /*! Constructor that inits the output to a SrString. */
    SrOutput ( SrString &s );

    /*! Constructor that inits the output to a function. The function receives
        the string to output and a void pointer to use as user data. */
    SrOutput ( void(*f)(const char*,void*), void* udata );

    /*! The destructor will close the associated output device. In fact,
        only in the case of a file ouput that closing the device is done.
        The destructor is implemented inline by calling the close() method. */
   ~SrOutput ();

    /*! If output is done to a file, do not close it, and redirects current ouput
        to the console. */
    void leave_file ();

    /*! If the output is done to a file, return the FILE pointer associated, otherwise
        will return 0 */
    FILE* filept ();

    /*! Closes current ouput and redirects it to the console. */
    void init ();

    /*! Closes current ouput and redirects it to a file. */
    void init ( FILE *f );

    /*! Closes current ouput and redirects it to a file, which is open with
        fopen(filename,mode). The filename is stored and can be retrieved with filename. */
    void init ( const char* filename, const char* mode );

    /*! Closes current ouput and redirects it to a SrString. */
    void init ( SrString &s );

    /*! Closes current ouput and redirects it to a function. The function
        receives the string to output and a user data void pointer. */
    void init ( void(*f)(const char*,void*), void* udata );

    /*! Returns the type of the output. Implemented inline. */
    Type type () const { return _type; }

    /*! Returns the file name used for opening a file output, or null if not available */
    const char* filename () const { return _filename; }
    
    /*! Associates with the output a file name. The string is stored but not used by SrOutput. */
    void filename ( const char* s ) { sr_string_set(_filename,s); }

    /*! Returns the current margin being used. The number returned represents 
        the number of tabs or spaces to be used as current margin */
    int margin () const { return _margin; }

    /*! Changes the current margin being used. */
    void margin ( int m ) { _margin=m; }

    /*! Set margin character, default is '\t' */
    void margin_char ( char c ) { _margin_char=c; }

    /*! returns the current margin character. */
    char margin_char () const { return _margin_char; }

    /*! Changes the current margin being used by adding parameter inc to the 
        current value. Normally, inc is passed as 1 or -1. Implemented inline. */
    void incmargin ( int inc ) { _margin+=inc; }

    /*! Checks whether the associated output device pointer is valid.
        A console type output will always be valid, returning true. Others output devices
        are considered valid if their access pointers are not zero */
    bool valid ();

    /*! Will close the associated output device if this is the case.
        If SrOutput is associated with a file, and if the file pointer is not zero, then
        fclose() will be called to close this file and SrOutput will have a new null file
        pointer. For other devices, nothing is done. The filename is set to null. */
    void close ();
 
    /*! Sets the format to print numbers in "printf format", e.g "%d", etc. */
    void fmt_int ( const char* s );
    void fmt_float ( const char* s );
    void fmt_double ( const char* s );

    const char* fmt_int () { return _intfmt; }
    const char* fmt_float () { return _floatfmt; }
    const char* fmt_double () { return _doublefmt; }

    /*! Outputs the margin as defined by methods margin() and margin_char(). */
    void outm ();

    /*! Will fflush() when output is a file or console. */
    void flush ();

    /*! Sets the SrString formats intfmt, floatfmt, and doublefmt to their default values,
        that are: "%d", "g", and "g". Format "g" is interpreted as a "%g", but with
        an added ".0" in case the real number is equal to its integer part.
        Format "i" will output real numbers as integer when equal to their integer part */
    void default_formats ();

    /*! Outputs a character to the connected device. No validity checkings are done. */
    void put ( char c );

    /*! Outputs a string to the connected device. No validity checkings are done. */
    void put ( const char *st );

    /*! Outputs a formated string to the connected device. The format string is the same
        as for the printf() function. An internal 256 bytes buffer is used to translate
        the format string. And then put() method is called. */
    void putf ( const char *fmt, ... );

    /*! Outputs a formatted error message and calls exit(1). */
    void fatal_error ( const char *fmt, ... );

    /*! Outputs a formatted warning message, but does not call exit(). */
    void warning ( const char *fmt, ... );

    /*! Outputs a string using the put() method. */
    friend SrOutput& operator<< ( SrOutput& o, const char *st );

    /*! Outputs a SrString using the put() method. */
    friend SrOutput& operator<< ( SrOutput& o, const SrString& st );

    /*! Outputs a character using the put() method. */
    friend SrOutput& operator<< ( SrOutput& o, char c );

    /*! Outputs a srbyte using intfmt with the putf() method. */
    friend SrOutput& operator<< ( SrOutput& o, srbyte c );

    /*! Outputs an int using intfmt with the putf() method. */
    friend SrOutput& operator<< ( SrOutput& o, int i );
	friend SrOutput& operator<< ( SrOutput& o, intptr_t i);

    /*! Outputs a bool as "true" or "false" with the put() method. */
    friend SrOutput& operator<< ( SrOutput& o, bool b );

    /*! Outputs a short int using intfmt with the putf() method. */
    friend SrOutput& operator<< ( SrOutput& o, short s );

    /*! Outputs a sruint using intfmt with the putf() method. */
    friend SrOutput& operator<< ( SrOutput& o, sruint i );

    /*! Outputs a float using floatfmt with the putf() method. */
    friend SrOutput& operator<< ( SrOutput& o, float f );

    /*! Outputs a double using doublefmt with the putf() method. */
    friend SrOutput& operator<< ( SrOutput& o, double d );
 };

/*! Used to output error messages and by the trace functions It can be 
    redirected with init() if needed. */
extern SrOutput sr_out;

//============================== end of file ===============================


# endif  // SR_OUTPUT_H


