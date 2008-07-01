#include "precompiled.h"
#include "0ad_warning_disable.h"
# include <stdlib.h>
# include <string.h>
# include <ctype.h>

# include "sr_input.h"
# include "sr_string.h"
# include "sr_array.h"

//# define SR_USE_TRACE1 //Parser
//# define SR_USE_TRACE2 //Init
# include "sr_trace.h"

# define ISNULL      _type==(srbyte)TypeNull
# define ISFILE      _type==(srbyte)TypeFile
# define ISSTRING    _type==(srbyte)TypeString

//=============================== SrInput =================================

struct SrInput::UngetData
 {  struct Token { char* string; srbyte type; };
    SrArray<int> character; // unget buffer for char reading
    SrArray<Token> token;   // unget buffer for token reading
    void init ();
 };

void SrInput::UngetData::init ()
 { 
   character.size(0); 
   while ( token.size() ) delete token.pop().string;
 }

void SrInput::_init ( char c )
 {
   _size = 0;
   _curline = 0;
   _comment_style = c;
   _type = (srbyte) TypeNull;
   _last_error = (srbyte) NoError;
   _last_token_type = 0;
   _max_token_size = 256;
   _lowercase_tokens = 1; // true
   _skipped_spaces = 0;
   _unget = new UngetData;
   _filename = 0;
 }

SrInput::SrInput ( char com )
 { 
   SR_TRACE2 ("Default Constructor");
   _init ( com );
 }

SrInput::SrInput ( const char *buff, char com )
 { 
   SR_TRACE2 ("String Constructor");
   _init ( com );
   if ( buff )
    { _cur.s = buff;
      _ini.s = buff;
      _size = (int)strlen ( buff );
      _curline = 1;
      _type = (srbyte) TypeString;
    }
 }

// static utility function:
static void get_size ( FILE *fp, int &size, int &start )
 { 
   start = (int)ftell(fp);
   fseek ( fp, 0, SEEK_END );
   size = (int)ftell(fp) - start;
   fseek ( fp, start, SEEK_SET );
 }

SrInput::SrInput ( FILE *file, char com )
 { 
   SR_TRACE2 ("File Constructor");
   _init ( com );
   if ( file )
    { _cur.f = file;
      _curline = 1;
      _type = (srbyte) TypeFile;
      get_size ( file, _size, _ini.f );
    }
 }

SrInput::SrInput ( const char* filename, const char* mode, char com )
 { 
   SR_TRACE2 ("File2 Constructor");
   _init ( com );
   FILE* file = fopen ( filename, mode );
   sr_string_set ( _filename, filename );
   if ( file )
    { _cur.f = file;
      _curline = 1;
      _type = (srbyte) TypeFile;
      get_size ( file, _size, _ini.f );
    }
 }

SrInput::~SrInput ()
 { 
   close (); // close frees all data inside _unget, and frees _filename
   delete _unget;
 }

void SrInput::init ( const char *buff ) 
 { 
   SR_TRACE2 ("Init with string");
   close ();
   if ( buff )
    { _cur.s = buff;
      _ini.s = buff;
      _size = (int)strlen ( buff );
      _curline = 1;
      _type = (srbyte) TypeString;
    }
 }

void SrInput::init ( FILE *file ) 
 { 
   SR_TRACE2 ("Init with file");
   close ();
   if ( file )
    { _cur.f = file;
      _curline = 1;
      _type = (srbyte) TypeFile;
      get_size ( file, _size, _ini.f );
    }
 }

void SrInput::init ( const char* filename, const char* mode )
 {
   SR_TRACE2 ("Init with file2");
   FILE* file = fopen ( filename, mode );
   init ( file );
   sr_string_set ( _filename, filename );
 }

void SrInput::close ()
 {
   if ( ISFILE ) fclose ( _cur.f );
   _size = 0;
   _curline = 0;
   _type = (srbyte) TypeNull;
   _last_error = (srbyte) NoError;
   _last_token = "";
   _last_token_type = 0;
   _unget->init ();
   sr_string_set ( _filename, 0 );
 }

void SrInput::leave_file ()
 { 
   _type = (srbyte) TypeNull;
   close ();   
 }

FILE* SrInput::filept ()
 {
   return _type==TypeFile? _cur.f:0;
 }

bool SrInput::valid () const
 { 
   return (ISNULL)? false : true;
 }

bool SrInput::finished ()
 {
   if ( _unget->character.size()>0 || _unget->token.size()>0 )  return false; 
   else if ( ISFILE ) return pos()>=_size? true:false;
   else if ( ISSTRING ) return *(_cur.s)? false:true;
   else return true;
 }

void SrInput::getall ( SrString& buf )
 {
   if ( ISFILE )
    { int s = size()-pos();
      buf.capacity ( s+2 ); // need +2 to cope with pc text files
      fread ( (void*)(const char*)buf, sizeof(char), (size_t)s, _cur.f );
      buf [ s+1 ] = 0;
    }
   else if ( ISSTRING )
    {
      buf.set ( _cur.s );
      _cur.s = _ini.s+_size;
    }
 }

int SrInput::getline ( SrString& buf )
 {
   int c;

   buf.len(0);
   do { c = get();
        buf << (char)c;
      } while ( c!='\n' && c!=EOF );

   return c;
 }

int SrInput::getchar ()
 {
   int c = EOF;

   if ( _unget->character.size()>0 ) 
    { c = _unget->character.pop();
    }
   else
    { if ( ISFILE ) c=fgetc(_cur.f); 
       else if ( ISSTRING ) c = *_cur.s? *_cur.s++:EOF;
    }

   if ( c=='\n' ) _curline++;
   return c;
}

static void skip_c_comment ( SrInput *p )
 {
   int c, d;
   while ( true )
    { c = p->getchar();
      if ( c=='/' ) // for nested comments
       { d = p->getchar();
         if ( d=='*' ) skip_c_comment(p); else p->unget(d);
       }
      else if ( c=='*' )
       { d = p->getchar();
         if ( d=='/' ) return; else p->unget(d);
       }
      else if ( c<0 ) return; // EOF found in the middle of a comment will not cause an error.
    }
 }

int SrInput::get ()
 {
   int c = getchar();

   if ( _comment_style==0 )
    { return c;
    }
   else if ( _comment_style=='C' && c=='/' )
    { int d = getchar();
      if ( d=='*' )
       { skip_c_comment(this);
         return get();
       }
      else if ( d=='/' )
       { skip_line ();
         return get();
       }
      else unget(d);
    }
   else if ( c==_comment_style )
    { skip_line ();
      return get();
    }
   return c;
 }

void SrInput::unget ( char c )
 {
   if ( c=='\n' && _curline>0 ) _curline--;
   _unget->character.push() = c;
 }

void SrInput::advance ( int n )
 {
   while ( n-->0 ) if ( getchar()<0 ) break;
 }

void SrInput::rewind ()
 {
   if ( ISNULL ) return;
   _unget->init();
   _curline = 1;
   if ( ISSTRING ) _cur.s = _ini.s;
    else fseek ( _cur.f, _ini.f, SEEK_SET );
 }

int SrInput::pos ()
 {
   if ( ISFILE ) return ((int)ftell(_cur.f)) - _ini.f;
    else if ( ISSTRING ) return _cur.s - _ini.s;
     else return 0;
 }

void SrInput::pos ( int pos )
 {
   _unget->init();
   if ( ISFILE ) fseek ( _cur.f, pos+_ini.f, SEEK_SET );
    else if ( ISSTRING ) _cur.s = _ini.s+pos;
 }

void SrInput::set_pos_and_update_cur_line ( int pos )
 {
   _unget->init();
   rewind ();
   advance ( pos );
 }

void SrInput::skip_line ()
 {
   int c;
   do { c = getchar();
      } while ( c!=EOF && c!='\n' );
 }

void SrInput::unget_token ( const char *token, SrInput::TokenType type )
 {
   UngetData::Token& t = _unget->token.push();
   t.string = 0;
   t.type = (srbyte) type;
   sr_string_set ( t.string, token );
 }

void SrInput::unget_token ()
 {
   if ( _last_token_type!=EndOfFile )
     unget_token ( _last_token, (TokenType)_last_token_type );
 }

bool SrInput::has_unget_data () const
 { 
   return _unget->token.size()==0 && _unget->character.size()==0? false:true;
 }

static char get_escape_char ( char c )
 {
   switch ( c )
    { case 'n' : return '\n';
      case 't' : return '\t';
      case '\n': return 0;    // Just skip line
      default  : return c;
    }
 }

SrInput::TokenType SrInput::get_token ( SrString &buf )
 {
   # define UNGET(c) if(c>0)unget(c)

   int i;
   int c = ' ';
   int size = _max_token_size;
   TokenType ret;

   _last_error = (srbyte) NoError;

   buf.capacity(size);

   if ( _unget->token.size()>0 )
    { UngetData::Token& t = _unget->token.pop();
      buf.set ( t.string );
      delete t.string;
      return (TokenType) t.type;
    }

   buf[0] = buf[1] = 0;

   _skipped_spaces = 0;
   while ( c && isspace(c) ) // skip initial spaces;
    { c=get(); _skipped_spaces++; }

   if ( c==EOF )
    { SR_TRACE1 ( "Got the End Of File!" );
      ret = EndOfFile;
    }
   else if ( strchr(SR_INPUT_DELIMITERS,c) && c!='.' ) // '.' will be detected after checking a real
    { buf[0]=c; ret=Delimiter;
      SR_TRACE1 ( "Got a Delimiter: "<<buf ); 
    }
   else if ( c=='"' )
    { SR_TRACE1 ( "Quote found..." );
      i = 0;
      while ( true )
       { c = getchar();                    // Comments inside a string are not considered
         if ( !c || c=='"' ) break;
         if ( c=='\\' ) c = get_escape_char ( getchar() );
         if ( c ) buf[i++]=c; 
         if ( i+1==size ) break;
       }
      buf[i]=0;
      if ( i+1==size ) { SR_TRACE1("Got an Error TooBig!"); ret=Error; _last_error=TooBig; }
       else if ( c ) { SR_TRACE1("Got a String: ["<<buf<<"] size="<<strlen(buf)); ret=String; }
       else { SR_TRACE1("Got an Error OpenString!"); ret=Error; _last_error=OpenString; }
    }
   else if ( c=='.' || isdigit(c) )
    { SR_TRACE1 ( "Digit found..." );
      bool pnt=false, exp=false;
      i = 0;
      while ( true )
       { if ( c=='e' ) c='E';
         if ( !pnt && c=='.' ) pnt=true;
          else if ( pnt && c=='.' ) break;
          else if ( !exp && c=='E' ) exp=pnt=true;
          else if ( (c=='+'||c=='-') && buf[i-1]=='E' );
          else if ( !isdigit(c) ) break;
         buf[i++]=c; 
         if ( i+1==size ) break;
         c = getchar();
       }
      buf[i]=0;

      if ( buf[0]=='.' && i==1 && strchr(SR_INPUT_DELIMITERS,'.') ) 
        { ret=Delimiter; UNGET(c); SR_TRACE1("Got a Delimiter: ["<<buf<<"]"); }
       else if ( i+1==size ) { ret=Error; _last_error=TooBig; SR_TRACE1("Got an Error TooBig!"); }
       else if ( pnt && c=='.' ) { ret=Error; _last_error=InvalidPoint; SR_TRACE1("Got an Error InvalidPoint!"); }
       else if ( pnt || exp ) { ret=Real;  UNGET(c); SR_TRACE1("Got a Real: ["<<buf<<"] => "<<atof(buf)); }
       else { ret=Integer; UNGET(c); SR_TRACE1("Got an Integer: ["<<buf<<"] => "<<atoi(buf)); }
    }
   else if ( c=='_' || isalpha(c) )
    { SR_TRACE1 ( "Alpha found..." );
      i = 0;
      while ( true )
       { if ( !isalnum(c) && c!='_' ) break;
         buf[i++]=c; 
         if ( i+1==size ) break;
         c = getchar();
       }
      buf[i]=0;
      if ( i+1==size ) { SR_TRACE1("Got an Error TooBig!"); ret=Error; _last_error=TooBig; }
       else { SR_TRACE1("Got a Name: ["<<buf<<']'); ret=Name; UNGET(c); }
      if ( _lowercase_tokens ) buf.lower();
    } 
   else
    { SR_TRACE1("Got an Error Undef, code: "<<(int)c<<" ["<<(char)c<<']');
      buf[0]=c;
      ret = Error;
      _last_error = UndefChar;
    }

   return ret;
   # undef UNGET
 }

SrInput::TokenType SrInput::get_token ()
 {
   _last_token_type = get_token ( _last_token );
   return (TokenType)_last_token_type;
 }

const char* SrInput::error_desc ( SrInput::ErrorType t )
 {
   switch ( t )
    { case UndefChar :      return "Undefined character found";
      case UnexpectedToken: return "Parsed token is of an unexpected type";
      case TooBig :         return "Name too big";
      case OpenString :     return "EOF found before end of a string";
      case InvalidPoint :   return "Misplaced decimal point";
      default : return 0;
    }
 }

SrString& SrInput::gets ()
 {
   get_token();
   if ( _last_token_type!=String && _last_token_type!=Name )
    { _last_error=UnexpectedToken; _last_token=""; }
   return _last_token;
 }

SrString& SrInput::gets ( SrString& buf )
 {
   TokenType t = get_token ( buf );
   if ( t!=String && t!=Name ) _last_error=UnexpectedToken;
   return buf;
 }

SrString& SrInput::getn ()
 {
   int signal=1;

   get_token();

   // accumulate +- delimiter if any:
   while ( _last_token_type==Delimiter )
    { if ( _last_token[0]=='-' ) 
        signal*=-1;
      else if ( _last_token[1]!='+' )
        break; // will then return ""
      get_token();
    }

   if ( _last_token_type!=Integer && _last_token_type!=Real )
    { _last_error=UnexpectedToken; _last_token=""; return _last_token; }

   if ( signal==-1 ) _last_token.insert(0,"-");

   return _last_token;
 }

char SrInput::getd ()
 {
   get_token();
   if ( _last_token_type!=Delimiter )
    { _last_error = UnexpectedToken;
      return 0;
    }
   return _last_token[0];
 }

bool SrInput::skip ( int n )
 {
   while ( n-- )
    { get_token();
      if ( _last_token_type==Error || _last_token_type==EndOfFile ) return false;
    }
   return true;
 }

bool SrInput::skipto ( const char *name )
 {
   while ( true )
    { get_token();
      if ( _last_token_type==Error || _last_token_type==EndOfFile ) return false;
      if ( _last_token_type==Name )
       { if ( _last_token==name ) return true; }
    }
 }

bool SrInput::has_field ()
 {
   if ( getd()!='<' ) { unget_token(); return false; }
   if ( gets()=="" )  { unget_token(); unget_token(); return false; }
   char d = getd();
   unget_token();
   unget_token();
   unget_token();
   return d=='>'? true:false;
 }

bool SrInput::read_field ( SrString& name )
 {
   name = "";
   if ( getd()!='<' ) return false;
   name = gets();
   if ( name=="" ) return false;
   if ( getd()!='>' ) return false;
   return true;
 }

bool SrInput::close_field ( const SrString& name )
 {
   if ( getd()!='<' ) return false;
   if ( getd()!='/' ) return false;
   if ( gets()!=name ) return false;
   if ( getd()!='>' ) return false;
   return true;
 }

bool SrInput::skip_field ( const SrString& name )
 {
   while ( true )
    { if ( close_field(name) ) return true;
      if ( _last_token_type==EndOfFile ) return false;
    }
 }

//================================= operators ==================================

SrInput& operator>> ( SrInput& in, int& i )      
 { 
   SrString& s = in.getn();
   i = s.atoi();
   return in; 
 }

SrInput& operator>> ( SrInput& in, sruint& i ) 
 { 
   SrString& s = in.getn();
   i = (sruint)s.atoi();
   return in; 
 }

SrInput& operator>> ( SrInput& in, srbyte& c ) 
 { 
   SrString& s = in.getn();
   c = (srbyte)s.atoi();
   return in; 
 }

SrInput& operator>> ( SrInput& in, float& f )    
 { 
   SrString& s = in.getn();
   f = s.atof();
   return in; 
 }

SrInput& operator>> ( SrInput& in, double& d ) 
 { 
   SrString& s = in.getn();
   d = s.atod();
   return in; 
 }

SrInput& operator>> ( SrInput& in, char* st ) 
 { 
   SrString& s = in.gets();
   strcpy ( st, s );
   return in; 
 }

//============================ End of File ==============================
