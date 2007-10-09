#include "precompiled.h"
# include "sr_sa_eps_export.h"
# include "sr_sa_bbox.h"
# include "sr_sn_shape.h"
# include "sr_lines.h"
# include "sr_points.h"
# include "sr_polygons.h"


//# define SR_USE_TRACE1 // constructor / destructor
//# define SR_USE_TRACE2 // render
# include "sr_trace.h"

//============================= export functions ====================================

// detect if lines are in 3d or 2d!!?
// make use of current matrix (and camera) transformation

static void export_lines ( SrSnShapeBase* shape, SrOutput& o, const SrSaEpsExport* epsexp )
 {
   SrLines& l = ((SrSnLines*)shape)->shape();
   SrArray<SrPnt>&   V = l.V;
   SrArray<SrColor>& C = l.C;
   SrArray<int>&     I = l.I;
   float c[4];

   if ( V.size()<2 ) return;

   shape->material().diffuse.get ( c );
   o << c[0] << srspc << c[1] << srspc << c[2] << " setrgbcolor\n";

   float res = shape->resolution(); // 0.5 1.0 1.5
   res *= 0.02f;    // makes resolution 1 be .02 cms
   res *= 28.346f;  // put in unit pts
   res /= epsexp->scale_factor();
   o << res << " setlinewidth\n";

   int v=0;               // current V index
   int i;                 // current I index
   int imax = I.size()-1; // max value for i
   int i1=-1, i2;         // pair I[i],I[i+1]

   if ( I.size()>1 ) { i=0; i1=I[i]; i2=I[i+1]; }

   while ( v<V.size() )
    {
      if ( v==i1 )
       { if ( i2<0 ) // new color
          {  C[-i2-1].get ( c );
             o << c[0] << srspc << c[1] << srspc << c[2] << " setrgbcolor\n";
          }
         else if ( v<V.size() ) // new polyline
          { o << "newpath\n";
            o << V[v].x << srspc << V[v].y << " moveto\n";
            v++;
            while ( v<V.size() && v<=i2 )
             { o << V[v].x << srspc << V[v].y << " lineto\n";
               v++;
             }
            //o << "closepath\n";
            o << "stroke\n\n";
          }

         i+=2; // update next I information
         if ( i<imax ) { i1=I[i]; i2=I[i+1]; } else i1=-1;
       }
      else
       { o << "newpath\n";
         bool move=true;
         while ( v<V.size() && v!=i1 )
          { o << V[v].x << srspc << V[v].y;
            v++;
            if ( move )
             { o<<" moveto\n"; move=false; }
            else
             { o<<" lineto\n"; move=true; }
          }
         //o << "closepath\n";
         o << "stroke\n\n";
       }
    }
 }

static void export_points ( SrSnShapeBase* shape, SrOutput& o, const SrSaEpsExport* epsexp )
 {
/*
   SrPoints& p = ((SrSnPoints*)shape)->get();

   if ( p.P.size()==0 ) return;

   glDisable ( GL_LIGHTING );
   glColor ( shape->material().diffuse );

   if ( shape->render_mode()==srRenderModeSmooth )
    { // render shperes, with resolution as radius?
    }

   glPointSize ( shape->resolution() ); // default is 1.0

   int i;
   glBegin ( GL_POINTS );
   for ( i=0; i<p.P.size(); i++ )
    glVertex ( p.P[i] );
   glEnd ();
*/
 }

static void export_polygon ( SrPolygon& p, srRenderMode rm, float res, 
							 SrOutput& o, const SrSaEpsExport* epsexp )
 {
   int i;

       { o << "newpath\n";
         o << p[0] << " moveto\n";
         for ( i=1; i<p.size(); i++ ) o << p[i] << " lineto\n";
         if ( !p.open() ) o << p[0] << " lineto\n";
         o << "closepath\n";
         o << "stroke\n\n";
       }

/*

   if ( p.open() )
    { glBegin ( GL_LINE_STRIP );
      for ( i=0; i<p.size(); i++ ) glVertex ( p[i] );
      glEnd ();
    }
   else
    { if ( rm==srRenderModeSmooth || rm==srRenderModeFlat || rm==srRenderModeDefault )
       { SrArray<SrPnt2> tris;
         p.ear_triangulation ( tris );
         glBegin ( GL_TRIANGLES );
         for ( i=0; i<tris.size(); i++ ) glVertex ( tris[i] );
         glEnd ();
       }
      else
       { o << "newpath\n";
         o << p[0] << " moveto\n";
         for ( i=1; i<p.size(); i++ ) o << p[i] << " lineto\n";
         if ( !p.open() ) o << p[0] << " moveto\n";
         o << "closepath\n";
         o << "stroke\n\n";
       }
    }

   if ( rm==srRenderModePoints || rm==srRenderModeFlat )
    { glPointSize ( res*4 );
      glBegin ( GL_POINTS );
      for ( i=0; i<p.size(); i++ ) glVertex ( p[i] );
      glEnd ();
    }
*/
 }

static void export_polygon ( SrSnShapeBase* shape, SrOutput& o,
							 const SrSaEpsExport* epsexp )
 {
   SrPolygon& p = ((SrSnPolygon*)shape)->shape();
   if ( p.size()==0 ) return;

   float resolution = shape->resolution();

//   glDisable ( GL_LIGHTING );
//   glColor ( shape->material().diffuse );
//   glLineWidth ( resolution ); // default is 1.0

   export_polygon ( p, shape->render_mode(), resolution, o, epsexp );
 }

static void export_polygons ( SrSnShapeBase* shape, SrOutput& o, const SrSaEpsExport* epsexp )
 {
   SrPolygons& p = ((SrSnPolygons*)shape)->shape();

   if ( p.size()==0 ) return;

   float resolution = shape->resolution();
/*   glDisable ( GL_LIGHTING );
   glColor ( shape->material().diffuse );
   glLineWidth ( resolution ); // default is 1.0
*/
   int i;
   for ( i=0; i<p.size(); i++ ) ::export_polygon ( p[i], shape->render_mode(), resolution, o, epsexp );
 }

//=============================== SrSaEpsExport ====================================

SrArray<SrSaEpsExport::RegData> SrSaEpsExport::_efuncs;

SrSaEpsExport::SrSaEpsExport ( SrOutput& o ) 
               : _output ( o )
 { 
   SR_TRACE1 ( "Constructor" );

   _page_width = 21.59f;  // == 612 pts
   _page_height = 27.94f; // == 792 pts
   _page_margin = 4.0f;
   _bbox_margin = 0.1f;

   if ( _efuncs.size()==0 ) // no functions registered
    { //register_export_function ( "model",    export_model );
      register_export_function ( "lines",    export_lines );
      register_export_function ( "points",   export_points );
      //register_export_function ( "box",      export_box );
      //register_export_function ( "sphere",   export_sphere );
      register_export_function ( "polygon",  export_polygon );
      register_export_function ( "polygons", export_polygons );
    }
 }

SrSaEpsExport::~SrSaEpsExport ()
 {
   SR_TRACE1 ( "Destructor" );
 }

void register_export_function ( const char* class_name, SrSaEpsExport::export_function efunc ) // friend function
 {
   SrArray<SrSaEpsExport::RegData> &ef = SrSaEpsExport::_efuncs;

   int i;
   for ( i=0; i<ef.size(); i++ ) 
    { if ( sr_compare(ef[i].class_name,class_name)==0 ) break;
    }
   if ( i==ef.size() ) ef.push(); // if not found, push a new position

   ef[i].class_name = class_name;
   ef[i].efunc = efunc;
 }

bool SrSaEpsExport::apply ( SrSn* n )
 {
   // get bounding box
   SrSaBBox ab;
   ab.apply(n);
   SrBox bbox = ab.get();
   SrVec bboxsize = bbox.size();
   SrVec bboxcenter = bbox.center();

   // PostScript uses pts as default unit: 1inch==72pts, 1cm==28.346pts
   // so we put this transformation constant here:
   const float topts = 28.346f;

   // The required scaling so that the bounding box is inside the page
   // and respecting the desired margin:
   _scale_factor = _page_width / (bboxsize.x+2*_page_margin);
   _scale_factor *= topts;

   // The required translation to make the center of the bounding box
   // be in the middle of the page:
   _translation.set ( _page_width*topts/2  - bboxcenter.x*_scale_factor, 
	                  _page_height*topts/2 - bboxcenter.y*_scale_factor );

   // Output eps header
   _output << "%!PS-Adobe-3.0 EPSF-3.0\n";

   // Output the ebounding box:
   // The four arguments of the bounding box comment correspond to the lowerleft
   // and upper-right corners of the bounding box, expressed in the default 
   // PostScript coordinate system (pts) (28.346 passes from cms to pts).
   float a = ( bbox.a.x - _bbox_margin ) * _scale_factor + _translation.x;
   float b = ( bbox.a.y + _bbox_margin ) * _scale_factor + _translation.y;
   float c = ( bbox.b.x - _bbox_margin ) * _scale_factor + _translation.x;
   float d = ( bbox.b.y + _bbox_margin ) * _scale_factor + _translation.y;
   _output << "%%BoundingBox: " << int(a) << srspc << int(b) << srspc
	                            << int(c) << srspc << int(d) << srnl;

//   %!SrBoundingBox: -28.000000 -28.000000 28.000000 28.000000

//   %%BoundingBox: 124 123 734 623


   // Output other information:
   _output << "%%Creator: Sr EPS Exporter, Marcelo Kallmann 2002-2003\n";
   _output << "%!SrBoundingBox: " << bbox.a.x << srspc << bbox.a.y << srspc
	                              << bbox.b.x << srspc << bbox.b.y << srnl;

   _output << "%!\n\n";

   // Save the graphics state:
   _output << "gsave\n";


   // Output transformations so to work in cms and to fit the bbox in the page:
   _output << _translation << " translate\n";
   _output << _scale_factor << srspc << _scale_factor << " scale\n";

   // Other settings:
//   _output << "1 setlinewidth\n";
   _output << srnl;

   // Call the scene graph:
   bool result = SrSa::apply ( n );

   // Finalize:
   _output << "\n";
   _output << "grestore\n";
//   _output << "showpage\n";

   return result;
 }

//==================================== virtuals ====================================

void SrSaEpsExport::mult_matrix ( const SrMat& mat )
 {
   //glMultMatrix ( mat );
 }

void SrSaEpsExport::push_matrix ()
 {
   //glPushMatrix ();
 }

void SrSaEpsExport::pop_matrix ()
 {
   //glPopMatrix ();
 }

bool SrSaEpsExport::shape_apply ( SrSnShapeBase* s )
 {
   // 1. Search for export function
   int i;
   const char* class_name = s->inst_class_name ();
   SrArray<SrSaEpsExport::RegData>& ef = SrSaEpsExport::_efuncs;
   for ( i=0; i<ef.size(); i++ )
    if ( sr_compare(ef[i].class_name,class_name)==0 ) break;
   if ( i==ef.size() ) return true; // not found: nothing is done

   // 2. Export only if needed
   if ( !s->visible() ) return true;

   // 3. Export
   ef[i].efunc ( s, _output, this );
   return true;
 }

//======================================= PS GUIDE ====================================

/*
http://adela.karlin.mff.cuni.cz/netkarl/prirucky/Flat/paths.html

72 72 scale             points to inches, 1 pt = 1/72 inches
2.8346 2.8346 scale     points to mms
1.0 setlinewidth	0 is valid
2 setlinecap		0, 1, 2: butt ends, round ends, square ends
2 setlinejoin		0, 1, 2: miter corners, round corners, bevel corners
0.5 setgray		0 to 1

gsave
grestore


EXAMPLE:
%!

0.025 setlinewidth
72 72 scale
0.8 setgray
newpath
1 1 moveto
7 1 lineto
1 5 lineto
closepath
fill

0 setgray
newpath
1 1 moveto
7 1 lineto
1 5 lineto
closepath
stroke

showpage


*/

//======================================= EOF ====================================

