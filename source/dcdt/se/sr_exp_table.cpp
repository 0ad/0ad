#include "precompiled.h"
#include "0ad_warning_disable.h"
//***************************************************************************
//
//  SrExpTABLE.H
//  By Marcelo Kallmann 08/98 - Brazil
//
//***************************************************************************

# include <math.h>
# include <string.h>
# include <stdlib.h>

# include "sr_array.h"
# include "sr_string.h"
# include "sr_exp_table.h"

//# define SR_USE_TRACE1 // Parse
//# define SR_USE_TRACE2 // Eval
//# define SR_USE_TRACE3 // Table
# include "sr_trace.h"

// ============================= EToken ========================================

typedef SrExpTable::Token  EToken;

inline void setNumb     ( EToken& t, double d ) { t.type=(char)EToken::Numb;   t.data.realval=d; }
inline void setVar      ( EToken& t, int id )   { t.type=(char)EToken::Var;    t.data.index=id;  }
inline void setFunc     ( EToken& t, int id )   { t.type=(char)EToken::Func;   t.data.index=id;  }
inline void setBinOp    ( EToken& t, char op )  { t.type=(char)EToken::BinOp;  t.data.code=op;   }
inline void setUnaOp    ( EToken& t, char op )  { t.type=(char)EToken::UnaOp;  t.data.code=op;   }
inline void setLeftPar  ( EToken& t )           { t.type=(char)EToken::LeftPar;  }
inline void setRightPar ( EToken& t )           { t.type=(char)EToken::RightPar; }

//======================= Modes while parsing =================================

enum srParMode { srParModeUnary, srParModeBinary, srParModeFunction };

//======================== functions methods ===================================

enum srFunc { srFuncAbs,  srFuncAcos,  srFuncAsin, srFuncAtan,  srFuncCeil,
              srFuncCos,  srFuncExp,   srFuncFact, srFuncFloor, srFuncLn,
              srFuncLog,  srFuncRound, srFuncSign, srFuncSin,   srFuncSqrt,
              srFuncTan,  srFuncUndefined }; // Alphabetical order !

static const char *Functions[] = { "abs", "acos",  "asin", "atan",  "ceil",
                             "cos", "exp",   "fact", "floor", "ln",
                             "log", "round", "sign", "sin",   "sqrt",
                             "tan"  }; // Alphabetical order !

static int compfunc ( const void *a, const void *b )
 {
   return strcmp ( ((char*)a), *((char**)b) );
 }

static srFunc func_code ( const char *st )
 {
   void *result = bsearch ( (const void*)st,         // to search
 		                    (const void*)Functions,  // table
                            (size_t)srFuncUndefined, // num elems in table
                            (size_t)sizeof(char*),   // size of each elem in table
                            compfunc                 // compare function
                          );
   if (!result) return srFuncUndefined;
   return (srFunc) ( ((intptr_t)result-(intptr_t)Functions)/sizeof(int) );
 }

const char *SrExpTable::function_name ( int index )
 {
   if ( index<0 || index>=(int)srFuncUndefined ) return 0;
   return Functions[index];
 }

static int oprank ( char op )
 {
   switch ( op )
    { case '[' :                       return 8;
      case '^' : case '%' :            return 7;
      case '/' : case '*' : case '|' : return 6;
      case '+' : case '-' :            return 5;
      case '>' : case '<' : 
      case ')' : case '(' : // >= <=
      case '~' : case '=' :            return 4;
      case 'a' : case 'o' :            return 3; // and or
    }
   return 0;
 }

//===================================== messages ==============================
// Global functions :

SrExpTable::Msg SrExpTable::translate_error ( SrInput::ErrorType e )
 {
   switch ( e )
    { case SrInput::UndefChar :    return MsgUndefChar;
      case SrInput::TooBig :       return MsgNameTooBig;
      case SrInput::OpenString :   return MsgStringNotClosed;
      case SrInput::InvalidPoint : return MsgMisplacedDecPoint;
      default : return MsgOk;
    }
 }

const char* SrExpTable::msg_desc ( SrExpTable::Msg m )
 {
   switch ( m )
    { // messages translated from sr_input.h :
      case MsgUndefChar           : return SrInput::error_desc ( SrInput::UndefChar );
      case MsgNameTooBig          : return SrInput::error_desc ( SrInput::TooBig );
      case MsgStringNotClosed     : return SrInput::error_desc ( SrInput::OpenString );
      case MsgMisplacedDecPoint   : return SrInput::error_desc ( SrInput::InvalidPoint );
      // new messages :
      case MsgOk                  : return "Ok";
      case MsgUnexpectedEndOfFile : return "Unexpected end of file encountered";
      case MsgUndefinedName       : return "Undefined variable"; 
      case MsgUndefBinOp          : return "Undefined binary operator: internal error";
      case MsgUndefUnaOp          : return "Undefined unary operator: internal error";
      case MsgUndefPreDefFunc     : return "Undefined pre-defined function: internal error";
      case MsgOperandExpected     : return "Operand expected in expression";
      case MsgOperatorExpected    : return "Operator expected in expression";
      case MsgFuncWithoutPar      : return "Function name must be followed by a left parenthesis";
      case MsgDivideByZero        : return "Divide by zero";
      case MsgRaisedZeroToNeg     : return "Cannot raise zero to a negative power";
      case MsgRaisedNegToReal     : return "Cannot raise a negative number to a non-integer power";
      case MsgUnmatchedParentesis : return "Unmatched parentesis";
      case MsgAcosArgsOutOfRange  : return "Arc cos arguments out of range";
      case MsgAsinArgsOutOfRange  : return "Arc sin arguments out of range";
      case MsgStackEmpty          : return "Stack is empty";
      case MsgSqrtArgIsNeg        : return "Sqrt argument is negative";
      default : return NULL;
    }
 }

//============================ internal ======================================

# define ISBINARY(c)    strchr("+-*/^%|><=[",c)
# define ISUNARY(c)     strchr("+-!",c)

static bool is_binary ( const SrString& token )
 {
   if ( strchr("+-*/^%|><~=[",token[0]) ) return true;
   if ( token=="and" || token=="or" ) return true;
   return false;
 }

static SrExpTable::Msg get_translated_token ( SrInput& in, 
                                              int& curline,
                                              EToken& etok,
                                              srParMode mode, 
                                              const char* delimeters, 
                                              SrExpTableVarManager* varman )
 {
   SrInput::TokenType toktype;
   SrString token;

   toktype = in.get_token(token);
   curline = in.curline();
   if ( toktype==SrInput::Error ) return SrExpTable::translate_error ( in.last_error() );
   if ( toktype==SrInput::EndOfFile ) return SrExpTable::MsgUnexpectedEndOfFile;

   if ( delimeters && toktype==SrInput::Delimiter && strchr(delimeters,token[0]) )
    { in.unget_token ( token, toktype ); 
      return SrExpTable::MsgUnexpectedEndOfFile; // permits to stop the parser before end of file
    }

   if ( mode==srParModeUnary && ISUNARY(token[0]) )
    { setUnaOp ( etok, token[0] );
      SR_TRACE1 ( "Got a unary op: ["<<token<<"]" );
      return SrExpTable::MsgOk;
    }

   if ( (mode==srParModeBinary || mode==srParModeFunction) && is_binary(token) )
    { char op = token[0];
      if ( op=='>' || op=='<' )
       { toktype = in.get_token(token);
         if ( toktype==SrInput::Error ) return SrExpTable::translate_error ( in.last_error() );
         curline = in.curline();
         if ( op=='>' && token[0]=='=' ) op = ')';  // that will mean >= 
          else
         if ( op=='<' && token[0]=='=' ) op = '(';  // that will mean <= 
          else
         if ( op=='<' && token[0]=='>' ) op = '~';  // <> mean difference op ~
          else
         in.unget_token ( token, toktype );
       }
      else if ( op=='=' )
       { toktype = in.get_token(token);
         if ( toktype==SrInput::Error ) return SrExpTable::translate_error ( in.last_error() );
         if ( token[0]!='=' ) return SrExpTable::MsgWrongEqualOperator;
       }
      else if ( op=='[' )
       {
	     in.unget_token ( "(", SrInput::Delimiter ); // so that arrays indices may form an expression
       }
      setBinOp ( etok, op );
      SR_TRACE1 ( "Got a binary op: ["<<op<<"]");
      return SrExpTable::MsgOk;
    }

   if ( token[0]=='(' ) { setLeftPar(etok);  SR_TRACE1("Got a left par.\n");      return SrExpTable::MsgOk; }
   if ( token[0]==')' ) { setRightPar(etok); SR_TRACE1("Got a right par.\n");     return SrExpTable::MsgOk; }
   if ( token[0]==']' ) { setRightPar(etok); SR_TRACE1("Got array right par.\n"); return SrExpTable::MsgOk; }

   if ( toktype==SrInput::Name ) // bool(int), func or var
    { if ( strcmp(token,"true")==0  ) { setNumb(etok,1.0); SR_TRACE1("Got a true.\n");  return SrExpTable::MsgOk; }
      if ( strcmp(token,"false")==0 ) { setNumb(etok,0.0); SR_TRACE1("Got a false.\n"); return SrExpTable::MsgOk; }

      srFunc f = func_code ( token ); // found a func?
      if ( f!=srFuncUndefined ) 
       { setFunc(etok,f); SR_TRACE1("Got func ["<<(Functions[(int)f])<<"]"); return SrExpTable::MsgOk; }

      if ( !varman ) return SrExpTable::MsgUndefinedName; // found a name that is not a function or variable
      setVar ( etok, varman->get_variable_index(token) ); // get the user index for the variable
      SR_TRACE1("Got a var ["<<token<<"], index "<<etok.data.index );
      if ( etok.data.index<0 ) return SrExpTable::MsgUndefinedName; 

      return SrExpTable::MsgOk;
    }

   if ( toktype==SrInput::Integer || toktype==SrInput::Real )
    { setNumb(etok,(double)atof(token)); SR_TRACE1("Got a number: "<<(atof(token))); return SrExpTable::MsgOk; }

   return SrExpTable::MsgUndefChar;
 }

//===================================== SrExpTable ===================================================

SrExpTable::SrExpTable ()
 { 
   _varman = 0;
   _delimiters = 0;
 }

SrExpTable::~SrExpTable () 
 {
   if ( _varman ) _varman->unref();
   delete _delimiters;
 }

void SrExpTable::set_var_manager ( SrExpTableVarManager* var_man )
 { 
   if ( _varman ) _varman->unref();
   _varman = var_man;
   _varman->ref();
 }

void SrExpTable::set_parser_delimiters ( const char *delimiters )
 {
   sr_string_set ( _delimiters, delimiters );
 }

SrExpTable::Msg SrExpTable::parse ( SrInput& in, int &curline )
 { 
   Msg msg;
   Token etok, t;
   srParMode mode = srParModeUnary;

   _table.size ( 0 );
   _stack.size ( 0 );

   while ( true )
    { 
      msg = get_translated_token ( in, curline, etok, mode, _delimiters, _varman );
      if ( msg==MsgUnexpectedEndOfFile ) break;
      if ( msg!=MsgOk ) return msg;

      if ( mode==srParModeUnary && (etok.type==Token::BinOp||etok.type==Token::RightPar) )
	   { return MsgOperandExpected; }

      if ( mode==srParModeBinary && etok.type!=Token::BinOp && etok.type!=Token::RightPar )
       { return MsgOperatorExpected; }

      if ( mode==srParModeFunction && etok.type!=Token::LeftPar )
	   { return MsgFuncWithoutPar; }

      switch ( etok.type )
       { case Token::Numb    :
         case Token::Var     : _stack.push(etok); mode=srParModeBinary; break;
         case Token::LeftPar :
         case Token::UnaOp   : _stack.push(etok); mode=srParModeUnary; break;
         case Token::Func    : _stack.push(etok); mode=srParModeFunction; break;

         case Token::BinOp   :
              while ( !_stack.empty() )
               { t=_stack.top ();
                 if ( t.type==Token::BinOp && oprank(etok.data.code)>oprank(t.data.code) ) break;
                 if ( t.type==Token::LeftPar ) break;
                 _stack.pop(); _table.push(t);
               }
              _stack.push(etok); mode=srParModeUnary; break;

         case Token::RightPar :
              if ( _stack.empty() ) return MsgStackEmpty;
              while ( !_stack.empty() ) 
               { t=_stack.pop(); 
                 if (t.type==Token::LeftPar) break; 
                 _table.push(t);
               } 
              if ( t.type!=Token::LeftPar ) return MsgOk; // here the stack will be empty
              if ( !_stack.empty() && _stack.top().type==Token::Func ) 
              _table.push( _stack.pop() );
              mode=srParModeBinary; 
              break;
       }
    }

   if ( mode==srParModeUnary) return MsgOperandExpected;

   while ( !_stack.empty() )
    { if ( _stack.top().type==Token::LeftPar ) { return MsgUnmatchedParentesis; }
      _table.push( _stack.pop() );
    }

   _table.push().type=Token::EndMark; // mark the end

   # ifdef SR_USE_TRACE3
   sr_out << "ExpTable: " << *this << srnl;
   # endif

   return MsgOk;
 }

static SrExpTable::Msg operateBinOp ( double x, double y, char op,
                                      SrArray<EToken>& stack, 
                                      SrExpTableVarManager* varman )
 {
   double r;
   switch ( op )
    { case '[' : r = varman->get_variable_value(int(x),int(y)); break;
      case '+' : r = x+y; break;
      case '-' : r = x-y; break;
      case '/' : if (!y) return SrExpTable::MsgDivideByZero; r=x/y; break;
      case '|' : if (!y) return SrExpTable::MsgDivideByZero; r=(double)(((int)x)|((int)y)); break;
      case '*' : r=x*y; break;
      case '%' : r = (x/100.0)*y; break;
      case '^' : if ( x==0.0 && y<0.0     ) return SrExpTable::MsgRaisedZeroToNeg;
                 if ( x<0.0  && y!=(int)y ) return SrExpTable::MsgRaisedNegToReal;
                 r=pow(x,y); break;
      case 'a' : r = x!=0 && y!=0? 1.0:0.0; break;
      case 'o' : r = x!=0 || y!=0? 1.0:0.0; break;
      case '>' : r = x>y?          1.0:0.0; break;
      case '<' : r = x<y?          1.0:0.0; break;
      case ')' : r = x>=y?         1.0:0.0; break;
      case '(' : r = x<=y?         1.0:0.0; break;
      case '~' : r = x!=y?         1.0:0.0; break;
      case '=' : r = x==y?         1.0:0.0; break;
      default : return SrExpTable::MsgUndefBinOp;
    }
   SR_TRACE2 ( "Operating: "<< x << " " << op << " " << y << " = " << r );
   setNumb ( stack.push(), r );
   return SrExpTable::MsgOk;
 }

static SrExpTable::Msg operateUnaOp ( double x, char op, SrArray<EToken>& stack )
 {
   double r;
   switch ( op )
    { case '+' : r=x;  break;
      case '-' : r=-x; break;
      case '!' : r = x==0? 1.0:0.0; break;
      default : return SrExpTable::MsgUndefUnaOp;
    }
   SR_TRACE2 ( "Operating: " << op << " " << x << " = " << r );

   setNumb ( stack.push(), r );
   return SrExpTable::MsgOk;
 }

static double dround ( double x ) { return (x>0.0)? (double)(long int)(x+0.5) : (double)(long int)(x-0.5); }
static double dsign  ( double x ) { return (x<0.0)? -1: (x>0.0)? 1:0; }
static double dfact  ( int x )    { if(x<2)return 1l; long m=(long)x; while(--x>1)m*=(long)x; return(double)m; }

static SrExpTable::Msg operateFunc ( double x, srFunc f, SrArray<EToken>& stack )
 {
   double r;
   switch ( f )
    { case srFuncAbs   : r = x<0.0? -x:x;    break;
      case srFuncAcos  : if ( x<-1.0 || x>1.0 ) return SrExpTable::MsgAcosArgsOutOfRange; 
                         r=acos(x);          break;
      case srFuncAsin  : if ( x<-1.0 || x>1.0 ) return SrExpTable::MsgAsinArgsOutOfRange; 
                         r=asin(x);          break;
      case srFuncAtan  : r=atan(x);          break;
      case srFuncCeil  : r=ceil(x);          break;
      case srFuncCos   : r=cos(x);           break;
      case srFuncExp   : r=pow(SR_E,x);      break;
      case srFuncFact  : r=dfact((int)x);    break;
      case srFuncFloor : r=floor(x);         break;
      case srFuncLn    : r=log(x);           break;
      case srFuncLog   : r=log10(x);         break;
      case srFuncSign  : r=dsign(x);         break;
      case srFuncSin   : r=sin(x);           break;
      case srFuncSqrt  : if ( x<-0.0 ) return SrExpTable::MsgSqrtArgIsNeg; 
                         r=sqrt(x);          break;
      case srFuncTan   : r=tan(x);           break;
      case srFuncRound : r=dround(x);        break;
      default : return SrExpTable::MsgUndefPreDefFunc;
    }

   SR_TRACE2 ( "Operating: " << Functions[f] << "(" << x << ") = " << r );

   setNumb ( stack.push(), r );
   return SrExpTable::MsgOk;
 }

static SrExpTable::Msg pop_value ( double &v, SrArray<EToken>& stack, 
                                   SrExpTableVarManager* varman, bool isarrayop )
 { 
   if ( stack.empty() ) return SrExpTable::MsgStackEmpty;
   EToken t = stack.pop();

   if ( t.type==EToken::Var )
    { v = isarrayop? (double)t.data.index : varman->get_variable_value(t.data.index,-1);
      return SrExpTable::MsgOk;
    }
   else if ( t.type==EToken::Numb )
    { v = t.data.realval;
      return SrExpTable::MsgOk;
    }
   else
    return SrExpTable::MsgOperandExpected;
 }

SrExpTable::Msg SrExpTable::evaluate ( double &result )
 {
   int i;
   double u, v;
   Token::Type t;
   Msg msg;

   _stack.size ( 0 );

   for ( i=0; i<_table.size(); i++ )
    { t = (Token::Type) _table[i].type;
      if ( t==Token::EndMark ) break; // Found a mark for the end (only when not compressed)

      if ( t==Token::Numb || t==Token::Var ) // a value
       { _stack.push ( _table[i] );
       }
      else
       { msg = pop_value ( v, _stack, _varman, false );
         if ( msg!=MsgOk ) return msg;

         switch ( t )
          { case Token::Func :
                 msg = operateFunc  ( v, (srFunc)_table[i].data.index, _stack );
                 break;

            case Token::UnaOp :
                 msg = operateUnaOp ( v, _table[i].data.code, _stack );
                 break;

            case Token::BinOp :
                 msg = pop_value ( u, _stack, _varman, _table[i].data.code=='['?true:false ); 
                 if ( msg!=MsgOk ) break;
                 msg = operateBinOp ( u, v, _table[i].data.code, _stack, _varman );
                 break;
	      }

         if ( msg!=MsgOk ) return msg;
       } 
    }

   if ( _stack.size()!=1 ) return MsgOperatorExpected;

   return pop_value ( result, _stack, _varman, false );
 }

void SrExpTable::compress ()
 {
   _table.compress ();
   _stack.compress();
 }

    /*! Takes the internal data of t, and leave t as an empty table. */
//    void take_data ( SrExpTable& t );

//    void take_data ( SrArray<SrExpTableToken>& tokens );
//    void leave_data ( SrArray<SrExpTableToken>& tokens );

/*
void SrExpTable::take_data ( SrExpTable& t )
 {
   if ( _tokens ) free ( _tokens );
   _tokens = t._tokens;     t._tokens = 0;
   _size = t._size;         t._size = 0;
   _capacity = t._capacity; t._capacity = 0;
 }
*/

//=============================== friends ============================

SrOutput& operator << ( SrOutput& o, const SrExpTable& t )
 {
   EToken tk;

   for ( int i=0; i<t._table.size(); i++ )
    { tk = t._table[i];

      switch ( tk.type )
       { case EToken::Numb  : o<<tk.data.realval<<srspc; break;
         case EToken::Var   : o<<"var:"<<tk.data.index<<srspc; break;
         case EToken::Func  : o<<Functions[tk.data.index]<<srspc; break;
         case EToken::UnaOp :
         case EToken::BinOp : o<<tk.data.code<<srspc; break;
         case EToken::EndMark : return o;
       }
    }

   return o;
     
 }

//============================ End of File ==============================

