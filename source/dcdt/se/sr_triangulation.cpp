#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_random.h"
# include "sr_geo2.h"
# include "sr_triangulation.h"

//# define SR_USE_TRACE1 // search triangle
//# define SR_USE_TRACE2 // recover delaunay
# include "sr_trace.h"

# define PRINT(t) { sr_out<<t->vid(); sr_out<<" "<<t->nxt()->vid()<<srnl; t->pri(); }
# define DELPREC 1.0E-10
# define LINPREC 1.0E-5

//============================== SrTri =======================================

void SrTri::set ( SrVtx* v0, SrVtx* v1, SrVtx* v2, SrTri* t0, SrTri* t1, SrTri* t2, int id )
 {
   _vtx[0] = v0;
   _vtx[1] = v1;
   _vtx[2] = v2;
   _side[0] = t0;
   _side[1] = t1;
   _side[2] = t2;
   _mark[0] = _mark[1] = _mark[2] = 0;
   _id = id;
 }

// returns the j index to achieve _side[i]->_side[j]==this
int SrTri::sideindex ( int i )
 {
   SrTri* s = _side[i];
   SrVtx* v = _vtx[i];
   return s->_vtx[1]==v? 0 :
          s->_vtx[2]==v? 1 : 2;
 }

//=========================== SrTravel ==============================

SrTravel SrTravel::sym () const
 {
   SrVtx* ov = _t->_vtx[_v];  // original vertex
   SrTri* at = _t->_side[_v]; // adjacent triangle
   return SrTravel ( at, at->_vtx[0]==ov? 2 : 
                         at->_vtx[1]==ov? 0 : 1 );
 }

bool SrTravel::is_delaunay () const
 {
   SrVtx* p = vtx(2); // opposite vtx
   SrTravel s = sym();
   SrVtx* a = s.vtx();
   SrVtx* b = s.vtx(1);
   SrVtx* c = s.vtx(2);

   //sr_out<<"Del test: "<<a->id()<<srspc<<b->id()<<srspc<<c->id()<<":"<<p->id()<<srnl;

   return !sr_in_circle ( a->x(), a->y(), b->x(), b->y(), c->x(), c->y(), p->x(), p->y() );
 }

bool SrTravel::is_flippable () const
 {
   double p1x, p2x, p3x, p4x, p1y, p2y, p3y, p4y;

   p1x=x();   p1y=y();   SrTravel s=sym();
   p2x=s.x(); p2y=s.y(); s=s.pri();
   p3x=s.x(); p3y=s.y(); s=pri();
   p4x=s.x(); p4y=s.y();

   return SR_CCW(p3x,p3y,p2x,p2y,p4x,p4y)>0 && 
          SR_CCW(p4x,p4y,p1x,p1y,p3x,p3y)>0 ? true:false;
 }

//============================== SrTriangulation =======================================

SrTriangulation::SrTriangulation ()
 {
 }

SrTriangulation::~SrTriangulation ()
 {
   while ( _tri.size() ) delete _tri.pop();
   while ( _vtx.size() ) delete _vtx.pop();
 }

const SrArray<SrTravel>& SrTriangulation::edges()
 {
   int i;
   SrTravel t;
   _stack.size(0);
   for ( i=2; i<_tri.size(); i++ ) // skip the first two backfaces
    { t.set ( _tri[i], 0 );
      if ( !t.marked() ) { t.mark(); t.sym().mark(); _stack.push()=t; }
      t = t.nxt();
      if ( !t.marked() ) { t.mark(); t.sym().mark(); _stack.push()=t; }
      t = t.nxt();
      if ( !t.marked() ) { t.mark(); t.sym().mark(); _stack.push()=t; }
    }
   reset_markers();
   return _stack;
 }

void SrTriangulation::init ( double ax, double ay, double bx, double by )
 {
   while ( _tri.size()>4 ) delete _tri.pop();
   while ( _vtx.size()>4 ) delete _vtx.pop();
   while ( _tri.size()<4 ) _tri.push() = new SrTri;
   while ( _vtx.size()<4 ) _vtx.push() = new SrVtx;

   # define TRISET(i,v0,v1,v2,t0,t1,t2) _tri[i]->set(_vtx[v0],_vtx[v1],_vtx[v2],_tri[t0],_tri[t1],_tri[t2],i)

   _vtx[0]->set ( ax, ay, 0 );
   _vtx[1]->set ( bx, ay, 1 );
   _vtx[2]->set ( bx, by, 2 );
   _vtx[3]->set ( ax, by, 3 );

   TRISET ( 0,  0, 2, 1,  1, 2, 2 );
   TRISET ( 1,  0, 3, 2,  3, 3, 0 );
   TRISET ( 2,  0, 1, 2,  0, 0, 3 );
   TRISET ( 3,  0, 2, 3,  2, 1, 1 );

   # undef TRISET
 }

SrTravel SrTriangulation::border ()
 {
   return SrTravel ( _tri[0], 2 ); // travel vertices: ( 1, 0 )
 }

void SrTriangulation::reset_markers ()
 {
   int i;
   for ( i=0; i<_tri.size(); i++ )
    { _tri[i]->_mark[0] = 0;
      _tri[i]->_mark[1] = 0;
      _tri[i]->_mark[2] = 0;
    }
 }

void SrTriangulation::addv ( SrTravel& tri, double x, double y )
 {
   SrVtx* v = _vtx.push() = new SrVtx;
   v->set ( x, y, _vtx.size()-1 );

   SrTri* n0 = tri._t;
   SrTri* n1 = _tri.push() = new SrTri; 
   SrTri* n2 = _tri.push() = new SrTri; 
   
   SrVtx* v0 = n0->_vtx[0];
   SrVtx* v1 = n0->_vtx[1];
   SrVtx* v2 = n0->_vtx[2];
   
   SrTri* s0 = n0->_side[0];
   SrTri* s1 = n0->_side[1];
   SrTri* s2 = n0->_side[2];

   int i0 = n0->sideindex ( 0 );
   int i1 = n0->sideindex ( 1 );
   int i2 = n0->sideindex ( 2 );

   n0->set ( v0, v1, v, s0, n1, n2, n0->_id );
   n1->set ( v1, v2, v, s1, n2, n0, _tri.size()-2 );
   n2->set ( v2, v0, v, s2, n0, n1, _tri.size()-1 );

   s0->_side[i0] = n0;
   s1->_side[i1] = n1;
   s2->_side[i2] = n2;
   
   tri._v = 2;
 }

void SrTriangulation::flip ( SrTravel& edg )
 {
   SrVtx *v0, *v1, *v2, *v3;
   SrTri *s0, *s1, *s2, *s3, *n0, *n1;

   SrTravel d = edg;
   SrTravel d0 = d.nxt();
   SrTravel d2 = d.pri();
   SrTravel d1 = d.sym().pri();
   SrTravel d3 = d1.pri();

   n0=d0.tri(); n1=d1.tri();

   v0=d0.vtx(); v1=d1.vtx(); v2=d2.vtx(); v3=d3.vtx();

   s0=d0.adt(); s1=d1.adt(); s2=d2.adt(); s3=d3.adt();

   int i0 = n0->sideindex ( d0._v );
   int i1 = n1->sideindex ( d1._v );
   int i2 = n0->sideindex ( d2._v );
   int i3 = n1->sideindex ( d3._v );

   n0->set ( v0, v2, v1, s0, n1, s1, n0->_id );
   n1->set ( v3, v1, v2, s3, n0, s2, n1->_id );
   s0->_side[i0] = s1->_side[i1] = n0;
   s2->_side[i2] = s3->_side[i3] = n1;
   
   edg._v = 1;
 }

/*
static void ptri ( const Travel* t )
 {
   static float p1x, p2x, p3x, p4x, p1y, p2y, p3y, p4y;
   Travel* s=t->dup();

   sr_out.fmt_float ( "%+5.3f" );
   p1x=s->x(); p1y=s->y(); s->nxt();
   p2x=s->x(); p2y=s->y(); s->nxt();
   p3x=s->x(); p3y=s->y(); s->nxt();
   sr_out<<p1x<<","<<p1y<<srspc;
   sr_out<<p2x<<","<<p2y<<srspc;
   sr_out<<p3x<<","<<p3y<<srspc;
   sr_out<<"Flippable: "<<(int)is_flippable(t)<<srspc;
   sr_out<<"Delaunay: "<<(int)is_delaunay(t)<<srspc;
   if ( s->x()!=p1x || s->y()!=p1y ) sr_out<<"(Not a Triangle) ";
   sr_out<<srnl;
   sr_out.default_formats();
   delete s;
 }

static void prot ( const Travel* t )
 {
   static float x, y;
   Travel* s=t->dup();

   sr_out.fmt_float ( "%+5.3f" );
   x=s->x(); y=s->y(); 
   sr_out<<x<<","<<y<<": ";
   do { s->nxt();
        x=s->x(); y=s->y(); 
        s->pri();
        sr_out<<x<<","<<y<<srspc;
        s->rot();
      } while ( !s->equal(t) );

   sr_out<<srnl;
   sr_out.default_formats();
   delete s;
 }
*/

void SrTriangulation::recover_delaunay ( SrTravel& vtx )
 {
   SR_TRACE2 ( "recover_delaunay..." );
   _stack.size(0);

   SrVtx* srvtx = vtx.vtx();

   SrTravel s=vtx;
   do { _stack.push() = s.nxt().sym();
        SR_TRACE2 ( "pushed: "<<s.nxt().sym().vid()<<":"<<s.nxt().sym().vnid() );
        s = s.rot();
      } while ( s!=vtx );

   while ( _stack.size() )
    { SR_TRACE2 ( "while" );
      s = _stack.pop();
      if ( s.tri()->back() ) continue;
      SR_TRACE2 ( "pop: ("<<s.vid()<<","<<s.vnid()<<")" );

      if ( s.is_flippable() )     // Can flip
       { SR_TRACE2 ( "flippable" );
         if ( !s.is_delaunay() )  // Flip will optimize triang
          { SR_TRACE2 ( "flip" );
            _stack.push() = s.pri().sym();
            _stack.push() = s.nxt().sym();
            flip ( s );
          }
       }
    }

   while ( vtx.vtx()!=srvtx ) vtx=vtx.nxt();
   SR_TRACE2 ( "Ok." );
 }

SrTriangulation::LocateResult SrTriangulation::locate_point ( double x, double y, SrTravel& s, double prec )
 {
   double p1x, p1y, p2x, p2y, p3x, p3y;

   SR_TRACE1 ( "locate_point for "<<x<<srspc<<y<<"..." );
   _stack.size(0);
   s = border().sym();

   while ( true )
    { 
      if ( s.back() ) { SR_TRACE1("Not Found."); return NotFound; }
      
      # ifdef SR_USE_TRACE1
      sr_out<<"cur tri: "<<s.vid()<<srspc; s=s.nxt(); 
      sr_out<<s.vid()<<srspc; s=s.nxt();
      sr_out<<s.vid()<<srnl; s=s.nxt();
      # endif

      p1x=s.x(); p1y=s.y();
      p2x=s.vtx(1)->x(); p2y=s.vtx(1)->y();      
      p3x=s.vtx(2)->x(); p3y=s.vtx(2)->y();

      if ( sr_next(x,y,p1x,p1y,prec) ) { SR_TRACE1("Found Vertex."); return VertexFound; }
      if ( sr_next(x,y,p2x,p2y,prec) ) { s=s.nxt(); SR_TRACE1("Found Vertex."); return VertexFound; }
      if ( sr_next(x,y,p3x,p3y,prec) ) { s=s.pri(); SR_TRACE1("Found Vertex."); return VertexFound; }

      _stack.size(0);
      if ( SR_CCW(x,y,p1x,p1y,p2x,p2y)<0 ) _stack.push()=s.sym();
      if ( SR_CCW(x,y,p2x,p2y,p3x,p3y)<0 ) _stack.push()=s.nxt().sym();
      if ( SR_CCW(x,y,p3x,p3y,p1x,p1y)<0 ) _stack.push()=s.pri().sym();

      if ( _stack.empty() )
       { SR_TRACE1("Found Triangle."); return TriangleFound; }
      else if ( _stack.size()==1 )
       { s = _stack[0]; }
      else // delaunay searches, in principle, do not need this extra check...
       { s = _stack [ SrRandom::randf()<0.5f? 0:1 ]; }
    }
 }

bool SrTriangulation::insert_point ( double x, double y, SrTravel& t, double prec )
 {
   LocateResult res = locate_point(x,y,t,prec);

   if ( res==TriangleFound )
    { addv ( t, x, y );
      recover_delaunay ( t );
      return true;
    }
    
   return res==NotFound? false:true;
 }

void SrTriangulation::output ( SrOutput& out )
 {
   int i;
   for ( i=0; i<_vtx.size(); i++ )
    { out<<i<<": "<<_vtx[i]->x()<<srspc<<_vtx[i]->y()<<srnl;
    }
    
   for ( i=0; i<_tri.size(); i++ )
    { out<<i<<": ";
      out<<_tri[i]->_vtx[0]->id()<<srspc;
      out<<_tri[i]->_vtx[1]->id()<<srspc;
      out<<_tri[i]->_vtx[2]->id()<<" : ";
      out<<_tri[i]->_side[0]->id()<<srspc;
      out<<_tri[i]->_side[1]->id()<<srspc;
      out<<_tri[i]->_side[2]->id()<<srnl;
    }
 }

//============================= EOF ===================================

/*

int Mesh::test_delaunay ()
 {
   SrArray<Travel*> stack(0,0,32);
   stack.push() = get_border_travel()->dup();
   Travel* t = stack.top()->dup();
   begin_marking();
   int n=0;

   while ( stack.size()>0 )
    { Travel* ti = stack.pop();
      t->set ( ti );

      do { if ( !is_delaunay(t) ) n++;
           t->mark();
           t->sym();
           if ( !t->marked() ) stack.push() = t->dup();
           t->sym();
           t->nxt();
         } while ( !t->equal(ti) );

      delete ti;
    }

   delete t;
   end_marking();
   return n;
 }
*/