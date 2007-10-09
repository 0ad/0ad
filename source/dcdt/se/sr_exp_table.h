
/** \file sr_exp_table.h 
 * Expression Table Evaluation. */

# ifndef SR_EXP_TABLE_H
# define SR_EXP_TABLE_H

# include "sr_array.h"
# include "sr_input.h"
# include "sr_output.h"
# include "sr_shared_class.h"

/*! \class SrExpTableVarManager sr_exp_table.h
    \brief Allows the use of variables in expressions

    This class should be derived and its two virtual methods
    implemented in order to make SrExpTable working with user-
    managed variables. Variables can be indexed like "var[n]". */
class SrExpTableVarManager : public SrSharedClass
 { public :

    /*! virtual destructor will call the destructor of the derived class, if any. */
    virtual ~SrExpTableVarManager () {};

    /*! Translates found user-variables into unique indices to be internally stored.
        Whenever a name is encountered during parsing, it is first checked if it
        corresponds to some internal function, if not, it is considered to be a
        user-variable. If -1 is returned, the variable is not defined and an
        error is generated. */
    virtual int get_variable_index ( const char* var_name )=0;

    /*! Used to retrieve a variable value during evaluation. The
        parameter var_id is the variable index registered at parsing time, and 
        if the parameter array_id is >=0, array_id will contain the array index
        requested for the variable. */
    virtual double get_variable_value ( int var_id, int array_id )=0;
 };


/*! \class SrExpTable sr_exp_table.h
    \brief Expression Table Evaluation

    SrExpTable is used to parse and keep an expression table for later evaluation.
    Recognized operators and their precedence are:    
    (1) !x (logical unary operator that negates a value 0<->1)
    (2) x^y, x%y (x raised to y, x percent of y)
    (3) x*y, x/y, x|y (x multiplied by y, x divided by y, x modulus y)
    (4) x+y, x-y (x plus y, x minus y)
    (5) x>y, x<y, x>=y, x<=y, x==y, x<>y (comparison logical operators)
    (6) x and y, x or y (logical operators)
    Names true and false are converted to 1 and 0.
    All calculations are done using double types.
    Suported functions are: abs, acos, asin, atan, ceil, cos, exp, fact, floor, 
    ln, log, round, sign, sin, sqrt, tan */
class SrExpTable
 { public :

    enum Msg { MsgOk,                  MsgUndefChar,           MsgNameTooBig,        
               MsgStringNotClosed,     MsgMisplacedDecPoint,   MsgUnexpectedEndOfFile, 
               MsgUndefinedName,       MsgUndefBinOp,          MsgUndefUnaOp,
               MsgUndefPreDefFunc,     MsgOperandExpected,     MsgOperatorExpected,
               MsgFuncWithoutPar,      MsgDivideByZero,        MsgRaisedZeroToNeg,
               MsgRaisedNegToReal,     MsgUnmatchedParentesis, MsgAcosArgsOutOfRange,
               MsgAsinArgsOutOfRange,  MsgStackEmpty,          MsgSqrtArgIsNeg,
               MsgWrongEqualOperator
             };


    struct Token
     {  enum Type { Numb, Var, Func, BinOp, UnaOp, LeftPar, RightPar, EndMark };
        char type;
        union { double realval; int index; char code; } data;
     };

   private :

    SrArray<Token> _stack; 
    SrArray<Token> _table; 
    SrExpTableVarManager* _varman;
    char* _delimiters;

   public : // static methods :

    /*! This method can be used to get the name of all supported pre-defined
        functions. It returns the name of a given function index, that starts
        from zero. When index is out of range, null is returned. */
    static const char* function_name ( int index );

    /*! This method returns the corresponding SrExpTable::Msg from the given
        SrInput::Error, the returned Msg can be one of : MsgOk, MsgUndefChar,
        MsgNameTooBig, MsgStringNotClosed, MsgMisplacedDecPoint. */
    static Msg translate_error ( SrInput::ErrorType e );

    /*! Returns a string description of the given msg. Descriptions start with
        an upper case letter, and have no ending period. */
    static const char* msg_desc ( Msg m );

  public :
    
    /*! Constructor. */
    SrExpTable  ();

    /*! Destructor. */
   ~SrExpTable  ();

    /*! A derived class of SrExpTableVarManager is needed in order to enable
        the use of variables. The passed manager must be allocated with operator
        new and its deletion is controlled by SrExpTable. A null pointer can
        be passed to indicate that variables are not used. */
    void set_var_manager ( SrExpTableVarManager* var_man );

    /*! Make the parsing stop when any special delimeter is found.
        The parsing stops when one of the 
        specified delimiters is read, and then,
        after ungetting the read delimiter, MsgOk is returned when parse() is called.
        If the passed string is null or "", delimiters will not be taken into account. */
    void set_parser_delimiters ( const char *delimiters );

    /*! Parses an expression from the given input. The current line of the 
        parsed input is updated in curline. */
    Msg parse ( SrInput &in, int &curline );

    /*! Evaluates the expression and put the value in result if no error occured. */
    Msg evaluate ( double &result );

    /*! Compress the internal arrays. */
    void compress ();

    /*! Print the internal table. */
    friend SrOutput& operator << ( SrOutput& o, const SrExpTable& t );
 };


# endif // SR_EXP_TABLE_H
