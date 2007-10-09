#include "precompiled.h"
# include "sr_string_array.h"
# include "sr_model.h"

//# define SR_USE_TRACE1    // keyword tracking
# include "sr_trace.h"

# define GETID(n,A) in>>n; if (n>0) n--; else if (n<0) n+=A.size()
 
static void get_face ( SrInput& in, SrModel& m, int& vc, int& vt, int& vn )
 {
   vc = vt = vn = -1;
   GETID(vc,m.V);

   if ( in.get_token()!=SrInput::Delimiter ) // was only: vc
    { in.unget_token(); return; }

   if ( in.get_token()==SrInput::Delimiter ) // is: vc//vn
    { GETID(vn,m.N);
      return;
    }

   in.unget_token();

   GETID(vt,m.T); // is: vc/vt

   if ( in.get_token()==SrInput::Delimiter )
    { GETID(vn,m.N); // is: vc/vt/vn
      return;
    }

   if ( !in.finished() ) in.unget_token (); // is only: vc/vt
 }

static SrColor read_color ( SrInput& in )
 {
   float r, g, b;
   in >> r >> g >> b;
   SrColor c(r,g,b);
   return c;
 }

static void read_materials ( SrArray<SrMaterial>& M,
                             SrStringArray& mnames,
                             const SrString& file,
                             const SrStringArray& paths )
 {
   SrString s;
   SrInput in;

   in.init ( fopen(file,"rt") );
   int i=0;
   while ( !in.valid() && i<paths.size() )
    { s = paths[i++];
      s << file;
      in.init ( fopen(s,"rt") );
    }
   if ( !in.valid() ) return; // could not get materials

   while ( !in.finished() )
    { in.get_token();
      if ( in.last_token()=="newmtl" )
       { M.push().init();
         in.get_token();
         SR_TRACE1 ( "new material: "<<in.last_token() );
         mnames.push ( in.last_token() );
       }
      else if ( in.last_token()=="Ka" )
       { M.top().ambient = read_color ( in );
       }
      else if ( in.last_token()=="Kd" )
       { M.top().diffuse = read_color ( in );
       }
      else if ( in.last_token()=="Ks" )
       { M.top().specular = read_color ( in );
       }
      else if ( in.last_token()=="Ke" ) // not sure if this one exists
       { M.top().emission = read_color ( in );
       }
      else if ( in.last_token()=="Ns" )
       { in >> i;
         M.top().shininess = i;
       }
      else if ( in.last_token()=="illum" ) // dont know what is this one
       { in >> i;
       }
    }
 }

static bool process_line ( const SrString& line,
                           SrModel& m,
                           SrStringArray& paths,
                           SrStringArray& mnames,
                           int& curmtl )
 {
   SrInput in (line);
   in.get_token();

   if ( in.last_token().len()==0 ) return true;

   if ( in.last_token()=="v" ) // v x y z [w]
    { SR_TRACE1 ( "v" );
      m.V.push();
      in >> m.V.top();
    }
   else if ( in.last_token()=="vn" ) // vn i j k
    { SR_TRACE1 ( "vn" );
      m.N.push();
      in >> m.N.top();
    }
   else if ( in.last_token()=="vt" ) // vt u v [w]
    { SR_TRACE1 ( "vt" );
      m.T.push();
      in >> m.T.top();
    }
   else if ( in.last_token()=="g" )
    { SR_TRACE1 ( "g" );
    }
   else if ( in.last_token()=="f" ) // f v/t/n v/t/n v/t/n (or v/t or v//n or v)
    { SR_TRACE1 ( "f" );
      int i=0;
      SrArray<int> v, t, n;
      v.capacity(8); t.capacity(8); n.capacity(8);
      while ( true )
       { if ( in.get_token()==SrInput::EndOfFile ) break;
         in.unget_token();
         get_face ( in, m, v.push(), t.push(), n.push() );
         if ( in.last_error()==SrInput::UnexpectedToken ) return false;
       }

      if ( v.size()<3 ) return false;

      for ( i=2; i<v.size(); i++ )
       { m.F.push().set ( v[0], v[i-1], v[i] );
         m.Fm.push() = curmtl;

         if ( t[0]>=0 && t[1]>=0 && t[i]>=0 )
          m.Ft.push().set( t[0], t[i-1], t[i] );

         if ( n[0]>=0 && n[1]>=0 && n[i]>=0 )
          m.Fn.push().set ( n[0], n[i-1], n[i] );
       }
    }
   else if ( in.last_token()=="s" ) // smoothing groups not supported
    { SR_TRACE1 ( "s" );
      in.get_token();
    }
   else if ( in.last_token()=="usemap" ) // usemap name/off
    { SR_TRACE1 ( "usemap" );
    }
   else if ( in.last_token()=="usemtl" ) // usemtl name
    { SR_TRACE1 ( "usemtl" );
      in.get_token ();
      curmtl = mnames.lsearch ( in.last_token() );
      SR_TRACE1 ( "curmtl = " << curmtl << " (" << in.last_token() << ")" );
    }
   else if ( in.last_token()=="mtllib" ) // mtllib file1 file2 ...
    { SR_TRACE1 ( "mtllib" );
      SrString token, file;
      int pos = line.get_next_string ( token, 0 ); // parse again mtllib
      while ( true )
       { pos = line.get_next_string ( token, pos );
         if ( pos<0 ) break;
         token.extract_file_name ( file );
         token.replace ( "\\", "/" ); // avoid win-unix problems
         SR_TRACE1 ( "new path: "<<token );
         paths.push ( token );
         read_materials ( m.M, mnames, file, paths );
       }
    }

   return true;
 }

/*! This function import the .obj format. If the import
    is succesfull, true is returned
    and the model m will contain the imported object, otherwise false
    is returned. */
bool SrModel::import_obj ( const char* file )
 {
   SrInput in ( fopen(file,"ra") );
   if ( !in.valid() ) return false;

   in.comment_style ( '#' );
   in.lowercase_tokens ( false );

   SrString path=file;
   SrString filename;
   path.extract_file_name(filename);
   SrStringArray paths;
   paths.push ( path );
   SR_TRACE1 ( "First path:" << path );

   int curmtl = -1;

   init ();
   name = filename;
   name.remove_file_extension();

   SrString line;
   while ( in.getline(line)!=EOF )
    { if ( !process_line(line,*this,paths,mtlnames,curmtl) ) return false;
    }

   validate ();
   remove_redundant_materials ();
   remove_redundant_normals ();
   compress ();

   SR_TRACE1("Ok!");
   return true;
 }

//============================ EOF ===============================
