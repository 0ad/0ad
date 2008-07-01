
# ifndef SR_INPUT_H
# define SR_INPUT_H

/** \file sr_input.h 
 * parses input file or string */

// sr_array.h cannot be included here because of circular references
# include <stdio.h>
# include "sr_string.h"

/*! Defines which characters are considered as delimiters. */
# define SR_INPUT_DELIMITERS "{}[]();,.=+-/^*%:&|!~<>#?@\\\'"

/*! \class SrInput sr_input.h
    \brief Parses a file or string buffer

    SrInput reads data from a string buffer or from an open file. It can
    be used to read data byte per byte, or by parsing tokens that are 
    recognized as names, delimiters, integers, reals or strings. 
    Comments can be specified to be skiped and error messages are returned. */
class SrInput
 { public :

    /*! Indicates the type of the current input. It can be TypeFile, TypeString,
        or TypeNull. TypeNull indicates that the SrInput is not valid, and need
        to be connected to a file or string to become valid (by calling init()). */
    enum Type { TypeFile,   //!< Input from a file
                TypeString, //!< Input from a string buffer
                TypeNull    //!< Input not initialized, valid() will return false
              };

    /*! Indicates the type of the token returned in get_token() */
    enum TokenType { Name,      //!< A sequence of chars without spaces and delimiters
                     Delimiter, //!< Any delimiter defined in macro SR_INPUT_DELIMITERS
                     Integer,   //!< An int type number
                     Real,      //!< A real type number
                     String,    //!< A seq inside "". Returns content converting c-like escape chars.
                     EndOfFile, //!< End of file encountered
                     Error      //!< There was an error when parsing the last token.
                   };

    /*! Indicates the error occured in the last get operation. See the comments in each
        function to know the types of error that can be generated. */
    enum ErrorType { NoError,         //!< No error occured in the last operation 
                     UndefChar,       //!< Unrecognized char was found
                     UnexpectedToken, //!< Parsed token is of an unexpected type
                     TooBig,          //!< token is bigger than the given buffer
                     OpenString,      //!< EOF found before end of a string
                     InvalidPoint     //!< Misplaced decimal point found in a real number
                   };

   private :

    struct UngetData;
    union { FILE  *f; const char *s; } _cur; // the current input position
    union { int f; const char *s; } _ini;    // the beginning of the buffer
    int        _size;             // the size of the input in bytes
    int        _curline;          // keeps track of the current line
    char       _comment_style;    // 0:none, 'C':C/C++ like, otherwise skip line
    srbyte     _type;             // the enumerator Type
    srbyte     _last_error;       // last error, if any.
    SrString   _last_token;       // buffer with the last token read
    srbyte     _last_token_type;  // buffer with the last token type read
    int        _max_token_size;   // max size allowed for parsed tokens
    char       _lowercase_tokens; // if tokens should be converted to lowercase
    int        _skipped_spaces;   // number of spaces skipped during last get_token
    UngetData* _unget;            // unget char and token information
    char*      _filename;         // optional file name of the open file
    void _init ( char c );
   public : 

    /*! Construct an input of type SrInput::TypeNull.
        SrInput will only be operational when linked to a file or string by calling
        an init() function later. While SrInput is of type Null, the valid() method will
        return false. The parameter com is the comment style for this input and is 0 by
        default. See comment_style() for a description of possible styles. */
    SrInput ( char com=0 ); // init as a null input

    /*! Construct an input of type SrInput::TypeString.
        In this way SrInput will read bytes from the given null-terminated string buffer. 
        If buff is null, SrInput will be initialized as TypeNull, and not as a String type.
        Com is the comment style for this input and is 0 by default. 
        See comment_style() for a description of possible styles. */
    SrInput ( const char *buff, char com=0 ); // init as a string input

    /*! Construct an input of type SrInput::File
        In this way, SrInput will read bytes from the given C-style file stream. If the file pointer
        is null, SrInput will be initialized as TypeNull, and not as a File type. The current 
        position pointed by file is considered to be the start, and so the size is measured
        since this given position. The parameter com is the comment style for this input and 
        is 0 by default. See comment_style() for a description of possible styles */
    SrInput ( FILE *file, char com=0 ); // init as a file input

    /*! Construct an input of type SrInput::File, from a given filename.
        This is similar to the previous constructor, but it receives the fopen() parameters
        directly (file name and mode strings) and stores internally the given file name. */
    SrInput ( const char* filename, const char* mode, char com=0 ); // init as a file input

    /*! Closes the associated file if it is a TypeFile input.
        If it is required to leave the associated input file open, call leave_file() before. */
   ~SrInput ();

    /*! Closes actual input, and init it as TypeString.
        If buff is null, SrInput will be initialized as TypeNull, and not as a String type. 
        The comment style is not changed, but unget data are freed. */
    void init ( const char *buff );

    /*! Closes actual input, and init it as a File type.
        If file is null, SrInput will be initialized as TypeNull, and not as a File type. 
        The comment style is not changed, but unget data are freed. */
    void init ( FILE *file );

    /*! Closes actual input, and init it as a File type, opening given file name in given mode.
        If file could not be open, SrInput is initialized as TypeNull, and not as a File type. 
        Filename is stored, the comment style is not changed, but unget data are freed. */
    void init ( const char* filename, const char* mode );

    /*! Closes actual input and set it as TypeNull.
        If SrInput is of type file, the associated file is closed. In all cases,
        unget data is freed, filename is set to null, the last error is cleaned,
        and size and curline are set to zero. */
    void close ();

    /*! Puts SrInput into TypeNull mode but without closing the current file.
        If SrInput is not of File type, the effect is the same as close(). */
    void leave_file ();

    /*! If the input is done from a file, return the FILE pointer associated, otherwise
        will return 0 */
    FILE* filept ();

    /*! Returns the file name used for opening a file input, or null if not available */
    const char* filename () const { return _filename; }
    
    /*! Associates with the input a file name. The string is stored but not used by SrInput. */
    void filename ( const char* s ) { sr_string_set(_filename,s); }

    /*! Returns true if the input is not of TypeNull type. */
    bool valid () const;

    /*! Returns the type of the SrInput. */
    Type type () const { return (Type) _type; }

    /*! Returns the size of the current input. Size is calculated as the number in bytes from
        the beginning of the input to the end of the input. The beginning is considered to be
        the pointer passed to the constructor or init() method, when it is calculated. */
    int size () const { return _size; }

    /*! Returns the current line of the input. When SrInput is created or initialized, the
        current line is set to 1. Afterwards, the current line is updated internally each
        time a newline char is read. Note that when pos(int) method is used, the current
        line is no more valid. */
    int curline () const { return _curline; }

    /*! Defines the comment style used for all "get methods", with exception to get_byte().
        The style can be set to 0 if no comments are allowed, can be set to 'C' to have
        standard C and C++ nested comments parsed, or otherwise when the specified
        character is encountered the whole line is skiped. */
    void comment_style ( char style ) { _comment_style=style; } // style can be 0,'C','anychar'

    /*! Returns the current comment style being used. */
    char comment_style () const { return _comment_style; }

    /*! Returns true if the current input is pointing after the end of the file. If the
        input source is empty but there is data to unget, the input is not considered finished.*/
    bool finished ();

    /*! Gets all bytes of the input, starting from the current position, and put them 
        in the given buffer. The input will point to its end. If input is of TypeNull, 
        nothing is done. Note: the input is not parsed, just copied, therefore comments
        are also copied. */
    void getall ( SrString& buf );

    /*! Gets all bytes of the current line, i.e. untill a '\n' is read,
        and put them in the given buffer (inlcuding the '\n').
        The input will point to the first byte after the new line character.
        The last character read is returned and will be either '\n' or EOF.
        Uses method get(), and therefore comments are parsed. */
    int getline ( SrString& buf );

    /*! Get current character and advances pointer, will return -1 if end was reached.
        Will read next char from the associated string or file. If SrInput is of
        TypeNull, -1 is always returned. Comments are not considered, but unget
        chars are correctly handled. */
    int getchar (); // comments not handled

    /*! Get current byte and advance, will return 0 if finish is reached.
        This is for ascii inputs where comments are skipped according to
        the current style. */
    int get (); // comments handled

    /*! Puts a byte in the unget stack. */
    void unget ( char c );

    /*! Reads the next n bytes of the current input.
        Note that unget data are considered, but comments are not. */
    void advance ( int n=1 );

    /*! puts the pointer in the beginning of the input.
        Any unget data is cleared. Nothing is done if SrInput is of type Null. */
    void rewind ();

    /*! Offset in bytes from the begining of the input. */
    int pos ();

    /*! Puts the input pointer to the position pos, clearing all unget data.
        After calling it, curline() becomes invalid. Alternatively, method
        set_pos_and_update_cur_line() can be used */
    void pos ( int pos ); 

    /*! Puts the input pointer to the position pos, clears all unget data and
        determines the line number using rewind() and advance(pos). */
    void set_pos_and_update_cur_line ( int pos );

    /*! Reads all the current line (with getchar), stopping one byte after the
        first newline encountered. */
    void skip_line ();     

    /*! Sets the maximum allowed size for parsed tokens. Default is 256. */
    void max_token_size ( int s ) { _max_token_size=s; }

    /*! Returns the current maximum allowed size for parsed tokens. */
    int max_token_size () const { return _max_token_size; }

    /*! Determines if parsed tokens are transformed to lowercase or not. Default is true. */
    void lowercase_tokens ( bool b ) { _lowercase_tokens = b; }

    /*! Retrieves the state if parsed tokens are transformed to lowercase or not. */
    bool lowercase_tokens () const { return _lowercase_tokens? true:false; }

    /*! Returns the number of leading spaces skipped during the 
        last call to get_token() */
    int skipped_spaces () const { return _skipped_spaces; }

    /*! Affects the next call to all methods using get_token(). 
        The token is copied and stored within SrInput, leaving the user buffer untouched. */
    void unget_token ( const char *token, TokenType type );

    /*! Affects the next call to all methods using get_token(). 
        The last token parsed is ungetted, if its type is not EndOfFile */
    void unget_token ();

    /*! Returns true if any data (token or byte) was put in the unget stacks */
    bool has_unget_data () const;

    /*! Puts a token in buf, returning its type, skipping comments, and considering
        unget() and unget_token() data. When EOF is reached, buf[0] is set to 0.
        Names are converted to lower case according to the state set by lowercase_tokens().
        The maximum length permitted for buf is determined by max_token_size(). 
        When SrInput::Error is returned, last_error() will return the description of
        the error occured. 
        Note: get_token() will parse an input like "-3" as having two tokens, a
        delimiter and an integer (use getn() for reading numbers). */
    TokenType get_token ( SrString& buf ); 

    /* Does the same as get_token(SrString&), but the parsed token and token type
       are kept in an internal buffer, to be later retrieved by last_token() and
       last_token_type() */
    TokenType get_token (); 

    /* Returns a reference to the internal buffer containing the last token parsed
       with get_token(void). */
    SrString& last_token() { return _last_token; }

    /* Returns the type of the last token parsed with get_token(void). */
    TokenType last_token_type() const { return (TokenType)_last_token_type; }

    /*! Returns the last error occured, if any. Errors are generated by methods
        using get_token() methods. */
    ErrorType last_error () const { return (ErrorType)_last_error; }

    /*! Returns true if there an error occured, and false otherwise. */
    bool had_error () const { return ((ErrorType)_last_error)==NoError? false:true; }

    /*! Sets the last error to NoError. */
    void reset_error () { _last_error=(srbyte)NoError; }

    /*! This static method will return a describing error message if t is one
        of the following: UndefChar, UnexpectedToken, TooBig, OpenString, or InvalidPoint.
        Otherwise it will return 0 : */
    static const char* error_desc ( ErrorType t );

    /*! Gets the next token of type string or name using get_token(void),
        and returns the token, which is kept in the internal last_token() string.
        If the returned string has length 0 (ie equal to ""), it means that
        the parsed token is not a string, neithert a name, or EOF was reached.
        In any of these cases, last_error() will return UnexpectedToken. */
    SrString& gets ();

    /*! Same as gets(void), but without using the internal buffer of last_token(),
        the given buffer buf is used instead. A reference to buf is retorned. */
    SrString& gets ( SrString& buf );

    /*! Gets the next token, considering it is a number (type int or real)
        preceeded or not by delimiters + or -, and returns the token, which
        is kept in the internal last_token() string.
        The type in last_token_type() will tell if it was an integer or real.
        If the returned string has length 0 (ie equal to ""), it means that
        the parsed token is not a number or EOF was reached.
        In any of these cases, last_error() will return UnexpectedToken. */
    SrString& getn ();

    /*! Reads the next token using get_token(), and tests if it is a delimiter.
        Returns the delimiter char if a delimiter was found, otherwise returns 0.
        When 0 is returned, last_error() will be set to UnexpectedToken. */
    char getd ();

    /*! Skips next n tokens, by calling get_token(). n has a default value of 1.
        False is returned if any error occurred or EOF is encountered.
        Otherwise true is returned. */
    bool skip ( int n=1 );

    /*! Reads tokens using get_token() until the given name is read as type Name. 
        skipto() uses the sr_compare() function that is case-insensitive.
        Returns false if EOF was reached before, otherwise returns true. */
    bool skipto ( const char *name );

    /*! Returns true if there is a field like <nnn> in the current position,
        or otherwise false is returned. All parsed tokens are ungetted, so that
        the effect is as the current input position is not changed. */
    bool has_field ();

    /*! Parse the input expecting to encounter a field like <name>, and puts
        field_name in the string name. If unexpected tokens are read, an error occurs
        and false is returned. If EOF is encountered, false is returned. */
    bool read_field ( SrString& name );
 
    /*! Checks if the following tokens in input is </name>, returning true or false. */
    bool close_field ( const SrString& name );

    /*! Reads the input until reading </name>. If the field is not found, false is
        returned. Otherwise true is returned and the input will point after </name>. */
    bool skip_field ( const SrString& name );

    /*! Operator to read an integer using method getn() and function atoi(). 
        Errors can be tracked using last_error(). */
    friend SrInput& operator>> ( SrInput& in, int& i );
    friend SrInput& operator>> ( SrInput& in, intptr_t& i );

    /*! Operator to read an unsigned integer using method getn() and function atoi(). 
        Errors can be tracked using last_error(). */
    friend SrInput& operator>> ( SrInput& in, sruint& i );

    /*! Operator to read an unsigned char using method getn() and function atoi(). 
        Errors can be tracked using last_error(). */
    friend SrInput& operator>> ( SrInput& in, srbyte& c );

    /*! Operator to read a float using method getn() and function atof(). 
        Errors can be tracked using last_error(). */
    friend SrInput& operator>> ( SrInput& in, float& f );

    /*! Operator to read a double using method getn() and function atof(). 
        Errors can be tracked using last_error(). */
    friend SrInput& operator>> ( SrInput& in, double& d );

    /*! Operator to read a string using method gets(). st must point to a buffer
        with enought space to receive the input. The input is read using the 
        internal buffer, and the result is copied to st using simply strcpy().
        Errors can be tracked using last_error(). */
    friend SrInput& operator>> ( SrInput& in, char* st );

    /*! Operator to read a SrString using method gets(SrString&). 
        Errors can be tracked using last_error(). Implemented inline. */
    friend SrInput& operator>> ( SrInput& in, SrString& s ) { in.gets(s); return in; }
 };

//============================== end of file ===============================

# endif  // SR_INPUT_H
