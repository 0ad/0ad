#include "precompiled.h"
# include "sr_sn_polygon_editor.h"

//============================ SrSnPolygonEditor ====================================

const char* SrSnPolygonEditor::class_name = "PolygonEditor";

SrSnPolygonEditor::SrSnPolygonEditor ()
             :SrSnEditor ( SrSnPolygonEditor::class_name )
 {
   _polygons = new SrSnSharedPolygons;
   child ( _polygons );

   _creation = new SrSnLines;
   _selection = new SrSnLines;

   helpers()->add ( _creation );
   helpers()->add ( _selection );

   _creation_color   = SrColor::red;
   _edition_color    = SrColor::green;
   _selection_color  = SrColor::black;
   _no_edition_color = SrColor::blue;

   _polygons->color ( _edition_color );
   _creation->color ( _creation_color );
   _selection->color ( _selection_color );

   _mode = ModeAdd;
   _draw_vertices = false;
   _precision_in_pixels = 5.0f;
   _precision = 0.05f;
   _selpol = _selvtx = -1;
   _max_polys = -1;
  
   _solid = true;
   _polygons->render_mode ( srRenderModeSmooth );
 }

SrSnPolygonEditor::~SrSnPolygonEditor ()
 {
 }

void SrSnPolygonEditor::init ()
 {
   _polygons->shape().init();
   _creation->shape().init();
   _selection->shape().init();
   _selpol = _selvtx = -1;
   if ( _mode!=ModeNoEdition && _mode!=ModeOnlyMove )
     _mode = ModeAdd;
 }

void SrSnPolygonEditor::set_polygons_to_share ( SrPolygons* p )
 {
   init ();
   _polygons->shape ( p );
 }

void SrSnPolygonEditor::solid ( bool b )
 {
   _solid=b;

   if ( _mode==ModeEdit )
    _polygons->render_mode ( _solid? srRenderModeFlat:srRenderModePoints );
   else
    _polygons->render_mode ( _solid? srRenderModeSmooth:srRenderModeLines );
 }

void SrSnPolygonEditor::set_creation_color ( SrColor c )
 {
   _creation_color = c;
   _creation->color ( _creation_color );
 }

void SrSnPolygonEditor::set_edition_color ( SrColor c )
 {
   _edition_color = c;
   _polygons->color ( _edition_color );
 }

void SrSnPolygonEditor::set_selection_color ( SrColor c )
 {
   _selection->changed (true);
   _selection_color = c;
 }

void SrSnPolygonEditor::set_no_edition_color ( SrColor c )
 {
   _no_edition_color = c;
 }

void SrSnPolygonEditor::set_maximum_number_of_polygons ( int i )
 {
   _max_polys = i;
 }

void SrSnPolygonEditor::mode ( SrSnPolygonEditor::Mode m )
 {
   SrPolygons& p = _polygons->shape();
   if ( m==ModeEdit && p.size()==0 ) m=ModeAdd;
   if ( m==ModeAdd && p.size()==_max_polys ) m=ModeEdit;

   if ( m==ModeEdit )
    _polygons->render_mode ( _solid? srRenderModeFlat:srRenderModePoints );
   else
    _polygons->render_mode ( _solid? srRenderModeSmooth:srRenderModeLines );

   if ( m==ModeNoEdition )
     _polygons->color ( _no_edition_color );
   else
     _polygons->color ( _edition_color );

   // clear edition nodes:
   if ( !_creation->const_shape().empty() ) _creation->shape().init();
   if ( !_selection->const_shape().empty() ) _selection->shape().init();
   _selpol = _selvtx = -1;
   
   // change mode:
   _mode = m;
 }

//==================================== private ====================================

bool SrSnPolygonEditor::pick_polygon_vertex ( const SrVec2& p )
 { 
   _selpol = _selvtx = -1;

   bool ret = _polygons->const_shape().pick_vertex ( p, _precision, _selpol, _selvtx );
   if ( ret==false ) return ret;

   //sr_out << "Vertex picked!\n";
   create_vertex_selection ( _selpol, _selvtx );

   return true;
 }

bool SrSnPolygonEditor::pick_polygon ( const SrVec2& p )
 { 
   _selvtx = -1;

   _selpol = _polygons->const_shape().pick_polygon ( p );
   if ( _selpol<0 ) return false;

   //sr_out << "Polygon picked!\n";
   create_polygon_selection ( _selpol );

   return true;
 }

bool SrSnPolygonEditor::subdivide_polygon_edge ( const SrVec2& p )
 { 
   if ( !_polygons->const_shape().pick_edge ( p, _precision, _selpol, _selvtx ) ) return false;

   //sr_out << "Edge picked!\n";
   SrPolygon& pol = _polygons->shape()[_selpol];
   _selvtx = (_selvtx+1)%pol.size();
   pol.insert ( _selvtx ) = p;
   create_vertex_selection ( _selpol, _selvtx );

   return true;
 }

void SrSnPolygonEditor::create_vertex_selection ( const SrVec2& p )
 {
   float r = _precision;

   r*=0.8f;
   SrVec2 v(r,r);
   SrLines& l = _selection->shape();
   l.init();
   l.push_color(_creation_color);
   l.push_line ( p-v, p+v ); v.x*=-1;
   l.push_line ( p-v, p+v );
 }

void SrSnPolygonEditor::create_vertex_selection ( int i, int j )
 {
   const SrVec2& p = _polygons->const_shape().const_get(i,j);
   float s = _precision;
   SrVec2 v ( p.x+s, p.y+s );

   _selection->color(_selection_color);
   SrLines& l = _selection->shape();
   l.init();
   l.begin_polyline();
   l.push_vertex ( v );
   v.y-=2*s; l.push_vertex ( v );
   v.x-=2*s; l.push_vertex ( v );
   v.y+=2*s; l.push_vertex ( v );
   v.x+=2*s; l.push_vertex ( v );
   l.end_polyline();
 }

void SrSnPolygonEditor::add_polygon_selection ( int i )
 {
   SrBox b;
   const SrPolygon& p = _polygons->shape().get(i);

 // this is to test the convex hull:
/*   SrPolygon hull;
   SrPolygon p = _polygons->shape().const_shape(i);
   p.convex_hull ( hull );
   p = hull;
*/
   SrLines& l = _selection->shape();
   l.begin_polyline();
   for ( i=0; i<p.size(); i++ ) l.push_vertex ( p.get(i) );
   if ( !p.open() ) l.push_vertex ( p.get(0) );
   l.end_polyline();
 }

void SrSnPolygonEditor::create_polygon_selection ( int i )
 {
   _selection->shape().init();
   _selection->color(_selection_color);
   add_polygon_selection ( i );
 }

void SrSnPolygonEditor::add_centroid_selection ( int i )
 {
   SrVec2 c, p;
   c = _polygons->const_shape().const_get(i).centroid();

   float r = _precision;
   r*=1.2f;
   p.set(r,r);
   _selection->shape().push_line ( c-p, c+p ); p.x*=-1;
   _selection->shape().push_line ( c-p, c+p );
 }

void SrSnPolygonEditor::translate_polygon ( int i, const SrVec2& lp, const SrVec2& p )
 {
   SrPolygon& pol = _polygons->shape()[i];
   pol.translate ( p-lp );
 }

void SrSnPolygonEditor::rotate_polygon ( int i, const SrVec2& lp, const SrVec2& p )
 {
   SrPolygon& pol = _polygons->shape()[i];
   SrVec2 cent = pol.centroid();

   float ang = angle_ori ( lp-cent, p-cent );
   pol.rotate ( cent, ang );
 }

void SrSnPolygonEditor::remove_selected_polygon ()
 {
   _selection->shape().init ();
   if ( _selpol<0 ) return;
   _polygons->shape().remove(_selpol);
   _selpol=_selvtx=-1;
   if ( _polygons->shape().size()==0 ) mode(ModeAdd);
 }

int SrSnPolygonEditor::handle_keyboard ( const SrEvent& e )
 {
   //sr_out<<"Key: "<<(int)e.key<<srnl;

   if ( e.key=='o' && _selpol>=0 )
    { SrPolygon& p = _polygons->shape()[_selpol];
      p.open ( !p.open() );
      _selvtx = -1;
      create_polygon_selection ( _selpol );
      return 1;
    }

   if ( e.key=='s' )
    { solid ( _solid? false:true );
      return 1;
    }

   switch ( _mode )
    { case ModeAdd:
       { if ( e.key==SrEvent::KeyDel || e.key==SrEvent::KeyBack )
          { SrLines& creat = _creation->shape();
            int size = _creation->shape().V.size();
            if ( size==0 ) return 0;
            creat.V.pop(); size--;
            creat.I[1] = size-1;
            if ( size==1 )
             create_vertex_selection ( SrVec2( (float*)creat.V.top()) );
            else
             _selection->shape().init();
            return 1;
          }
         else if ( e.key==SrEvent::KeyEsc )
          { int size = _creation->shape().V.size();
            if ( size<3 ) { mode(ModeEdit); return 1; }
            SrPolygons& p = _polygons->shape();
            p.push().size ( size );
            for ( int i=0; i<size; i++ )
              p.top()[i]=_creation->shape().V[i];
            mode ( ModeAdd );
            return 1; // tell that I understood the event
          }
       } break;

      case ModeEdit:
       { if ( e.key==SrEvent::KeyEsc ) { mode(ModeAdd); return 1; }
         if ( e.key==SrEvent::KeyDel && _selvtx>-1 )
          { SrPolygons& p = _polygons->shape();
            int size = p[_selpol].size();
            bool open = p[_selpol].open();
            if ( size<=2 || (size<=3 && !open) )
             { remove_selected_polygon(); }
            else
             { p[_selpol].remove(_selvtx);
               if (--_selvtx<0) _selvtx=size-2;
               create_vertex_selection ( _selpol, _selvtx );
             }
            return 1;
          }
       } break;

      case ModeMove:
       { if ( e.key==SrEvent::KeyEsc ) { mode(ModeEdit); return 1; }
         if ( e.key==SrEvent::KeyDel && _selpol>-1 )
          { remove_selected_polygon ();
            return 1;
          }
         if ( e.key==SrEvent::KeyEnd && _selpol>-1 && _polygons->const_shape().size()>1 )
          { SrPolygons& p = _polygons->shape();
            for ( int i=_selpol; i<p.size()-1; i++ ) p.swap(i,i+1);
            _selpol = p.size()-1;
            return 1;
          }
         if ( e.key==SrEvent::KeyHome && _selpol>-1 && _polygons->const_shape().size()>1 )
          { SrPolygons& p = _polygons->shape();
            for ( int i=_selpol; i>0; i-- ) p.swap(i,i-1);
            _selpol = 0;
            return 1;
          }
       } break;

    };
   return 0;
 }

int SrSnPolygonEditor::handle_only_move_event ( const SrEvent& e, const SrVec2& p, const SrVec2& lp )
 {
   if ( e.type==SrEvent::Push )
    { if ( pick_polygon_vertex(p) )
       { add_polygon_selection ( _selpol );
         add_centroid_selection ( _selpol );
         return 1;
       }
      else if ( pick_polygon(p) )
       { return 1; }
    }
   else if ( e.type==SrEvent::Drag && _selvtx>-1 )
    { rotate_polygon ( _selpol, lp, p );
      create_vertex_selection ( _selpol, _selvtx );
      add_polygon_selection ( _selpol );
      add_centroid_selection ( _selpol );
      return 1;
    }
   else if ( e.type==SrEvent::Drag && _selpol>-1 )
    { translate_polygon ( _selpol, lp, p );
      create_polygon_selection ( _selpol );
      return 1;
    }
   return 0; // event not used
 }

int SrSnPolygonEditor::handle_event ( const SrEvent &e )
 {
   //sr_out<<"SrSnPolygonEditor event: "<<e.mousep<<srnl; 
   //sr_out<<"scene dims: "<<e.scenew<<srspc<<e.sceneh<<srnl;

   if ( _mode==ModeNoEdition ) return 0;

   _precision = e.pixel_size*_precision_in_pixels;

   if ( e.type==SrEvent::Keyboard )
    return handle_keyboard(e);

   SrVec2 p ( e.mousep.x, e.mousep.y );
   SrVec2 lp ( e.lmousep.x, e.lmousep.y );

   if ( _mode==ModeOnlyMove ) return handle_only_move_event(e,p,lp);

   const SrPolygons& polys = _polygons->const_shape();
   switch ( _mode )
    { case ModeAdd:
       { SrLines& creat = _creation->shape();
         if ( e.type==SrEvent::Push )
          { int size = creat.V.size();

            if ( polys.size()==_max_polys )
             { mode ( ModeEdit ); }
            else if ( size==0 )
             { create_vertex_selection ( p );
               creat.push_vertex(p); // lines are not rendered with only 1 vertex
             }
            else if ( size==1 )
             { creat.I.size(2);
               creat.I[0]=0;
               creat.I[1]=1;
               creat.push_vertex(p);
               _selection->shape().init();
             }
            else
             { creat.I[1] = size;
               creat.push_vertex(p);
             }
            return 1; // tell that I understood the event
          }
       } break;

      case ModeEdit:
       { if ( e.type==SrEvent::Push )
          { if ( pick_polygon_vertex(p) )
             { }
            else if ( subdivide_polygon_edge(p) )
             { }
            else if ( pick_polygon(p) )
             { _mode=ModeMove; }
            else
             { if (_selection->const_shape().V.size()==0 ) break;
               _selection->shape().init();
             }
            return 1;
          }
         else if ( e.type==SrEvent::Drag && _selvtx>-1 )
          { _polygons->shape()(_selpol,_selvtx) = p;
            create_vertex_selection ( _selpol, _selvtx );
            return 1;
          }
       } break;

      case ModeMove: // case when a polygon has been selected
       { if ( e.type==SrEvent::Push )
          { int pol = _selpol;
            if ( pick_polygon_vertex(p) )
             { if ( _selpol!=pol )
                { _mode=ModeEdit; }
               else
                { add_polygon_selection ( _selpol );
                  add_centroid_selection ( _selpol );
                }
             }
            else if ( pick_polygon(p) )
             { }
            else
             { mode(ModeEdit); break; }
            return 1;
          }
         else if ( e.type==SrEvent::Drag && _selvtx>-1 )
          { rotate_polygon ( _selpol, lp, p );
            create_vertex_selection ( _selpol, _selvtx );
            add_polygon_selection ( _selpol );
            add_centroid_selection ( _selpol );
            return 1;
          }
         else if ( e.type==SrEvent::Drag && _selpol>-1 )
          { translate_polygon ( _selpol, lp, p );
            create_polygon_selection ( _selpol );
            return 1;
          }
       } break;

    };

   return 0; // event not used
 }

//================================ End of File =================================================

