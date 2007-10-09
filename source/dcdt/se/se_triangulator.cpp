#include "precompiled.h"

# include <math.h>
# include <stdlib.h>

# include "sr_geo2.h"
# include "sr_heap.h"
# include "sr_deque.h"

# include "se_triangulator.h"

//# define SR_TRACE_NO_FILENAME

//# define SR_USE_TRACE1 // locate point
//# define SR_USE_TRACE2 // ins delaunay point
//# define SR_USE_TRACE3 // ins line constraint
//# define SR_USE_TRACE4 // triangulate face
//# define SR_USE_TRACE5 // search path
//# define SR_USE_TRACE6 // remove vertex
//# define SR_USE_TRACE7 // shortest path (funnel)
# include "sr_trace.h"

//=========================== debugging tools ====================================

# define PNL printf("\n")

# define PRINTP(x,y)  printf("(%+9.7f,%+9.7f) ", x, y)

# define PRINTV(v)  { double x, y; \
                      _man->get_vertex_coordinates ( v, x, y ); \
                      PRINTP(x,y); }

//================================================================================
//========================= SeTriangulatorManager ================================
//================================================================================

void SeTriangulatorManager::triangulate_face_created_edge ( SeEdge* /*e*/ )
 {
 }

void SeTriangulatorManager::new_intersection_vertex_created ( SeVertex* /*v*/ )
 {
 }

void SeTriangulatorManager::new_steiner_vertex_created ( SeVertex* /*v*/ )
 {
 }

void SeTriangulatorManager::vertex_found_in_constrained_edge ( SeVertex* /*v*/ )
 {
 }

bool SeTriangulatorManager::is_constrained ( SeEdge* /*e*/ )
 {
   return false;
 }

void SeTriangulatorManager::set_unconstrained ( SeEdge* /*e*/ )
 {
 }

void SeTriangulatorManager::get_constraints ( SeEdge* /*e*/, SrArray<int>& /*ids*/ )
 {
 }

void SeTriangulatorManager::add_constraints ( SeEdge* /*e*/, const SrArray<int>& /*ids*/ )
 {
 }

bool SeTriangulatorManager::ccw ( SeVertex* v1, SeVertex* v2, SeVertex* v3 )
 {
   double x1, y1, x2, y2, x3, y3;
   get_vertex_coordinates ( v1, x1, y1 );
   get_vertex_coordinates ( v2, x2, y2 );
   get_vertex_coordinates ( v3, x3, y3 );
   return sr_ccw ( x1, y1, x2, y2, x3, y3 )>0? true : false;
 }

bool SeTriangulatorManager::in_triangle ( SeVertex* v1, SeVertex* v2, SeVertex* v3, SeVertex* v )
 {
   double x1, y1, x2, y2, x3, y3, x, y;
   get_vertex_coordinates ( v1, x1, y1 );
   get_vertex_coordinates ( v2, x2, y2 );
   get_vertex_coordinates ( v3, x3, y3 );
   get_vertex_coordinates ( v, x, y );
   return sr_in_triangle ( x1, y1, x2, y2, x3, y3, x, y );
 }

bool SeTriangulatorManager::in_triangle ( SeVertex* v1, SeVertex* v2, SeVertex* v3,
                                          double x, double y )
 {
   double x1, y1, x2, y2, x3, y3;
   get_vertex_coordinates ( v1, x1, y1 );
   get_vertex_coordinates ( v2, x2, y2 );
   get_vertex_coordinates ( v3, x3, y3 );
   return sr_in_triangle ( x1, y1, x2, y2, x3, y3, x, y );
 }

bool SeTriangulatorManager::in_segment ( SeVertex* v1, SeVertex* v2, SeVertex* v, double eps )
 {
   double x1, y1, x2, y2, x, y;
   get_vertex_coordinates ( v1, x1, y1 );
   get_vertex_coordinates ( v2, x2, y2 );
   get_vertex_coordinates ( v, x, y );
   return sr_in_segment ( x1, y1, x2, y2, x, y, eps );
 }

bool SeTriangulatorManager::is_delaunay ( SeEdge* e )
 {
   SeBase* s1 = e->se();
   SeBase* s2 = s1->sym();
   SeBase* s3 = s2->nxt()->nxt();
   SeBase* s4 = s1->nxt()->nxt();

   double x1, y1, x2, y2, x3, y3, x4, y4;
   get_vertex_coordinates ( s1->vtx(), x1, y1 );
   get_vertex_coordinates ( s2->vtx(), x2, y2 );
   get_vertex_coordinates ( s3->vtx(), x3, y3 );
   get_vertex_coordinates ( s4->vtx(), x4, y4 );

   return sr_in_circle ( x1, y1, x2, y2, x4, y4, x3, y3 )? false:true;
 }

bool SeTriangulatorManager::is_flippable_and_not_delaunay ( SeEdge* e )
 {
   SeBase* s1 = e->se();
   SeBase* s2 = s1->sym();
   SeBase* s3 = s2->nxt()->nxt();
   SeBase* s4 = s1->nxt()->nxt();

   if ( s3->nxt()!=s2 ) return false; // not a triangle !

   double x1, y1, x2, y2, x3, y3, x4, y4;
   get_vertex_coordinates ( s1->vtx(), x1, y1 );
   get_vertex_coordinates ( s2->vtx(), x2, y2 );
   get_vertex_coordinates ( s3->vtx(), x3, y3 );
   get_vertex_coordinates ( s4->vtx(), x4, y4 );

   if ( sr_ccw(x3,y3,x2,y2,x4,y4)<=0 || sr_ccw(x4,y4,x1,y1,x3,y3)<=0 ) return false;

   return sr_in_circle ( x1, y1, x2, y2, x4, y4, x3, y3 );
 }

// attention: s must be well positioned
int SeTriangulatorManager::test_boundary ( SeVertex* v1, SeVertex* v2, SeVertex* v3,
                                           double x, double y, double eps, SeBase*& s )
 {
   double x1, y1, x2, y2, x3, y3;
   get_vertex_coordinates ( v1, x1, y1 );
   get_vertex_coordinates ( v2, x2, y2 );
   get_vertex_coordinates ( v3, x3, y3 );

   if ( sr_next(x1,y1,x,y,eps) ) { return SeTriangulator::VertexFound; }
   if ( sr_next(x2,y2,x,y,eps) ) { s=s->nxt(); return SeTriangulator::VertexFound; }
   if ( sr_next(x3,y3,x,y,eps) ) { s=s->nxt()->nxt(); return SeTriangulator::VertexFound; }
   if ( sr_in_segment(x1,y1,x2,y2,x,y,eps) ) { return SeTriangulator::EdgeFound; }
   if ( sr_in_segment(x2,y2,x3,y3,x,y,eps) ) { s=s->nxt(); return SeTriangulator::EdgeFound; }
   if ( sr_in_segment(x3,y3,x1,y1,x,y,eps) ) { s=s->nxt()->nxt(); return SeTriangulator::EdgeFound; }
   return SeTriangulator::NotFound;
 }

bool SeTriangulatorManager::segments_intersect ( SeVertex* v1, SeVertex* v2, 
                                                 SeVertex* v3, SeVertex* v4,
                                                 double& x, double& y )
 {
   double x1, y1, x2, y2, x3, y3, x4, y4;
   get_vertex_coordinates ( v1, x1, y1 );
   get_vertex_coordinates ( v2, x2, y2 );
   get_vertex_coordinates ( v3, x3, y3 );
   get_vertex_coordinates ( v4, x4, y4 );
   return sr_segments_intersect ( x1, y1, x2, y2, x3, y3, x4, y4, x, y );
 }

bool SeTriangulatorManager::segments_intersect ( SeVertex* v1, SeVertex* v2, 
                                                 SeVertex* v3, SeVertex* v4 )
 {
   double x1, y1, x2, y2, x3, y3, x4, y4;
   get_vertex_coordinates ( v1, x1, y1 );
   get_vertex_coordinates ( v2, x2, y2 );
   get_vertex_coordinates ( v3, x3, y3 );
   get_vertex_coordinates ( v4, x4, y4 );
   return sr_segments_intersect ( x1, y1, x2, y2, x3, y3, x4, y4 );
 }

bool SeTriangulatorManager::segments_intersect ( double x1, double y1, double x2, double y2,
                                                 SeVertex* v3, SeVertex* v4 )
 {
   double x3, y3, x4, y4;
   get_vertex_coordinates ( v3, x3, y3 );
   get_vertex_coordinates ( v4, x4, y4 );
   return sr_segments_intersect ( x1, y1, x2, y2, x3, y3, x4, y4 );
 }

void SeTriangulatorManager::segment_midpoint ( SeVertex* v1, SeVertex* v2, double& x, double& y )
 {
   double x1, y1, x2, y2;
   get_vertex_coordinates ( v1, x1, y1 );
   get_vertex_coordinates ( v2, x2, y2 );
   x = (x1+x2)/2.0;
   y = (y1+y2)/2.0;
 }

//================================================================================
//=============================== PathTree =======================================
//================================================================================

class SeTriangulator::PathNode // a node can be seen as one edge
 { public :
    int parent;   // index of the parent node, -1 if root
    SeBase* s; // the arrival edge of the node
    double ncost; // accumulated cost until touching s->edg()
    double hcost; // heuristic cost from s-edg()
   public :
    void init ( int pnode, SeBase* q, double nc, double hc )
         { parent=pnode; s=q; ncost=nc; hcost=hc; }
 };

class SeTriangulator::PathTree
 { public :
    SrArray<SeTriangulator::PathNode> nodes;
    SrHeap<int,double> leafs;
   public :
    PathTree ()
     { nodes.capacity(64); leafs.capacity(64); }

    void init () 
     { nodes.size(0); leafs.init(); }

    void add_child ( int pleaf, SeBase* s, double ncost, double hcost )
     { nodes.push().init ( pleaf, s, ncost, hcost );
       int ni = nodes.size()-1;
       leafs.insert ( ni, cost(ni) );
     }

    double cost ( int i )
     { return nodes[i].ncost + nodes[i].hcost; }

    int lowest_cost_leaf ()
     { 
       if ( leafs.empty() ) return -1;
       int i = leafs.top();
       leafs.remove();
       return i;
     }

    void print ()
     { int i;
       sr_out << "NParents: ";
       for ( i=0; i<nodes.size(); i++ ) sr_out<<nodes[i].parent<<srspc;
       sr_out << "\nLeafs: ";
       for ( i=0; i<leafs.size(); i++ ) sr_out<<leafs.elem(i)<<srspc;
       sr_out<<srnl;
     }
 };

//================================================================================
//============================== FunnelDeque =====================================
//================================================================================

class SrFunnelPt : public SrVec2
 { public :
    char apex;
    SrFunnelPt () { apex=0; }
    void set ( float a, float b ) { x=a; y=b; apex=0; }
 };

class SeTriangulator::FunnelDeque : public SrDeque<SrFunnelPt>
 { public :
 };

//================================================================================
//============================ SeTriangulator ====================================
//================================================================================

SeTriangulator::SeTriangulator ( Mode mode, SeMeshBase* m,
                                 SeTriangulatorManager* man, double epsilon )
 {
   _mode = mode;
   _mesh = m;
   _man = man;
   _epsilon = epsilon;
   _ptree = 0;
   _path_found = false;
   _fdeque = 0;
 }

SeTriangulator::~SeTriangulator ()
 {
   _man->unref();
   delete _ptree;
   delete _fdeque;
 }

//================================================================================
//============================= init triangulation ===============================
//================================================================================

SeBase* SeTriangulator::init_as_triangulated_square 
                               ( double x1, double y1, double x2, double y2,
                                 double x3, double y3, double x4, double y4 )
 {
   SeBase* s = _mesh->init();
   _man->set_vertex_coordinates ( s->vtx(), x3, y3 );
   _man->set_vertex_coordinates ( s->nxt()->vtx(), x4, y4 );
   s = _mesh->mev ( s );
   _man->set_vertex_coordinates ( s->vtx(), x2, y2 );
   s = _mesh->mev ( s );
   _man->set_vertex_coordinates ( s->vtx(), x1, y1 );
   _mesh->mef ( s, s->nxt()->nxt()->nxt() ); // close the square
   _mesh->mef ( s, s->nxt()->nxt() ); // triangulate square

   return s;
 }

//================================================================================
//============================= face triangulation ===============================
//================================================================================

bool SeTriangulator::triangulate_face ( SeFace* f, bool optimize )
 {
   SeBase *s, *x, *xn, *xp, *v, *vp, *vn;
   SrArray<SeBase*>& stack = _buffer;
   bool ok;

   x = s = f->se();

   SR_TRACE4 ( "starting triangulate..." );

   stack.size(0);
   if (optimize)
    { _mesh->begin_marking ();
      do { _mesh->mark(x->edg()); x=x->nxt(); } while ( x!=s );
    }

   SR_TRACE4 ( "entering loop..." );
  
   while ( !_mesh->is_triangle(x) )
    { ok = true;
      xn=x->nxt(); xp=x->pri(); v=xn->nxt();

      if ( _man->ccw(xp->vtx(),x->vtx(),xn->vtx()) )
       { do { if ( _man->in_triangle(xp->vtx(),x->vtx(),xn->vtx(),v->vtx()) ) // boundary is considered inside
               { ok=false; break; }
              v = v->nxt();
            } while ( v!=xp );
       }
      else ok=false;

      if (ok)
       { s = _mesh->mef ( xp, xn );
         _man->triangulate_face_created_edge ( s->edg() );
         if (optimize) stack.push()=s;
         s=xn;
       }
      x=xn;
      if ( !ok && x==s )
       { // Loop occured: this already happened because of dup points, and could
         // also occur because of self-intersecting or non CCW faces
         if (optimize) _mesh->end_marking ();
         sr_out.warning("Loop occured in SeTriangulator::triangulate_face!\n");
         // do { PRINTV(x->vtx()); PNL; x=x->nxt(); } while ( x!=s ); while(1);
         return false;
       }
    }

   SR_TRACE4 ( "optimizing..."<<stack.size() );

   while ( stack.size()>0 )
    { SR_TRACE4 ( "optimizing... "<<stack.size() );
      x = stack.pop();
      if ( _man->is_flippable_and_not_delaunay(x->edg()) ) // Flip will optimize triangulation
       { xn=x->nxt(); xp=xn->nxt(); vn=x->sym()->nxt(); vp=vn->nxt();
         if ( !_mesh->marked(vp->edg()) ) stack.push(vp);
         if ( !_mesh->marked(vn->edg()) ) stack.push(vn);
         if ( !_mesh->marked(xp->edg()) ) stack.push(xp);
         if ( !_mesh->marked(xn->edg()) ) stack.push(xn);
         _mesh->flip ( x );
       }
    }

   if (optimize) _mesh->end_marking ();

   SR_TRACE4 ( "ok!" );

   return true;
 }

//================================================================================
//============================= Locate Point=== ==================================
//================================================================================

// Still this one seems to be the most tricky algorithm, the problem is that any
// small perturbations given from the other methods will make location to crash.
// typical example is when a triangle becomes non-ccw. Have to re-checkk all other
// methods to avoid this to happen.
// Note: the "random walk" is generally worse than this mark/linear solution
SeTriangulator::LocateResult SeTriangulator::locate_point
                             ( const SeFace* iniface,
                               double x, double y,
                               SeBase*& result )
 {
   SeVertex *v1, *v2, *v3;
   SeBase *sn, *snn;
   SeBase *s = iniface->se();
   SrArray<SeBase*>& stack = _buffer;
   int sres = NotFound;
   int triangles = _mesh->faces();
   int visited_count=0;
   int ccws;
//   bool epsilon_mode = false;
   bool walk_failed = false;
   double x1, y1, x2, y2, x3, y3;

   SR_TRACE1 ( "locate_point..." );

   if ( !_mesh->is_triangle(s) ) 
    { sr_out.warning ("NON-TRIANGLE CASE FOUND IN SEARCH_TRIANGLE!\n");
      return NotFound;
    }

   _mesh->begin_marking ();
   _mesh->mark ( s->fac() );

   while ( true )
    { 
      sn=s->nxt(); snn=sn->nxt();
      v1=s->vtx(); v2=sn->vtx(); v3=snn->vtx(); 

/*      if ( epsilon_mode )
       { sres = _man->test_boundary ( v1, v2, v3, x, y, _epsilon, s );
         if ( sres!=NotFound ) 
          { SR_TRACE1 ("LOOP SOLVED IN EPSILON MODE!");
            break;
          }
       }*/

      stack.size(0);
      ccws=0;
      _man->get_vertex_coordinates ( v1, x1, y1 );
      _man->get_vertex_coordinates ( v2, x2, y2 );
      _man->get_vertex_coordinates ( v3, x3, y3 );

      if ( sr_ccw(x,y,x2,y2,x1,y1)>0 ) { ccws++; if(_mesh->is_triangle(s->sym())) stack.push()=s->sym(); }
      if ( sr_ccw(x,y,x3,y3,x2,y2)>0 ) { ccws++; if(_mesh->is_triangle(sn->sym())) stack.push()=sn->sym(); }
      if ( sr_ccw(x,y,x1,y1,x3,y3)>0 ) { ccws++; if(_mesh->is_triangle(snn->sym())) stack.push()=snn->sym(); }

      if ( ccws!=stack.size() )
       { sr_out.warning ("BORDER OR NON TRIANGULAR/CCW FACE ENCOUNTERED IN SEARCH_TRIANGLE!\n");
         PRINTV(v1);PRINTV(v2);PRINTV(v3); sr_out<<srnl;
         PRINTP(x,y); sr_out<<srnl<<srnl;
         walk_failed = true;
         sres = NotFound;
         break;
       } 

      if ( stack.empty() )
       { sres = ccws==0? TriangleFound:NotFound; break; } 
      else if ( stack.size()==1 )
       { s = stack[0];
       }
      else if ( stack.size()==2 ) // here we use marking, instead of a random choice
       { s = _mesh->marked(stack[0]->fac())?  stack[1]:stack[0];
       }
      else
       { SR_TRACE1 ("DEGENERATED CASE FOUND IN SEARCH_TRIANGLE!"); // already happen
         walk_failed = true;
         break;
       }

      if ( _mesh->marked(s->fac()) )
       { SR_TRACE1 ("SEARCH_TRIANGLE: Already Marked Found and test mode on!");
         //_mesh->end_marking ();
         //_mesh->begin_marking (); // increment the marking flag
         //epsilon_mode=true;
         walk_failed = true;
         break;
       }

/*      if ( epsilon_mode && _mesh->marked(s->fac()) ) 
       { SR_TRACE1 ("LOOP DETECTED IN EPSILON_MODE!"); // Not detected yet
         walk_failed = true;
         break;
       }*/
      if ( visited_count++>triangles )
       { SR_TRACE1 ("WALKED MORE THAN NUMBER OF TRIANGLES!\n"); 
         walk_failed = true;
         break;
       }

      _mesh->mark ( s->fac() );
    }

   SR_TRACE1 ( "Triangles Visited: " << visited_count );

   if ( walk_failed ) // do linear search...
    { SeFace *f, *fi;
      f = fi = s->fac();
      sr_out <<"PERFORMING LINEAR SEARCH... \n";
      do { s=f->se(); sn=s->nxt(); snn=sn->nxt();
           v1=s->vtx(); v2=sn->vtx(); v3=snn->vtx(); 
           if ( snn->nxt()==s ) // is triangle
            { if ( _man->in_triangle(v1,v2,v3,x,y) )
               { result=s;
                 sres = TriangleFound;
                 break;
               }
            }
           f = f->nxt();
         } while ( f!=fi );
      if ( sres==NotFound ) // try now with test boundary...
       { sr_out <<"PERFORMING 2ND LINEAR SEARCH... \n";
         f = fi;
         do { s=f->se(); sn=s->nxt(); snn=sn->nxt();
              v1=s->vtx(); v2=sn->vtx(); v3=snn->vtx(); 
              if ( snn->nxt()==s ) // is triangle
               { sres = _man->test_boundary ( v1, v2, v3, x, y, _epsilon, s );
                 if ( sres!=NotFound )
                  { result = s;
                    break;
                  }
               }
              f = f->nxt();
            } while ( f!=fi );
       }
      /*if ( sres==NotFound ) // now just get the closest vertex...
       { sr_out <<"PERFORMING 3RD LINEAR SEARCH... \n";
         v1 = v2 = v3 = f->se()->vtx();
         double dist2, min_dist2=-1;
         do { _man->get_vertex_coordinates ( v1, x1, y1 );
              dist2 = x*x1+y*y1;
              if ( dist2<min_dist2 || min_dist2<0 )
               { v3 = v1;
                 min_dist2=dist2;
               }
              v1 = v1->nxt();
            } while ( v1!=v2 );
         sres = VertexFound;
         s = v3->se();
       }*/
      if ( sres==NotFound ) sr_out.warning ("LINEAR SEARCH NOT FOUND!\n");
    }

   if ( sres==TriangleFound ) // perform extra tests
    { sres = _man->test_boundary ( v1, v2, v3, x, y, _epsilon, s );
      if ( sres==NotFound ) sres=TriangleFound;
    }

   SR_TRACE1 ( "Search Result : " << (sres==NotFound?"NotFound" : sres==VertexFound?"VertexFound" : sres==EdgeFound?"EdgeFound":"TriangleFound") );

   _mesh->end_marking ();
   result = s;
   return (LocateResult)sres;
 }

//================================================================================
//============================== Insert Vertex ===================================
//================================================================================

SeVertex* SeTriangulator::insert_point_in_face ( SeFace* f, double x, double y )
 {
   SR_TRACE2 ( "insert_point_in_face..." );

   SrArray<SeBase*>& stack = _buffer;
   SeBase *s, *t;

   // even if the point is in an edge the degenerated triangle will be
   // soon flipped by the circle test (this was the previous used solution)
   stack.size(3);
   s = f->se();
   t = _mesh->mev(s);
   SeVertex* v = t->vtx(); // this is the return vertex
   stack[0]=s;
   _man->set_vertex_coordinates ( v, x, y );

   s = s->nxt();
   t = _mesh->mef ( t, s ); //s->nxt()->nxt()->nxt() );
   stack[1]=s; 

   s = s->nxt();
   _mesh->mef ( t, s );
   stack[2]=s; 

   _propagate_delaunay ();

   return v;
 }

SeVertex* SeTriangulator::insert_point_in_edge ( SeEdge* e, double x, double y )
 {
   SR_TRACE2 ( "insert_point_in_edge..." );

   //return insert_point_in_face ( e, x, y );

   SeBase *s = e->se();

   /* We must project into the edge to ensure correctness, otherwise we may encounter
      cases where the polygon of the neighboors of a vertex v does not contain v */
   double x1, y1, x2, y2, qx, qy ;
   _man->get_vertex_coordinates ( s->vtx(), x1, y1 );
   _man->get_vertex_coordinates ( s->nxt()->vtx(), x2, y2 );
   if ( sr_segment_projection ( x1, y1, x2, y2, x, y, qx, qy, _epsilon ) )
    { x=qx; y=qy; 
    }
   else 
    { if ( sqrt(sr_dist2(x,y,x1,y1))<sqrt(sr_dist2(x,y,x2,y2)) )
       //{ x=x1; y=y1; } else { x=x2; y=y2; }
      return s->vtx(); else return s->nxt()->vtx();
    }

   SrArray<SeBase*>& stack = _buffer;
   SeBase *t;
   stack.size(4);

   t = _mesh->mev ( s, s->rot() );
   _man->set_vertex_coordinates ( t->vtx(), x, y );

   stack[0]=s->nxt();
   stack[1]=s->nxt()->nxt();
   stack[2]=t->nxt();
   stack[3]=t->nxt()->nxt();

   _mesh->mef ( s, stack[1] );
   _mesh->mef ( t, stack[3] );

   // in principle we do not need to fix this in the constrained case, where
   // intersections are required to be detected before insertion
   if ( _mode==ModeConstrained || _mode==ModeConforming )
    { if ( _man->is_constrained(e) )
       { SR_TRACE2 ( "Point in edge in constrained case..." );
         SrArray<int> ids;
         _man->get_constraints ( e, ids );
         _man->add_constraints ( t->edg(), ids ); // constrain the other subdivided part
         _man->vertex_found_in_constrained_edge ( s->vtx() );
       }
    }

   _propagate_delaunay ();

   return s->vtx();
 }

void SeTriangulator::_propagate_delaunay ()
 {
   SR_TRACE2 ( "propagate_delaunay..." );

   SeBase *s, *x, *t;

   if ( _mode==ModeConforming ) // ther might be recursion, so all arrays are local
    { SrArray<SeVertex*> constraints;
      SrArray<SrArray<int>*> constraints_ids; 
      SrArray<SeBase*>& stack = _buffer;

      constraints.capacity(32); 
      constraints_ids.capacity(32); 

      SR_TRACE2 ( "optimizing in conforming mode..."<<stack.size() );

      while ( stack.size()>0 )
       { x = stack.pop();
         if ( !_man->is_delaunay(x->edg()) )  // Flip will optimize triang
          { s=x->sym()->pri(); t=s->nxt()->nxt();
            if ( _man->is_constrained(x->edg()) )
             { SR_TRACE2 ( "Prepare to flip constrained edge..." );
               constraints_ids.push() = new SrArray<int>;
               _man->get_constraints ( x->edg(), *constraints_ids.top() );
               constraints.push() = x->vtx();
               constraints.push() = x->nxt()->vtx();
               _man->set_unconstrained ( x->edg() );
             }
            _mesh->flip(x); stack.push(s); stack.push(t);
          }
       }
   
      while ( constraints.size()>0 )
       { SR_TRACE2 ( "Fixing Constraints..." );
         _conform_line ( constraints.pop(), constraints.pop(), *constraints_ids.top() );
         delete constraints_ids.pop();
       }
    }
   else if ( _mode==ModeConstrained ) // (no recursion)
    { SrArray<SeBase*>& stack = _buffer;

      SR_TRACE2 ( "optimizing in constrained mode..."<<stack.size() );

      while ( stack.size()>0 )
       { x = stack.pop();
         if ( !_man->is_delaunay(x->edg()) )  // Flip will optimize triang
          { s=x->sym()->pri(); t=s->nxt()->nxt();
            if ( !_man->is_constrained(x->edg()) )
             { _mesh->flip(x); stack.push(s); stack.push(t); }
          }
       }
    }
   else // ModeUnconstrained (no recursion)
    { SrArray<SeBase*>& stack = _buffer;

      SR_TRACE2 ( "optimizing in unconstrained mode..."<<stack.size() );

      while ( stack.size()>0 )
       { x = stack.pop();
         // remember that in the delaunay case edges are always flippable;
         // originating from a star polygon
         if ( !_man->is_delaunay(x->edg()) )  // Flip will optimize triang
          { s=x->sym()->pri(); t=s->nxt()->nxt();
            _mesh->flip(x); stack.push(s); stack.push(t);
          }
       }
    }
   SR_TRACE2 ( "Ok." );
 }

SeVertex* SeTriangulator::insert_point ( double x, double y, const SeFace* inifac )
 { 
   SeBase* s;
   LocateResult res;

   res = locate_point ( inifac? inifac:_mesh->first()->fac(), x, y, s );

   if ( res==NotFound )
    { return 0;
    }
   else if ( res==SeTriangulator::VertexFound )
    { return s->vtx();
    }
   else if ( res==SeTriangulator::EdgeFound )
    { return insert_point_in_edge ( s->edg(), x, y );
    }
   else // a face was found
    { return insert_point_in_face ( s->fac(), x, y );
    }
 }

bool SeTriangulator::remove_vertex ( SeVertex* v )
 {
   SR_TRACE2 ( "Remove Vertex." );
   SeBase *s = _mesh->delv ( v->se() );
   return triangulate_face ( s->fac() );
 }

//================================================================================
//========================= Insert Line Constraint ===============================
//================================================================================

bool SeTriangulator::insert_line_constraint ( SeVertex *v1, SeVertex *v2, int id )
 {
   SrArray<int>& ids = _ibuffer;
   ids.size(1);
   ids[0]=id;
   if ( v1==v2 ) return true;
   switch ( _mode )
    { case ModeUnconstrained: return false;
      case ModeConforming: return _conform_line  ( v1, v2, ids );
      case ModeConstrained: return _constrain_line ( v1, v2, ids );
    }
   return false;
 }

bool SeTriangulator::insert_segment ( double x1, double y1, double x2, double y2, int id, const SeFace* inifac )
 { 
   SeVertex *v1, *v2;

   v1 = insert_point ( x1, y1, inifac );
   if ( !v1 ) return false;
   v2 = insert_point ( x2, y2, inifac );
   if ( !v2 ) return false;

   return insert_line_constraint ( v1, v2, id );
 }

// conform_line() is recursive and relies on many geometrical tests.
// It has been successfully tested with many examples.
// In one case I found that, when compiling with Visual C++, optimizations 
// should be set to disabled or default.
// After that I fixed a bug in kef(), that could be the cause.
// However there are still problems when several constraints intersect
// and are almost parallel
bool SeTriangulator::_conform_line ( SeVertex *v1, SeVertex *v2,
                                    const SrArray<int>& constraints_ids )
 {
   enum Case { LineExist, VertexInEdge, NeedToSubdivide };
   Case c;
   SeEdge* interedge;
   SeVertex *v=0;
   SeBase *x, *s;
   SrArray<SeVertex*> a(0,32);
   double px, py;

   SR_TRACE3 ( "Starting conform_line..." );

   a.push()=v1; a.push()=v2;

   while ( !a.empty() )
    { SR_TRACE3 ( "Stack size:"<<a.size());
      v2=a.pop();
      v1=a.pop();
      interedge=0; // edge intersecting v1,v2 and constrained
      c=NeedToSubdivide;
      x=s=v1->se();
      do { if ( x->nxt()->vtx()==v2 ) // (v1,v2) is there.
            { c=LineExist; break; }
           x=x->rot();
         } while ( x!=s );

      if ( c==NeedToSubdivide )
      do { if ( _man->in_segment(v1,v2,x->nxt()->vtx(),_epsilon) )
            { c=VertexInEdge; break; }
           x=x->rot();
         } while ( x!=s );

      if ( c==NeedToSubdivide )
      do { // will subdivide, so just check if we use an intersection point
           if ( _man->is_constrained(x->nxt()->edg()) &&
                _man->segments_intersect 
                    ( x->nxt()->vtx(), x->pri()->vtx(), v1, v2, px, py ) )
                 { interedge = x->nxt()->edg(); break; }
            
           x=x->rot();
         } while ( x!=s );

      //static int i=0; se_out<<i<<"\n"; if ( i++>300 ) while(1);

      if ( c==NeedToSubdivide )
       { SR_TRACE3 ( "NeedToSubdivide"<<(const char*)(interedge? "(with intersection)":"") );
         SeTriangulator::LocateResult res;
         if ( !interedge ) _man->segment_midpoint ( v1, v2, px, py );
         res = locate_point ( s->fac(), px, py, s );
         if ( res==SeTriangulator::NotFound ) 
          { return false; }
         else if ( res==SeTriangulator::VertexFound )
          { v=s->vtx(); } // I've already found one case here
         else if ( res==SeTriangulator::EdgeFound )
          { v = insert_point_in_edge ( s->edg(), px, py );
            if ( !v ) return false; 
            _man->new_steiner_vertex_created ( v );
          }
         else
          { v = insert_point_in_face ( s->fac(), px, py );
            if ( !v ) return false; 
            _man->new_steiner_vertex_created ( v ); // WAS v1
          }
         a.push()=v1; a.push()=v;
         a.push()=v; a.push()=v2;
       }
      else if ( c==VertexInEdge )
       { SR_TRACE3 ( "VertexInEdge");
         v1=x->nxt()->vtx();
         _man->vertex_found_in_constrained_edge ( v1 );
         a.push()=v1; a.push()=v2;
         _man->add_constraints ( x->edg(), constraints_ids );
       }
      else if ( c==LineExist )
       { SR_TRACE3 ( "LineExist");
         _man->add_constraints ( x->edg(), constraints_ids );
       }
    }

   return true;
 }

// handle ok if v1-v2 is already there
// To check: restrict_line doesnt seem to need to receive an array of indices.
bool SeTriangulator::_constrain_line ( SeVertex* v1, SeVertex* v2,
                                       const SrArray<int>& constraints_ids )
 {
   SrArray<SeBase*> totrig = _buffer; 
   SrArray<ConstrElem>& ea = _elem_buffer;
   SeBase *s, *e, *s1, *s2;
   SeVertex* v;
   int i, j, iv1, iv2;
   double px, py;

   SR_TRACE3 ( "Starting restrict_line..." );
   SR_TRACE3 ( "Looking for traversed elements..." );

   // First get lists of traversed elements (ea)
   v = v1;
   e = 0;
   s = v1->se();
   ea.size(0);
   ea.push().set(v,0);

   while ( v!=v2 )
    { if ( v ) _v_next_step ( s, v1, v2, v, e );
        else   _e_next_step ( s, v1, v2, v, e );

      //static int i=0; sr_out<<i<<"\n"; if ( i++>300 ) while(1);

      if ( e && _man->is_constrained(e->edg()) ) // intersects with another constrained edge
       { SrArray<int> ids;
         s = e;
         _man->segments_intersect ( v1, v2, s->vtx(), s->nxt()->vtx(), px, py );
         _man->get_constraints ( s->edg(), ids );
         s = _mesh->mev ( s, s->rot() );
         e=0; v=s->vtx();
         _man->set_vertex_coordinates ( v, px, py );
         _man->new_intersection_vertex_created ( v );
         _man->add_constraints ( s->edg(), ids );
         _mesh->mef ( s, s->nxt()->nxt() ); s=s->sym();
         _mesh->mef ( s->nxt(), s->pri() );
         SR_TRACE3 ( "Vertex added in constraint intersection" ); 
       }

      if ( v )
       { ea.push().set(v,0);
         s=v->se();
         SR_TRACE3 ( (v==v2?"Found vertex v2.":"Crossed existing vertex.") );
         if ( v!=v2 ) _man->vertex_found_in_constrained_edge(v);
         v1=v; // advance v1
       }
      else
       { ea.push().set(0,e);
         s=e->sym();
         SR_TRACE3 ( "Crossed edge." );
       }
    }

   //for ( i=0; i<ea.size(); i++ ) if(ea[i].v) PRINTV(ea[i].v); PNL;

   // Now kill crossed edges and constrain v1-v2 edge pieces
   totrig.size(0);
   iv1=0;
   for ( i=1; i<ea.size(); i++ ) // first element is always v1
    { if ( ea[i].v ) // 2nd vertex found (can be v2)
       { iv2=i;
         if ( iv2==iv1+1 )
          { SR_TRACE3 ( "Constraining existing edge..." );
            s = ea[iv1].v->se();
            while ( s->nxt()->vtx()!=ea[iv2].v ) s=s->rot();
            _man->add_constraints(s->edg(),constraints_ids);
          }
         else // kill inbetween edges and put new constraint there
          { //sr_out<<(iv1+1)<<" .. "<<(iv2-1)<<"\n";
            SR_TRACE3 ( "Killing crossed edges "<<(iv1+1)<<" to "<<(iv2-1)<<" ..." );
            s1 = ea[iv1+1].e->pri();
            for ( j=iv1+1; j<iv2; j++ ) s2=_mesh->kef(ea[j].e,&s);
            SR_TRACE3 ( "Adding edge constraint..." );
            s = _mesh->mef ( s1, s2->nxt() );
            _man->add_constraints(s->edg(),constraints_ids);
            totrig.push() = s;
            totrig.push() = s->sym();
          }
         iv1=iv2;
       }
    }

   // Finally retriangulate open regions:
   SR_TRACE3 ( "Retriangulating "<<totrig.size()<<" regions..." );
   while ( totrig.size()>0 )
    { s2 = totrig.pop();
      s1 = s2->nxt()->nxt();
      if ( s1->nxt()==s2 )
       { SR_TRACE3 ( "Region "<<totrig.size()<<" is already a triangle." ); continue; }
      for ( s=s1; s!=s2; s=s->nxt() )
       { if ( _can_connect(s2,s) )
          { if ( s==s1 )
             { SR_TRACE3 ( "MEF case 1" );
               totrig.push() = _mesh->mef ( s2, s );
             }
            else if ( s->nxt()==s2 )
             { SR_TRACE3 ( "MEF case 2" );
               totrig.push() = _mesh->mef ( s, s2->nxt() );
             }
            else
             { SR_TRACE3 ( "MEF case 3" );
               totrig.push() = _mesh->mef ( s, s2->nxt() );
               totrig.push() = _mesh->mef ( s2, s );
             }
            break;
          }
       } 
    }

   SR_TRACE3 ( "Done." );
   return true;
 }

void SeTriangulator::_v_next_step ( SeBase* s, SeVertex* v1, SeVertex* v2,
                                   SeVertex*& v, SeBase*& e )
 {
   e=0;
   SeBase* x = s;
   double x1, y1, x2, y2, x3, y3, x4, y4;

   // First look if final vertex is found:
   do { if ( x->nxt()->vtx()==v2 ) { v=v2; return; }
        if ( x->pri()->vtx()==v2 ) { v=v2; return; }
        x=x->rot();
      } while ( x!=s );

   _man->get_vertex_coordinates ( v1, x1, y1 );
   _man->get_vertex_coordinates ( v2, x2, y2 );

   // First test the first vertex:
   v = x->nxt()->vtx();
   _man->get_vertex_coordinates ( v, x3, y3 );

   if ( sr_in_segment(x1,y1,x2,y2,x3,y3,0) ) return;

   // Then look for intersections in all neighbours:
   do { v = x->nxt()->nxt()->vtx();
        _man->get_vertex_coordinates ( v, x4, y4 );

        if ( sr_in_segment(x1,y1,x2,y2,x4,y4,0) ) return;
        if ( sr_segments_intersect(x1,y1,x2,y2,x3,y3,x4,y4) ) { v=0; e=x->nxt(); return; }
     
        x3=x4; y3=y4;
        x=x->rot();
      } while ( x!=s );

   //======== this point should never be reached! ========================

    v=0; e=0; printf ( "Error in v_next_step() !\n" );

   printf("Constr: "); PRINTV(v1); PRINTV(v2); PNL;
   printf("Center: "); PRINTV(s->vtx()); PNL;
   printf("Star Vertex:\n");

   x = s;
   do { v = x->nxt()->vtx();
        PRINTV(v); PNL;
        x=x->rot();
      } while ( x!=s );

   v=0; e=0;
 }

void SeTriangulator::_e_next_step ( SeBase* s, SeVertex* v1, SeVertex* v2,
                                                     SeVertex*& v, SeBase*& e )
 {
   v=0;
   e=0;
   SeVertex *va, *vb;
   va = s->nxt()->vtx();
   vb = s->nxt()->nxt()->vtx();

   if ( vb==v2 ) { v=v2; return; }

   double x1, y1, x2, y2, x3, y3, x4, y4;
   _man->get_vertex_coordinates ( v1, x1, y1 );
   _man->get_vertex_coordinates ( v2, x2, y2 );
   _man->get_vertex_coordinates ( va, x3, y3 );
   _man->get_vertex_coordinates ( vb, x4, y4 );

   if ( sr_in_segment(x1,y1,x2,y2,x4,y4,0) ) { v=vb; }
   else if ( sr_segments_intersect(x1,y1,x2,y2,x3,y3,x4,y4) ) { e=s->nxt(); }
   else { e=s->nxt()->nxt(); }
 }

bool SeTriangulator::_can_connect ( SeBase* se, SeBase* sv )
 {
   double x1, y1, x2, y2, x3, y3, x, y;

   _man->get_vertex_coordinates ( se->vtx(), x1, y1 );
   _man->get_vertex_coordinates ( se->nxt()->vtx(), x2, y2 );
   _man->get_vertex_coordinates ( sv->vtx(), x3, y3 );

   SeBase* s;
   for ( s=se->nxt()->nxt(); s!=se; s=s->nxt() )
    { if ( s==sv ) continue;
      _man->get_vertex_coordinates ( s->vtx(), x, y );
      if ( sr_in_circle(x1,y1,x2,y2,x3,y3,x,y) ) return false;
    }
   
   return true;
 }

//================================================================================
//============================== search path =====================================
//================================================================================

bool SeTriangulator::_blocked ( SeBase* s )
 {
    SeBase *sym = s->sym();

    // test if the triangle is already visited:
    if ( _mesh->marked(sym->fac()) ) return true;

    // test if we're trying to cross an obstacle:
    if ( _man->is_constrained(s->edg()) ) return true;

    // security to avoid entering the border if a given point is outside the domain:
    if ( sym->nxt()->nxt()->nxt()!=sym ) return true;

    // ok, the edge is not blocked:
    return false;
 }

//We should use the Euclidian distance to have better results
# define PTDIST(a,b,c,d) sqrt(sr_dist2(a,b,c,d))

void SeTriangulator::_ptree_init ( SeTriangulator::LocateResult res, SeBase* s,
                                   double xi, double yi, double xg, double yg )
 {
   double c, h;
   double x1, y1, x2, y2, x3, y3, x, y;

   _man->get_vertex_coordinates ( s->vtx(), x1, y1 );
   _man->get_vertex_coordinates ( s->nxt()->vtx(), x2, y2 );
   _man->get_vertex_coordinates ( s->pri()->vtx(), x3, y3 );
   
   _ptree->init ();

   if ( res==EdgeFound && !_blocked(s) )
    { h = PTDIST(xi,yi,xg,yg);
      _ptree->add_child ( -1, s, 0, h );
      _ptree->add_child ( -1, s->sym(), 0, h );
      return;
    }

   _mesh->mark ( s->fac() );

   if ( !_blocked(s) )
    { x=(x1+x2)/2; y=(y1+y2)/2;
      c = PTDIST(xi,yi,x,y);
      h = PTDIST(x,y,xg,yg);
      _ptree->add_child ( -1, s, c, h );
    }
   
   s = s->nxt();
   if ( !_blocked(s) )
    { x=(x2+x3)/2; y=(y2+y3)/2;
      c = PTDIST(xi,yi,x,y);
      h = PTDIST(x,y,xg,yg);
      _ptree->add_child ( -1, s, c, h );
    }

   s = s->nxt();
   if ( !_blocked(s) )
    { x=(x3+x1)/2; y=(y3+y1)/2;
      c = PTDIST(xi,yi,x,y);
      h = PTDIST(x,y,xg,yg);
      _ptree->add_child ( -1, s, c, h );
    }
 }

# define ExpansionNotFinished  -1
# define ExpansionBlocked      -2

int SeTriangulator::_expand_lowest_cost_leaf ()
 {
   double x1, y1, x2, y2, x3, y3, x, y, a, b;
   double dcost, hcost, ncost;
   int min_i;
   
   min_i = _ptree->lowest_cost_leaf ();
   SR_TRACE5 ( "Expanding leaf: "<<min_i );

   if ( min_i<0 ) return ExpansionBlocked; // no more leafs: indicates that path could not be found!

   // Attention: n can be invalidated later on because of array reallocation
   PathNode& n = _ptree->nodes[min_i];//leaf(min_i);
   SeBase* s = n.s;
   ncost = n.ncost;

   _man->get_vertex_coordinates ( s->vtx(), x1, y1 );
   _man->get_vertex_coordinates ( s->nxt()->vtx(), x2, y2 );
   _man->get_vertex_coordinates ( s->sym()->pri()->vtx(), x3, y3 );

   // note that (p1,p2,p3) is not ccw, so we test (p2,p1,p3):
   if ( sr_in_triangle(x2,y2,x1,y1,x3,y3,_xg,_yg) ) return min_i;//leafs[min_i]; // FOUND!

   s = s->sym();
   _mesh->mark ( s->fac() );
   x=(x1+x2)/2; y=(y1+y2)/2;

   s = s->nxt();
   if ( !_blocked(s) )
    { a=(x1+x3)/2; b=(y1+y3)/2;
      dcost = PTDIST(x,y,a,b);
      hcost = PTDIST(a,b,_xg,_yg);
      _ptree->add_child ( min_i, s, ncost+dcost, hcost );   
    }

   s = s->nxt();
   if ( !_blocked(s) )
    { a=(x3+x2)/2; b=(y3+y2)/2;
      dcost = PTDIST(x,y,a,b);
      hcost = PTDIST(a,b,_xg,_yg);
      _ptree->add_child ( min_i/*_ptree->leafs[min_i]*/, s, ncost+dcost, hcost );   
    }

   return ExpansionNotFinished; // continue the expansion
 }

/* - Changing of triangle according to the edge semi-plane criteria (as for locate_point),
     has no correlation to the shortest path...
   - If the path is created from a list of faces, when linking the centroids, the
     trajectory is much worse, creating many zig-zags.
   - This is the A* algorithm that takes O(nf), f is the faces in the "expansion frontier".
     Without the "add distance to goal heuristic" in the cost function, this would
     be the Dijkstra algorithm. */
bool SeTriangulator::search_path ( double x1, double y1, double x2, double y2,
                                   const SeFace* iniface, bool vistest )
 {
   LocateResult res;
   SeBase *s;

   SR_TRACE5 ( "Starting Find Path..." );

   if ( !_ptree ) _ptree = new PathTree;
   _path_found = false;
   _channel.size(0);
   _xi=x1; _yi=y1; _xg=x2; _yg=y2;

   if ( !iniface ) return _path_found=false;

   res=locate_point ( iniface, x1, y1, s );
   if ( res==NotFound )
    { SR_TRACE5 ( "Could not locate first point!" );
      return _path_found=false;
    }

   // Even if p1 is coincident to a vertex or edge, locate_point should return in s 
   // the face which is most likely to contain p1
   if ( _man->in_triangle(s->vtx(),s->nxt()->vtx(),s->pri()->vtx(),x2,y2) )
    { SR_TRACE5 ( "Found both points in the same triangle! (0 length channel returned)" );
      return _path_found=true;
    }

   // The graph used in the A* search gives and aproximate shortest path.
   // If we dont want to miss shortest paths which are a direct line,
   // a specific test is performed for such cases.
   # define INTER(q) _man->segments_intersect(x1,y1,x2,y2,q->vtx(),q->nxt()->vtx())
   if ( vistest )
    { // Find first intersection with the triangle of p2:
      SeBase *se=s;
      int i=0;
      while ( !INTER(se) )
       { se=se->nxt();
         if ( i++==3 ) return _path_found=true; // none intersections (seg in triangle) (already tested anyway)
       }
      // Continues until end:
      SeBase *sen, *sep;
      while ( !_man->is_constrained(se->edg()) )
       { se=se->sym(); sen=se->nxt(); sep=se->pri();
         if ( INTER(sen) ) se=sen;
          else
         if ( INTER(sep) ) se=sep;
          else
         { SR_TRACE5 ( "Path is a direct line!" ); return _path_found=true; }
       }
    }
   # undef INTER

   SR_TRACE5 ( "Initializing A* search..." );
   _mesh->begin_marking ();
   _ptree_init ( res, s, x1, y1, x2, y2 ); // remember we start from the init point (x1,y1)

   SR_TRACE5 ( "Expanding leafs..." );
   int found = ExpansionNotFinished;
   while ( found==ExpansionNotFinished )
    found = _expand_lowest_cost_leaf();

   _mesh->end_marking ();

   if ( found==ExpansionBlocked )
    { SR_TRACE5 ( "Points are not connectable!" );
      return _path_found=false;
    }

   int n = found; // the starting leaf
   do { _channel.push() = _ptree->nodes[n].s;
        n = _ptree->nodes[n].parent; 
      } while ( n!=-1 );
   _channel.revert();

   SR_TRACE5 ( "Path crosses "<<_channel.size()<<" edges." );

   return _path_found=true;
 }

void SeTriangulator::get_channel_boundary ( SrPolygon& channel )
 {
   channel.size(0);
   if ( !_path_found ) return;

   int i;
   double x, y;
   SeVertex* v;
   SeVertex* lastv=0;

   channel.push().set ( (float)_xi, (float)_yi );

   for ( i=0; i<_channel.size(); i++ )
    { v = _channel[i]->vtx();
      if ( v==lastv ) continue;
      lastv = v;
      _man->get_vertex_coordinates ( v, x, y );
      channel.push().set ( (float)x, (float)y );
    }

   channel.push().set ( (float)_xg, (float)_yg );

   for ( i=_channel.size()-1; i>=0; i-- )
    { v = _channel[i]->nxt()->vtx();
      if ( v==lastv ) continue;
      lastv = v;
      _man->get_vertex_coordinates ( v, x, y );
      channel.push().set ( (float)x, (float)y );
    }
 }

void SeTriangulator::get_canonical_path ( SrPolygon& path )
 {
   path.open ( true );
   if ( !_path_found ) { path.size(0); return; }

   int i;
   double x1, y1, x2, y2;
   path.size(0);
   path.push().set ( (float)_xi, (float)_yi );
   for ( i=0; i<_channel.size(); i++ )
    { _man->get_vertex_coordinates ( _channel[i]->vtx(), x1, y1 );
      _man->get_vertex_coordinates ( _channel[i]->nxt()->vtx(), x2, y2 );
      path.push().set ( (float)(x1+x2)/2, (float)(y1+y2)/2 );
    }
   path.push().set ( (float)_xg, (float)_yg );
 }

//================================================================================
//=========================== funnel shortest path ===============================
//================================================================================


static bool ordccw ( bool normal_order, SrPnt2 p1, SrPnt2 p2, float x, float y )
 {
   if ( normal_order )
    return SR_CCW(p1.x,p1.y,p2.x,p2.y,x,y)>=0? true:false;
   else
    return SR_CCW(p2.x,p2.y,p1.x,p1.y,x,y)>=0? true:false;
 }

void SeTriangulator::_funnel_add ( bool intop, SrPolygon& path, const SrPnt2& p )
 {
   FunnelDeque& dq = *_fdeque;

   dq.top_mode ( intop );

   bool ccw;
   bool order = intop;
   bool newapex = false;
   if ( dq.size()<=2 ) { dq.push().set(p.x,p.y); return; }

   while(1)
    { SrFunnelPt& p1 = dq.get();
      SrFunnelPt& p2 = dq.get(1);

      // if the apex is passed, the orientation test changes
      if ( p1.apex ) { order=!order; newapex=true; }
      ccw = ordccw ( order, p1, p2, p.x, p.y );
      if ( !ccw ) break;

      dq.pop();
      if ( newapex ) path.push().set ( p2.x, p2.y );

      if ( dq.size()==1 ) break;
    }

   if ( dq.size()==1 ) 
    { SrFunnelPt g=dq.get(); // cannot get a reference as the deque will change
      dq.get().apex=true;
      dq.push().set(p.x,p.y);
      dq.top_mode(!intop);
      dq.push().set(g.x,g.y);
    }
   else
    { if ( newapex ) dq.get().apex=true;
      dq.push().set(p.x,p.y);
    }

   SR_TRACE7 ( " psize:"<<path.size() <<" dqsize:"<<dq.size() << ((char*)intop? ", top ":", bot ") << (int)p.x << "," << (int) p.y );
 }

void SeTriangulator::get_shortest_path ( SrPolygon& path )
 {
   SR_TRACE7 ( "Entering shortest path..." );
   path.open ( true );
   path.size ( 0 );
   if ( !_path_found ) { path.size(0); return; }

   path.push().set((float)_xi,(float)_yi);

   if ( _channel.empty() )
    { path.push().set((float)_xg,(float)_yg);
      return;
    }

   int i;
   double x, y;
   if ( !_fdeque ) _fdeque = new FunnelDeque;
   FunnelDeque& dq = *_fdeque;
   dq.init();
   
   // add the first apex:
   SR_TRACE7 ( "Addind apex..." );
   dq.pusht().set((float)_xi,(float)_yi); dq.top().apex=1;

   // pass the funnel:
   SeBase* s = _channel[0];
   SrPnt2 p1, p2;
   _man->get_vertex_coordinates ( s->vtx(), x, y ); p1.set((float)x,(float)y);
   _man->get_vertex_coordinates ( s->nxt()->vtx(), x, y ); p2.set((float)x,(float)y);
   _funnel_add ( 0, path, p1 );
   _funnel_add ( 1, path, p2 );

   int max = _channel.size()-1;
   SeBase *s1, *s2;
   for ( i=0; i<max; i++ )
    { SR_TRACE7 ( "Processing channel edge "<<i );
      s1 = _channel[i];
      s2 = _channel[i+1];

      _man->get_vertex_coordinates ( s2->vtx(), x, y ); p1.set((float)x,(float)y);
      _man->get_vertex_coordinates ( s2->nxt()->vtx(), x, y ); p2.set((float)x,(float)y);

      if ( s1->vtx()==s2->vtx() ) // upper vertex rotates
       _funnel_add ( 1, path, p2 );
	  //left
      else
       _funnel_add ( 0, path, p1 );
	  //right
   }

   // To finalize:
   // 1. check where is the zone in the funnel that the goal is:
   bool order=true;
   dq.top_mode ( order );
   while ( dq.size()>1 )
    { if ( dq.get().apex ) { order=!order; dq.top_mode(order); }
      if ( ordccw(order,dq.get(),dq.get(1),(float)_xg,(float)_yg) ) dq.pop();
       else break;
    }

   // 2. add the needed portion of the funnel to the path:
   for ( i=0; dq.get(i).apex==0; i++ );
   while ( --i>=0 ) path.push() = dq.get(i);

   // 3. end path:
   path.push().set((float)_xg,(float)_yg);

   SR_TRACE7 ( "End..." );
 }

//============================ End of File =================================


