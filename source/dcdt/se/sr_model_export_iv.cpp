#include "precompiled.h"
# include "sr_model.h"

//# define SR_USE_TRACE1    // keyword tracking
# include "sr_trace.h"


static void wcolor ( SrColor c, SrOutput& o )
 {
   float f[4];
   c.get(f);
   o << f[0] << srspc << f[1] << srspc << f[2];
 }

bool SrModel::export_iv ( const char* file )
 {
   int i;
   SrOutput o ( fopen(file,"wt") );
   if ( !o.valid() ) return false;

   // ===== Header =================================
   o << "#VRML V1.0 ascii\n\n";
   o << "Separator {\n\n";
   o << "  Info { string \"Produced by SR Toolkit exporter v0.5, M. Kallmann 2003\" }\n\n";

   // ===== Hints ==================================
   o << "  ShapeHints {\n";
   if ( culling ) o << "   shapeType SOLID\n";
   o << "   vertexOrdering COUNTERCLOCKWISE\n";
   o << "   faceType CONVEX\n";
   o << "  }\n\n";
   
   //===== Materials ===============================
   if ( M.size()==1 )
    { o << "  Material {\n";
      o << "  diffuseColor "; wcolor(M[0].diffuse,o); o<<srnl;
      o << "  }\n\n";
    }
   else if ( M.size()>1 )
    { o << "  Material {\n";
      o << "  diffuseColor [\n";
      for ( i=0; i<M.size(); i++ )
       { o<<srtab;
         wcolor(M[i].diffuse,o);
         if ( i<M.size()-1 ) o<<",\n"; else o<<"]\n";
       }
      o << "  }\n";
      o << "  MaterialBinding { value PER_FACE_INDEXED }\n\n";
    }
   
   //===== Coordinates ===============================
   o << "  Coordinate3 { point [\n";
   for ( i=0; i<V.size(); i++ )
    { o << srtab << V[i];
      if ( i<V.size()-1 ) o<<",\n"; else o<<"]\n";
    }
   o << "  }\n\n";

   //===== Faces ===============================
   o << "  IndexedFaceSet { coordIndex [\n";

   for ( i=0; i<F.size(); i++ )
    { o << srtab << F[i].a << ", " << F[i].b << ", " << F[i].c << ", -1";
      if ( i<F.size()-1 ) o<<",\n"; else o<<"]\n";
    }

   if ( M.size()>1 )
    { o << "    materialIndex [\n";
      for ( i=0; i<Fm.size(); i++ )
       { o << srtab << Fm[i];
         if ( i<Fm.size()-1 ) o<<",\n"; else o<<"]\n";
       }
    }

   o << "  }\n\n";

   //===== End ===============================

   o << "}\n\n";

   SR_TRACE1("Ok!");
   return true;
 }

//============================ EOF ===============================
