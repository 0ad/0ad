#include "precompiled.h"
# include <ctype.h>
# include <string.h>
# include <stdlib.h>

# include "sr_var.h"
# include "sr_output.h"

//======================================== SrVar =======================================

SrVar::SrVar ()
 {
   _name = 0;
   _type = 'i';
 }

SrVar::SrVar ( char type )
 {
   _name = 0;
   _type = type;
   switch ( _type )
    { case 'i': break;
      case 'f': break;
      case 's': break;
      case 'b': break;
      default: _type = 'i';
    }
 }

SrVar::SrVar ( const char* name, char type )
 {
   _name = 0;
   sr_string_set ( _name, name );
   _type = type;
   switch ( _type )
    { case 'i': break;
      case 'f': break;
      case 's': break;
      case 'b': break;
      default: _type = 'i';
    }
 }

SrVar::SrVar ( const char* name, bool value )
 {
   _name = 0;
   sr_string_set ( _name, name );
   _type = 'b';
   _data.push().b = value;
 }

SrVar::SrVar ( const char* name, int value )
 {
   _name = 0;
   sr_string_set ( _name, name );
   _type = 'i';
   _data.push().i = value;
 }

SrVar::SrVar ( const char* name, float value )
 {
   _name = 0;
   sr_string_set ( _name, name );
   _type = 'f';
   _data.push().f = value;
 }

SrVar::SrVar ( const char* name, const char* value )
 {
   _name = 0;
   sr_string_set ( _name, name );
   _type = 's';
   _data.push().s = 0;
   sr_string_set ( _data[0].s, value );
 }

SrVar::SrVar ( const SrVar& v )
 {
   _type = 'i';
   _name = 0;
   init ( v );
 }

SrVar::~SrVar ()
 {
   name ( 0 );
   init ( 'i' );
 }

void SrVar::name ( const char* n )
 {
   sr_string_set ( _name, n );
 }

const char* SrVar::name () const
 {
   return _name? _name:"";
 }

void SrVar::init ( char type )
 {
   if ( _type=='s' ) // delete all used strings
    { while ( _data.size() ) delete[] _data.pop().s;
    }

   _data.size ( 0 );
   _type = type;

   switch ( _type )
    { case 'i': break;
      case 'f': break;
      case 's': break;
      case 'b': break;
      default: _type = 'i';
    }
 }

void SrVar::init ( const SrVar& v )
 {
   name ( v.name() );
   init ( v.type() );

   if ( _type!='s' )
    { _data = v._data;
    }
   else
    { int i;
      _data.size ( v.size() );
      for ( i=0; i<_data.size(); i++ )
       { _data[i].s = 0;
         sr_string_set ( _data[i].s, v._data[i].s );
       }
    }  
 }

void SrVar::size ( int ns )
 {
   int i, s = _data.size();

   if ( ns<s )
    { if ( _type=='s' )
       { for ( i=ns; i<s; i++ ) delete[] _data[i].s; }
      _data.size ( ns );
    }
   else if ( ns>s )
    { _data.size ( ns );
      for ( i=s; i<ns; i++ ) _data[i].s=0; // ok for all types
    }
 }

//--------------------------- set value ---------------------------

void SrVar::set ( bool b, int index )
 {
   switch ( _type )
    { case 'b':
       if ( index<0 || index>=_data.size() )
        { _data.push().b = b; }
       else
        { _data[index].b = b; }
       break;

      case 'i': set ( (int)b, index );
      case 'f': set ( (float)b, index );
      case 's': set ( b? "true":"false", index ); break;
    }
 }

void SrVar::set ( int i, int index )
 {
   switch ( _type )
    { case 'i':
       if ( index<0 || index>=_data.size() )
        { _data.push().i = i; }
       else
        { _data[index].i = i; }
       break;

      case 'f': set ( float(i), index );
      case 'b': set ( bool(i? true:false), index );
      case 's': { SrString s; s<<i; set ( s, index ); } break;
    }
 }

void SrVar::set ( float f, int index )
 {
   switch ( _type )
    { case 'f':
       if ( index<0 || index>=_data.size() )
        { _data.push().f = f; }
       else
        { _data[index].f = f; }
       break;

      case 'i': set ( int(f), index );
      case 'b': set ( bool(f==1.0f? true:false), index );
      case 's': { SrString s; s<<f; set ( s, index ); } break;
    }
 }

void SrVar::set ( const char* s, int index )
 {
   switch ( _type )
    { case 's':
       if ( index<0 || index>=_data.size() )
        { _data.push().s = 0;
          index = _data.size()-1;
        }
       sr_string_set ( _data[index].s, s );
       break;

      case 'i': set ( atoi(s), index );
      case 'f': set ( atof(s), index );
      case 'b': set ( bool(sr_compare(s,"true")==0? true:false), index );
    }
 }

void SrVar::push ( bool b )
 {
   SR_ASSERT ( _type=='b' );
   _data.push().b = b;
 }

void SrVar::push ( int i )
 {
   SR_ASSERT ( _type=='i' );
   _data.push().i = i;
 }

void SrVar::push ( float f )
 {
   SR_ASSERT ( _type=='f' );
   _data.push().f = f;
 }

void SrVar::push ( const char* s )
 {
   SR_ASSERT ( _type=='s' );

   _data.push().s = 0;
   sr_string_set ( _data.top().s, s );
 }

//--------------------------- get value ---------------------------

bool SrVar::getb ( int index ) const
 {
   if ( index<0 || index>=_data.size() ) return 0;
   switch ( _type )
    { case 's': if ( !_data[index].s ) return false;
                if ( sr_compare(_data[index].s,"true")==0 ) return true;
                return false;
      case 'i': return _data[index].i? true:false;
      case 'f': return _data[index].f==0? false:true;
      case 'b': return _data[index].b;
    }
   return false;
 }

int SrVar::geti ( int index ) const
 {
   if ( index<0 || index>=_data.size() ) return 0;
   switch ( _type )
    { case 's': return _data[index].s? atoi(_data[index].s):0;
      case 'i': return _data[index].i;
      case 'f': return (int)_data[index].f;
      case 'b': return (int)_data[index].b;
    }
   return 0;
 }

float SrVar::getf ( int index ) const
 {
   if ( index<0 || index>=_data.size() ) return 0;
   switch ( _type )
    { case 's': return _data[index].s? (float)atof(_data[index].s):0;
      case 'i': return (float)_data[index].i;
      case 'f': return _data[index].f;
      case 'b': return (float)_data[index].b;
    }
   return 0;
 }

const char* SrVar::gets ( int index ) const
 {
   if ( index<0 || index>=_data.size() ) return 0;
   if ( _type=='s' && _data[index].s )
    return _data[index].s;
   return "";
 }

//-----------------------------------------------------------------

void SrVar::remove ( int i, int n )
 {
   if ( n<=0 || i<0 || i>=_data.size() ) return;
   if ( _type=='s' )
    { int pi, pe=i+n;
      for ( pi=i; pi<=pe && pi<_data.size(); pi++ ) 
       delete[] _data[pi].s;
    }
   _data.remove ( i, n );
 }

void SrVar::insert ( int i, int n )
 {
   if ( n<=0 || i<0 || i>_data.size() ) return;
   _data.insert ( i, n );
   while ( i<i+n )
     _data[i].s = 0; // will work for all types
 }

SrVar& SrVar::operator= ( const SrVar& v )
 {
   init ( v );
   return *this;
 }

//============================== friends ========================================

static void outs ( SrOutput& o, int i, const char* s, SrString& buf )
 {
   if ( !s ) { o<<"\"\""; return; }

   if ( i==0 ) // i==0 means the first element of a string type
    { if ( sr_compare(s,"true")==0  ) { o << "\"true\""; return; }
      if ( sr_compare(s,"false")==0  ) { o << "\"false\""; return; }
    }

   buf.make_valid_string ( s );
   o << buf;
 }

SrOutput& operator<< ( SrOutput& o, const SrVar& v )
 {
   if ( v._name==0 )
    o << "var";
   else if ( v._name[0]==0 )
    o << "var";
   else
    o << v._name;

   o << " = ";

   if ( v._data.size()==0 )
    { switch ( v._type )
       { case 'b' : o<<"false;"; break;
         case 'i' : o<<"0;";     break;
         case 'f' : o<<"0.0;";   break;
         case 's' : o<<"\"\";";  break;
       }
      return o;
    }

   int len=0;
   int i, s=v._data.size();
   int e = s-1;
   SrString buf;

   switch ( v._type )
    { case 'f': for ( i=0; i<s; i++ )
                 { o<<v._data[i].f;
                   len+=1;
                   if (i<e)
                    { if ( len>16 ) { o<<srnl<<srtab; len=0; }
                       else o<<srspc;
                    }
                 } break;

      case 'i': for ( i=0; i<s; i++ )
                 { o<<v._data[i].i;
                   len+=1;
                   if (i<e)
                    { if ( len>20 ) { o<<srnl<<srtab; len=0; }
                       else o<<srspc;
                    }
                 } break;

      case 's': for ( i=0; i<s; i++ ) 
                 { outs(o,i,v._data[i].s,buf);
                   len += strlen(v._data[i].s);
                   if (i<e)
                    { if ( len>80 ) { o<<srnl<<srtab; len=0; }
                       else o<<srspc;
                    }
                 } break;

      case 'b': for ( i=0; i<s; i++ )
                 { o<<v._data[i].b;
                   len += v._data[i].b? 5:7;
                   if (i<e)
                    { if ( len>80 ) { o<<srnl<<srtab; len=0; }
                       else o<<srspc;
                    }
                 } break;
    }

   return o<<';';
 }

SrInput& operator>> ( SrInput& in, SrVar& v )
 {
   SrString buf;

   while ( true )
    { in.get_token();
      if ( in.finished() ) return in;
      if ( in.last_token()[0] == '=' ) break;
      buf << in.last_token();
    }
 
   v.name ( buf );
   v.init ( 'i' );

   bool negative = false;

   in.get_token();
   SrString& tok = in.last_token();

   if ( in.last_token().len()==0 ) return in;

   // get first token and determine SrVar type:
   switch ( in.last_token_type() )
    { case SrInput::Name : 
           if ( sr_compare(tok,"true")==0 ) 
            { v.init('b'); v.set(true); break; }
           else if ( sr_compare(tok,"false")==0 )
            { v.init('b'); v.set(false); break; }
           else
            { // this will be a string, let enter the String case
            }

      case SrInput::String :
           v.init ('s');
           v.set ( (const char*)tok );
           break;

      case SrInput::Delimiter : // can be int or float type
           if ( tok[0]=='-' || tok[0]=='+' )
            { in.unget_token();
              SrString& s=in.getn();
              if ( in.last_token_type()==SrInput::Integer )
               { v.init('i'); v.set(s.atoi()); }
              else if ( in.last_token_type()==SrInput::Real )
               { v.init('f'); v.set(s.atof()); }
              break;
            }
      case SrInput::Integer :
           v.init ('i');
           v.set (tok.atoi());
           break;
      case SrInput::Real :
           v.init ('f');
           v.set (tok.atof());
           break;
      default : break;
    };

   // read while ';' is found:
   in.get_token ();
   if ( in.last_token().len()==0 ) return in;
   while ( in.last_token()[0]!=';' )
    { 
      switch ( v._type )
       { case 'f': in.unget_token();
                   v.push ( in.getn().atof() );
                   break;
         case 'i': in.unget_token();
                   v.push ( in.getn().atoi() );
                   break;
         case 's': v.push ( in.last_token() );
                   break;
         case 'b': if ( sr_compare(in.last_token(),"true")==0 ) 
                    v.push(true);
                   else 
                    v.push(false);
                   break;
       }
      in.get_token();
      if ( in.last_token().len()==0 ) return in;
    }

   return in;
 }

int sr_compare ( const SrVar* v1, const SrVar* v2 )
 {
   return sr_compare ( v1->name(), v2->name() );

/*
   int cmp = v1->type()-v2->type();
   if ( cmp!=0 ) return cmp;

   switch ( v1->type() )
    { case 'f': cmp = SR_COMPARE ( v1->getf(), v2->getf() ); break;
      case 'i': cmp = SR_COMPARE ( v1->geti(), v2->geti() ); break;
      case 's': cmp = sr_compare ( v1->gets(), v2->gets() ); break;
      case 'b': cmp = SR_COMPARE ( v1->getb(), v2->getb() ); break;
    }

   return cmp;
*/
 }

//================================ End of File =================================================
