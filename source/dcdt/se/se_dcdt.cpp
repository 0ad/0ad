#include "precompiled.h"

# include <math.h>
# include <stdlib.h>
# include <string.h>
# include "sr_box.h"
# include "sr_geo2.h"
# include "se_dcdt.h"

//# define SR_USE_TRACE1  // insert polygon
//# define SR_USE_TRACE2  // init
//# define SR_USE_TRACE3  // insert polygon
//# define SR_USE_TRACE4  // remove polygon
//# define SR_USE_TRACE5  // funnel
//# define SR_USE_TRACE6  // translate
//# define SR_USE_TRACE7  // ray
//# define SR_USE_TRACE8  // tri manager
//# define SR_USE_TRACE9  // extract contours
# include "sr_trace.h"

//DJD: declaring static member variables {

template <class T>
SrArray<SeLinkFace<T> *> SeLinkFace<T>::processing;
template <class T>
int SeLinkFace<T>::current = 0;

//declaring static member variables }

//=============================== SeDcdt ==================================

SeDcdt::SeDcdt () 
 { 
   _mesh = new SeDcdtMesh;
 
   _triangulator = new SeTriangulator 
                 ( SeTriangulator::ModeConstrained,
                   _mesh,
                   new SeDcdtTriManager,
                   0.000001 /*epsilon*/ );

   _first_symedge = 0;
   _cur_search_face = 0;
   _xmin = _xmax = _ymin = _ymax = 0;
   _radius = -1;
 }

SeDcdt::~SeDcdt () 
 {
   delete _triangulator;
   delete _mesh;
 }

void SeDcdt::get_mesh_edges ( SrArray<SrPnt2>* constr, SrArray<SrPnt2>* unconstr )
 {
   SeDcdtEdge *e, *ei;
   SeDcdtSymEdge *s;
   SrArray<SrPnt2>* pa;

   if ( constr ) constr->size(0);
   if ( unconstr ) unconstr->size(0);
   
   e = ei = _mesh->first()->edg();

   do { if ( e->is_constrained() ) pa=constr; else pa=unconstr;
        if ( pa )
         { s = e->se();  pa->push() = s->vtx()->p;
           s = s->nxt(); pa->push() = s->vtx()->p;
         }
        e = e->nxt();
      } while ( e!=ei );
 }

//================================================================================
//================================ save ==========================================
//================================================================================

bool SeDcdt::save ( SrOutput& out )
 {
   int elems = _polygons.elements();

   out << "SeDcdt\n\n";
   out << "# domain:" << (elems>0?1:0) << " polygons:" << (elems-1) << srnl << srnl;
   out << "epsilon " << _triangulator->epsilon() << srnl << srnl;

   if ( _radius!=-1 )
     out << "init_radius " << _radius << srnl << srnl;

   if ( elems==0 ) return true;

   int i, id, psize;
   int maxid = _polygons.maxid();
   for ( id=0; id<=maxid; id++ )
    { if ( !_polygons[id] ) continue;

      SeDcdtInsPol& p = *_polygons[id];
   
      if ( id==0 )
       { out << "domain\n"; }
      else
       { out << "polygon " << id;
         if ( p.open ) out << " open";
         out << srnl;
       }

      psize = p.size();
      for ( i=0; i<psize; i++ )
       { out << p[i]->p;
         if ( i==psize-1 ) out<<';'; else out<<srspc;
       }
      out << srnl << srnl;
    }

   return true;
 }

//================================================================================
//================================ load ==========================================
//================================================================================

static void _read_pol ( SrInput& inp, SrPolygon& pol )
 {
   pol.size ( 0 );
   while ( 1 )
    { inp.get_token();
      if ( inp.last_token()==';' ) break;
      inp.unget_token ();
      inp >> pol.push();
    }
 }

bool SeDcdt::load ( SrInput& inp )
 {
   SrPolygon pol;
   pol.capacity ( 64 );

   SrArray<int> invids;

   float epsilon = 0.00001f;
   float radius = -1.0f;
   int id, nextid=1;

   // signature:
   inp.comment_style ( '#' );
   inp.get_token ();
   if ( inp.last_token()!="SeDcdt") return false;

   while ( 1 )
    { inp.get_token ();
      if ( inp.finished() ) break;
      SrString& s = inp.last_token();
 
      if ( s=="epsilon" )
       { inp >> epsilon;
       }
      if ( s=="init_radius" )
       { inp >> radius;
       }
      else if ( s=="domain" )
       { _read_pol ( inp, pol );
         pol.open ( false );
         init ( pol, epsilon, radius );
       }
      else if ( s=="polygon" )
       { if ( num_polygons()==0 ) return false;
         inp >> id;
         while ( id>nextid ) { invids.push()=_polygons.insert(); nextid++; }
         inp.get_token();
         if ( inp.last_token()=="open" )
          { pol.open(true); }
         else
          { inp.unget_token();
            pol.open(false);
          }
         _read_pol ( inp, pol );
         insert_polygon ( pol );
         nextid++;
       }
    }

   while ( invids.size() ) _polygons.remove(invids.pop());

   return true;
 }

//================================================================================
//================================ init ==========================================
//================================================================================

void SeDcdt::init ( const SrPolygon& domain , float epsilon, float radius )
 {
   SR_TRACE2 ( "Starting init()..." );

   // Reset cur searched face:
   _cur_search_face = 0;

   // Clear structures if needed:
   if ( _first_symedge ) 
    { _mesh->destroy(); _first_symedge=0; _polygons.init(); }

   // Checks parameters:
   if ( domain.size()<3 || domain.open() )
     sr_out.fatal_error ( "SeDcdt::init(): domain polygon must be simple and closed!");

   // Calculates an external polygon (the border) :
   SR_TRACE2 ( "Calculating External Polygon for domain with "<<domain.size()<<" points..." );
   SrBox b;
   domain.get_bounding_box ( b );
   _xmin=b.a.x; _xmax=b.b.x; _ymin=b.a.y; _ymax=b.b.y;
   float dx = (_xmax-_xmin)/5.0f;
   float dy = (_ymax-_ymin)/5.0f;
   if ( radius>0 )
    { if ( dx<radius ) dx=radius;
      if ( dy<radius ) dy=radius;
    }
   if ( radius!=0 )
    { _xmax+=dx; _xmin-=dx; _ymax+=dy; _ymin-=dy; }

   // Creates the triangulated external square (the border):
   SR_TRACE2 ( "Creating External Polygon..." );
   _triangulator->epsilon ( epsilon );
   _first_symedge = (SeDcdtSymEdge*)_triangulator->init_as_triangulated_square
                    ( _xmin, _ymin, _xmax, _ymin, _xmax, _ymax, _xmin, _ymax );
   _radius = radius;

   // Inserts the vertices of the domain according to radius:
   if ( radius==0 )
    { _using_domain = false;
    }
   else
    { _using_domain = true;
      insert_polygon ( domain );
    }
 }

void SeDcdt::get_bounds ( float& xmin, float& xmax, float& ymin, float& ymax ) const
 {
   xmin=_xmin; xmax=_xmax; ymin=_ymin; ymax=_ymax;
 }

//================================================================================
//=========================== insert polygon ====================================
//================================================================================

int SeDcdt::insert_polygon ( const SrPolygon& pol )
 {
   int i, i1, id;
   SeVertex* v;
   SeFace* sface;

   SR_TRACE1 ( "Inserting entry in the polygon set..." ); // put in _polygons
   id = _polygons.insert();
   SeDcdtInsPol& ip = *_polygons[id];
   ip.open = pol.open();

   SR_TRACE1 ( "Inserting polygon points..." ); // insert vertices
   sface = _search_face();
   for ( i=0; i<pol.size(); i++ )
    { v = _triangulator->insert_point ( pol[i].x, pol[i].y, sface );
      if ( !v ) sr_out.fatal_error ( "Search failure in _insert_polygon()." );
      ip.push() = (SeDcdtVertex*)v;
      sface = v->se()->fac();
    }

   // should we keep here collinear vertices (which are not corners)? 
   // they can be removed as Steiner vertices when removing another intersecting polygon
 
   _cur_search_face=0; // Needed because edge constraint may call kef

   SR_TRACE1 ( "Inserting polygon edges constraints..." ); // insert edges
   for ( i=0; i<ip.size(); i++ )
    { i1 = (i+1)%ip.size();
      if ( i1==0 && ip.open ) break; // do not close the polygon
      if ( !_triangulator->insert_line_constraint ( ip[i], ip[i1], id ) ) 
        sr_out.fatal_error ( "Unable to insert constraint in _insert_polygon()." );
    }

   return id;
 }

int SeDcdt::polygon_max_id () const
 {
   return _polygons.maxid();
 }

int SeDcdt::num_polygons () const
 {
   return _polygons.elements();
 }

//================================================================================
//=========================== remove polygon ====================================
//================================================================================

void SeDcdt::_find_intermediate_vertices_and_edges ( SeDcdtVertex* vini, int oid )
 {
   SeDcdtEdge *e;
   SeDcdtVertex *v;
   SeDcdtSymEdge *s, *si;

   _mesh->begin_marking ();
   _varray.size(0); _earray.size(0); _stack.size(0);

   // initialize stack with all constrained edges from v:
   _varray.push() = vini;
   _mesh->mark ( vini );
   s = si = vini->se();
   do { e = s->edg();
        if ( e->has_id(oid) )
         { _stack.push() = s;
           _earray.push() = s;
           _mesh->mark ( e );
         }
        s = s->rot();
      } while ( s!=si );

   // advances untill all edges are reached:
   while ( _stack.size() )
    { s = si = _stack.pop()->nxt();
      v = s->vtx();
      if ( !_mesh->marked(v) )
       { _varray.push() = v;
         _mesh->mark ( v );
       }
      do { e = s->edg();
           if ( !_mesh->marked(e) && e->has_id(oid) )
            { _stack.push() = s;
              _earray.push() = s;
              _mesh->mark ( e );
            }
           s = s->rot();
         } while ( s!=si );
    }

   _mesh->end_marking ();
 }

bool SeDcdt::_is_intersection_vertex
             ( SeDcdtVertex* v, int id, SeDcdtVertex*& v1, SeDcdtVertex*& v2 )
 {
   SeDcdtSymEdge *si, *s;
   v1 = v2 = 0;

   // at this point, it is guaranteed that only two constraints with index id
   // are adjacent to v, because there is a test that ensures that only two
   // constrained edges are incident to v in _remove_vertex_if_possible().
   // (however these two constrained edges may have more than one id)

   si = s = v->se();
   do { if ( s->edg()->has_id(id) )
         { if ( v1==0 ) v1=s->nxt()->vtx();
            else if ( v2==0 ) { v2=s->nxt()->vtx(); break; }
         }
        s = s->rot();
      } while ( s!=si );

   if ( v1==0 || v2==0 ) return false; // v is an extremity of an open constraint

   double d = sr_point_segment_dist ( v->p.x, v->p.y, v1->p.x, v1->p.y, v2->p.x, v2->p.y );
   return d<=_triangulator->epsilon()? true:false;
 }

void SeDcdt::_remove_vertex_if_possible ( SeDcdtVertex* v, const SrArray<int>& vids )
 {
   SR_TRACE4 ( "Try to remove vertex with ref="<<vids.size()<<"..." );

   // Easier case, vertex is no more used:
   if ( vids.size()==0 ) // dangling vertex
    { SR_TRACE4 ( "Removing dangling vertex" );
      _triangulator->remove_vertex(v);
      return;
    }

   int i;
   int num_of_incident_constrained_edges=0;
   SeDcdtSymEdge *s;
   s = v->se();
   do { if ( s->edg()->is_constrained() ) num_of_incident_constrained_edges++;
        s = s->rot();
      } while ( s!=v->se() );

   if ( num_of_incident_constrained_edges!=2 ) return;

   // test if all polygons using the vertex no more need it:
   SeDcdtVertex *v1, *v2;
   SrArray<SeDcdtVertex*>& va = _varray2; // internal buffer
   va.size ( 0 );

   // (note that we can have vids.size()>1 at this point)
   for ( i=0; i<vids.size(); i++ )
    { if ( !_is_intersection_vertex(v,vids[i],v1,v2) )
       { SR_TRACE4 ( "Not an intersection vertex!" );
         return;
       }
      va.push()=v1; va.push()=v2;
    }

   SR_TRACE4 ( "Removing one intersection vertex..." );
   _triangulator->remove_vertex(v);

   // and recover all deleted constraints:
   for ( i=0; i<vids.size(); i++ )
    { _triangulator->insert_line_constraint ( va[i*2], va[i*2+1], vids[i] );
    }
 }

void SeDcdt::remove_polygon ( int polygonid )
 {
   int i;
   SR_TRACE4 ( "Entering remove_polygon..." );

   if ( polygonid==0 )
     sr_out.fatal_error("Domain cannot be removed by SeDcdt::remove_polygon()");

   if ( polygonid<0 || polygonid>_polygons.maxid() )
     sr_out.fatal_error("Invalid id sent to SeDcdt::remove_polygon()");

   if ( !_polygons[polygonid] )
     sr_out.fatal_error("SeDcdt::remove_polygon(): polygon already removed");

   // _search_face can be invalidated so make it unavailable:
   _cur_search_face = 0;

   // Recuperate intermediate vertices inserted as Steiner points:
   SR_TRACE4 ( "Recuperating full polygon..." );
   _find_intermediate_vertices_and_edges ( _polygons[polygonid]->get(0), polygonid ); 
   SR_TRACE4 ( "polygon has "<<_varray.size()<<" vertices " << "and "<<_earray.size()<<" edges." ); 

   // Remove all ids which are equal to polygonid in the edge constraints
   SR_TRACE4 ( "Updating edge constraints..." );
   for ( i=0; i<_earray.size(); i++ )
    { _earray[i]->edg()->remove_id ( polygonid ); // remove all occurences
    }

   SR_TRACE4 ( "Removing free vertices..." );
   for ( i=0; i<_varray.size(); i++ )
    { _varray[i]->get_references ( _ibuffer ); //vobs
      SR_TRACE4 ( "Vertex "<<i<<": (" << _varray[i]->p << ")" );
      _remove_vertex_if_possible ( _varray[i], _ibuffer );
    }

   _polygons.remove ( polygonid );
   SR_TRACE4 ( "Finished." );
 }

//================================================================================
//============================= get polygon ======================================
//================================================================================

void SeDcdt::get_polygon ( int polygonid, SrPolygon& polygon )
 { 
   polygon.size(0);

   if ( polygonid<0 || polygonid>_polygons.maxid() ) return;
   if ( !_polygons[polygonid] ) return;

   SeDcdtInsPol& p = *_polygons[polygonid];
   
   int i;
   polygon.size ( p.size() );
   polygon.open ( p.open? true:false );
   for ( i=0; i<p.size(); i++ )
    polygon[i] = p[i]->p;
 }

void SeDcdt::get_triangulated_polygon ( int polygonid, SrArray<SeDcdtVertex*>* vtxs, SrArray<SeDcdtEdge*>* edgs )
 { 
   if ( polygonid<0 || polygonid>_polygons.maxid() ) return;
   if ( !_polygons[polygonid] ) return;

   _find_intermediate_vertices_and_edges ( _polygons[polygonid]->get(0), polygonid ); 

   if ( vtxs ) *vtxs = _varray;

   if ( edgs ) // need to convert symedges to edges
    { int i;
      edgs->size ( _earray.size() );
      for ( i=0; i<edgs->size(); i++ ) (*edgs)[i] = _earray[i]->edg();
    }
 }

//================================================================================
//=========================== ray intersection ===================================
//================================================================================

static bool _intersects ( double p1x, double p1y, double p2x, double p2y, SeDcdtSymEdge* s )
 {
   SeDcdtVertex* v1 = s->vtx();
   SeDcdtVertex* v2 = s->nxt()->vtx();
   return sr_segments_intersect ( p1x, p1y, p2x, p2y,
                                  v1->p.x, v1->p.y, v2->p.x, v2->p.y );
 }

void SeDcdt::ray_intersection ( float x1, float y1, float x2, float y2,
                                SrArray<int>& polygons, int depth )
 {
   SeTriangulator::LocateResult res;
   SeBase *ses;
   SeDcdtSymEdge *s;

   polygons.size(0);
   if ( depth==0 ) return;

   double p1x=x1, p1y=y1, p2x=x2, p2y=y2;

   res = _triangulator->locate_point ( _search_face(), p1x, p1y, ses );
   if ( res==SeTriangulator::NotFound )
    { SR_TRACE7 ( "First point not found!" );
      return;
    }

   s = (SeDcdtSymEdge*)ses;
   _cur_search_face = s->fac();

   // Find first intersection with the first triangle:
   int i=0;
   while ( !_intersects(p1x,p1y,p2x,p2y,s) )
    { s=s->nxt();
      if ( i++==3 )
       { SR_TRACE7 ( "None intersections, ray in triangle!" );
         return;
       }
    }

   // Continues until end:
   while ( true )
    { if ( s->edg()->is_constrained() )
       { //int r = rand() % e->ids_size;
         SR_TRACE7 ( "found one!" );
         SrArray<int>& a = s->edg()->ids;
         for ( i=0; i<a.size(); i++ )
          if ( polygons.size()<depth ) polygons.push()=a[i];
         if ( depth>0 && polygons.size()>=depth )
           { SR_TRACE7 ( "depth reached!" ); return; }
       }
      s = s->sym();
      if ( _intersects(p1x,p1y,p2x,p2y,s->nxt()) ) s=s->nxt();
       else
      if ( _intersects(p1x,p1y,p2x,p2y,s->pri()) ) s=s->pri();
       else
      { SR_TRACE7 ( "Finished Ray!" ); return; }
    }
 }

void SeDcdt::_add_contour ( SeDcdtSymEdge* s, SrPolygon& vertices, SrArray<int>& pindices )
 {
   SeDcdtSymEdge* si=s;
   pindices.push() = vertices.size();

   SR_TRACE9 ( "Begin contour: "<<pindices.top() );
   do { vertices.push() = s->vtx()->p;
        _mesh->mark ( s->edg() );         
        do { s=s->rot(); } while ( !s->edg()->is_constrained() );
        s = s->sym();
      } while ( s!=si );

   pindices.push() = vertices.size()-1;
   SR_TRACE9 ( "End contour: "<<pindices.top() );
 }

SeDcdtSymEdge* SeDcdt::_find_one_obstacle ()
 {
   SeDcdtSymEdge *s;

   while ( 1 )
    { if ( _earray.empty() ) break;

      s = _earray.pop();
      if ( _mesh->marked(s->edg()) ) continue;
      if ( s->edg()->is_constrained() ) return s;
      _mesh->mark(s->edg());

      s = s->sym()->nxt();
      if ( !_mesh->marked(s->edg()) ) { _earray.push()=s; }
      s = s->nxt();
      if ( !_mesh->marked(s->edg()) ) { _earray.push()=s; }
    }

   return 0;
 }

void SeDcdt::extract_contours ( SrPolygon& vertices, SrArray<int>& pindices, float x, float y )
 {
   SeTriangulator::LocateResult res;
   SeBase *ses;

   vertices.size(0);
   pindices.size(0);

   SR_TRACE9 ( "Begin Extract" );

   res = _triangulator->locate_point ( _search_face(), x, y, ses );
   if ( res==SeTriangulator::NotFound ) return;

   _mesh->begin_marking();
   _earray.size(0);
   SeDcdtSymEdge *s, *si;
   s = si = (SeDcdtSymEdge*)ses;
   do { _earray.push() = s;
        s = s->nxt();
      } while ( s!=si );

   while (1)
    { s = _find_one_obstacle ();
      if ( !s ) break;
      _add_contour ( s, vertices, pindices );
    }

   _mesh->end_marking();
   SR_TRACE9 ( "End Extract" );
 }

//================================================================================
//=========================== inside polygon ====================================
//================================================================================

static int interhoriz ( float px, float py, float p1x, float p1y, float p2x, float p2y )
 {
   if ( p1y>p2y ) { float tmp; SR_SWAP(p1x,p2x); SR_SWAP(p1y,p2y); } // swap
   if ( p1y>=py ) return false; // not intercepting
   if ( p2y<py  ) return false; // not intercepting or = max 
   float x2 = p1x + (py-p1y) * (p2x-p1x) / (p2y-p1y);
   return (px<x2)? true:false;
 }

int SeDcdt::inside_polygon ( float x, float y , SrArray<int>* allpolys )
 {
   if ( _polygons.elements()<=1 ) return -1; // if there is only the domain, return.

   int cont=0, i, size;
   SeDcdtInsPol* pol;
   SeDcdtVertex *v1, *v2;

   if ( allpolys ) allpolys->size(0);

   int polid=0;
   if ( _using_domain ) polid++;
   for ( ; polid<=_polygons.maxid(); polid++ )
    { pol = _polygons[polid];
      if ( !pol ) continue;

      size = pol->size();
      for ( i=0; i<size; i++ )
       { v1 = pol->get(i);
         v2 = pol->get( (i+1)%size );
         cont ^= interhoriz ( x, y, v1->p.x, v1->p.y, v2->p.x, v2->p.y );
       }
      if (cont) 
       { if ( allpolys ) allpolys->push()=polid;
          else return polid;
       }
    }

   if ( allpolys ) if ( allpolys->size()>0 ) return allpolys->get(0);
   return -1;
 }

int SeDcdt::pick_polygon ( float x, float y )
 {
   if ( _polygons.elements()==0 ) return -1;
   if ( _using_domain && _polygons.elements()==1 ) return -1; // if there is only the domain, return.

   SeTriangulator::LocateResult res;
   SeBase *sse;

   res = _triangulator->locate_point ( _search_face(), x, y, sse );
   if ( res==SeTriangulator::NotFound ) return -1;

   SeDcdtSymEdge *s = (SeDcdtSymEdge*)sse;
   SrPnt2 pt(x,y);
   float d1 = dist2 ( pt, s->vtx()->p );
   float d2 = dist2 ( pt, s->nxt()->vtx()->p );
   float d3 = dist2 ( pt, s->nxt()->nxt()->vtx()->p );
   if ( d2<=d1 && d2<=d3 ) s=s->nxt();
   if ( d3<=d1 && d3<=d2 ) s=s->nxt()->nxt();

   SeDcdtSymEdge* k=s;
   do { SrArray<int>& ids = k->edg()->ids;
        if ( ids.size()>0 ) { if ( !_using_domain || ids[0]>0 ) return ids[0]; } // 0 is the domain
        k=k->rot();
      } while ( k!=s );

   return -1;
 }

//================================================================================
//=============================== search path ======================================
//================================================================================

bool SeDcdt::search_path ( float x1, float y1, float x2, float y2, 
                           const SeDcdtFace* iniface, bool vistest )
 {
   // fast security test to ensure at least that points are not outside the border limits:
   if ( x1<_xmin || x1>_xmax || x2<_xmin || x2>_xmax ) return false;

   if ( !iniface ) iniface = _search_face();

   bool found = _triangulator->search_path ( x1, y1, x2, y2, iniface, vistest );

   // to optimize searching for next queries around the same point,
   // set the next starting search face to the first channel face:
   if ( _triangulator->get_channel_interior_edges().size()>0 )
     _cur_search_face = (SeDcdtFace*)_triangulator->get_channel_interior_edges()[0]->fac();

   return found;
 }

//============================ End of File ===============================

