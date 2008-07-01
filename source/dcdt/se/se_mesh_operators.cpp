#include "precompiled.h"
#include "0ad_warning_disable.h"

# include <stdlib.h>
# include "se_mesh.h"

//# define CHECKALL_OPS   // Will call check_all() after each operator
//# define SR_USE_TRACE1  // Operators trace
# include "sr_trace.h"

//===================================== Some Macros =======================================

# ifdef CHECKALL_OPS
# include <stdlib.h>
# define CHECKALL { int err=check_all(); if(err!=0) { printf("CHECK ERROR: %d\n",err ); exit(1); } }
# else
# define CHECKALL
# endif

# define SE_MAX3(a,b,c) (a>b? (a>c?(a):(c)):(b>c?(b):(c)))

# define EDG_CREATE(x,y) x=new SeBase; y=new SeBase; \
                         x->_next=y; x->_rotate=x;   \
                         y->_next=x; y->_rotate=y;   

# define DEF_SPLICE_VARS SeBase *sp_xnxt, *sp_ynxt, *sp_xsym

// the effect is: swap(x.nxt,y.nxt); swap(x.nxt.rot,y.nxt.rot)
# define SPLICE(x,y)     sp_xnxt = x->_next; \
                         sp_ynxt = y->_next; \
                         sp_xsym = sp_xnxt->_rotate; \
                         x->_next = sp_ynxt; \
                         y->_next = sp_xnxt; \
                         sp_xnxt->_rotate = sp_ynxt->_rotate; \
                         sp_ynxt->_rotate = sp_xsym

//======================================== private members ========================================

void SeMeshBase::_new_vtx ( SeBase *s )
 {
   _vertices++;
   SeVertex* v = (SeVertex*)_vtxman->alloc();
   v->_symedge = s;
   if ( _first ) _first->vtx()->_insert(v);
   s->_vertex = v;
 }

void SeMeshBase::_new_edg ( SeBase *s )
 {
   _edges++;
   SeEdge* e = (SeEdge*)_edgman->alloc();
   e->_symedge = s;
   if ( _first ) _first->edg()->_insert(e);
   s->_edge = e;
 }

void SeMeshBase::_new_fac ( SeBase *s )
 {
   _faces++;
   SeFace* f = (SeFace*)_facman->alloc();
   f->_symedge = s;
   if ( _first ) _first->fac()->_insert(f);
   s->_face = f;
 }

void SeMeshBase::_del_vtx ( SeElement *e )
 {
   _vertices--;
   if ( e==_first->vtx() ) _first = e->nxt()==e? 0:e->nxt()->se();
   e->_remove();
   _vtxman->free ( e );
 }

void SeMeshBase::_del_edg ( SeElement *e )
 {
   _edges--;
   if ( e==_first->edg() ) _first = e->nxt()==e? 0:e->nxt()->se();
   e->_remove();
   _edgman->free ( e );
 }

void SeMeshBase::_del_fac ( SeElement *e )
 {
   _faces--;
   if ( e==_first->fac() ) _first = e->nxt()==e? 0:e->nxt()->se();
   e->_remove();
   _facman->free ( e );
 }

//==================================== operator related funcs ======================================

const char* SeMeshBase::translate_op_msg ( OpMsg m )
 {
   switch (m)
    { case OpNoErrors            : return "No Errors occured";
      case OpMevParmsEqual       : return "Parms in mev are equal";
      case OpMevNotSameVtx       : return "Parms in mev are not in the same vertex";
      case OpMefNotSameFace      : return "Parms in mef are not in the same face";
      case OpKevLastEdge         : return "Cannot delete the last edge with kev";
      case OpKevManyEdges        : return "More then one edge in parm vertex in kev";
      case OpKevOneEdge          : return "Vertex of parm has only one edge in kev";
      case OpKefSameFace         : return "Parm in kef must be between two different faces";
      case OpMgUncompatibleFaces : return "Parms in mg must be adjacent to faces with the same number of vertices";
      case OpMgSameFace          : return "Parms in mg must be, each one, adjacent to a different face";
      case OpFlipOneFace         : return "Parm in flip must be between two different faces";
      case OpEdgColOneFace       : return "Parm in edgcol must be between two different faces";
      case OpVtxSplitParmsEqual  : return "Parms in vtxsplit are equal";
      case OpVtxSplitNotSameVtx  : return "Parms in vtxsplit not in the same vertex";
    }
   return "Undefined error code";
 }


static SeMeshBase::ChkMsg check ( SeBase *x, int max )
 {
   int i=0;
   SeBase *xi = x;
   x = xi->nxt();
   while ( x!=xi ) { x=x->nxt(); i++; if(i>max) return SeMeshBase::ChkNxtError; }
   i=0;
   x = xi->rot();
   while ( x!=xi ) { x=x->rot(); i++; if(i>max) return SeMeshBase::ChkRotError; }
   return SeMeshBase::ChkOk;
 }

SeMeshBase::ChkMsg SeMeshBase::check_all () const
 {
   ChkMsg m;
   int i, max = 2*SE_MAX3(_vertices,_edges,_faces); // A very high roof to avoid infinite loop
   SeElement *e, *ei;

   i = 0;
   e = ei = _first->vtx();
   do { if ( e->se()->vtx()!=e || i++>max ) return ChkVtxError;
        e = e->nxt();
      } while ( e!=ei );

   i = 0;
   e = ei = _first->edg();
   do { if ( e->se()->edg()!=e || i++>max ) return ChkEdgError;
        e = e->nxt();
      } while ( e!=ei );

   i = 0;
   e = ei = _first->fac();
   do { if ( e->se()->fac()!=e || i++>max ) return ChkFacError;
        e = e->nxt();
      } while ( e!=ei );

   e = ei = _first->edg();
   do { m = check ( e->se(), max );
        if ( m!=ChkOk ) return m;
        m = check ( e->se()->sym(), max );
        if ( m!=ChkOk ) return m;
        e = e->nxt();
      } while ( e!=ei );
   return ChkOk;
 }     

//======================================== SeMeshBase Operators ========================================

void SeMeshBase::destroy ()
 {
   if (!_first) return;
   SR_TRACE1("destroy...");

   int i;
   SeElement *ini, *cur, *curn;

   // Save all symedges in a list:
   SrArray<SeBase*> S(0,_edges*2);
   cur = ini = _first->edg();
   do { S.push() = cur->se();
        S.push() = cur->se()->sym();
        cur = cur->nxt();
      } while ( cur!=ini );
  
   // delete element lists :
   SrClassManagerBase* man;
   for ( i=0; i<3; i++ ) // for each element type
    { if ( i==0 ) 
       { ini=_first->vtx(); man=_vtxman; }
      else if ( i==1 ) 
       { ini=_first->edg(); man=_edgman; }
      else  
       { ini=_first->fac(); man=_facman; }

      cur=ini;
      do { curn = cur->nxt(); // for each element
           man->free ( cur );
           cur = curn;
	     } while ( cur!=ini );
    }

   // delete all symedges:
   for ( i=0; i<S.size(); i++ ) delete S[i];

   _defaults ();

   SR_TRACE1("Ok.");
 }

SeBase *SeMeshBase::init ()
 {
   SR_TRACE1("init...");
   destroy ();
   SeBase *ne1, *ne2;
   EDG_CREATE ( ne1, ne2 );

   _new_vtx(ne1);
   _new_vtx(ne2); ne2->vtx()->_insert(ne1->vtx());
   _new_edg(ne1); ne2->_edge=ne1->edg();
   _new_fac(ne1); ne2->_face=ne1->fac();

   _first = ne2; // _first must be set only here!

   CHECKALL;
   SR_TRACE1("Ok.");
   return ne2;
 }

SeBase *SeMeshBase::mev ( SeBase *s )
 {
   SR_TRACE1("mev...");

   SeBase *ne1, *ne2;
   EDG_CREATE ( ne1, ne2 );

   ne1->_vertex = s->vtx();
   _new_vtx(ne2); ne1->_vertex=s->vtx();
   _new_edg(ne2); ne1->_edge=ne2->edg();
   ne1->_face = ne2->_face = s->fac();

   DEF_SPLICE_VARS;
   SPLICE ( s->pri(), ne2 );

   _op_last_msg = OpNoErrors;

   CHECKALL;
   SR_TRACE1("Ok.");

   return ne2;
 }

SeBase *SeMeshBase::mev ( SeBase *s1, SeBase *s2 )
 {
   SR_TRACE1("mev2...");

   if ( _op_check_parms )
    { if ( s1==s2 ) return _op_error(OpMevParmsEqual);
      if ( s1->vtx()!=s2->vtx() ) return _op_error(OpMevNotSameVtx);
      _op_last_msg = OpNoErrors;
    }

   SeBase *ne1, *ne2;
   EDG_CREATE ( ne1, ne2 );

   _new_vtx(ne1); ne2->_vertex=s2->vtx();
   _new_edg(ne1); ne2->_edge = ne1->edg();
   ne1->_face = s1->sym()->fac();
   ne2->_face = s2->sym()->fac();
   s2->_vertex->_symedge = s2;

   DEF_SPLICE_VARS;
   SPLICE ( s1->sym(), ne2 );
   SPLICE ( s2->sym(), ne1 );
   SPLICE ( ne1, ne2 );

   s2=s1;
   while ( s1!=ne1 )
    { s1->_vertex = ne1->vtx();
      s1=s1->rot();
      if(s1==s2) { printf("MEV2 INFINITE LOOP FOUND!\n"); break; }
                   // Prevents infinite loop by incoherent args
    }

   CHECKALL;

   SR_TRACE1("Ok.");

   return ne1;
 }

SeBase *SeMeshBase::mef ( SeBase *s1, SeBase *s2 )
 {
   SR_TRACE1("mef...");

   if ( _op_check_parms )
    { if ( s1->fac()!=s2->fac() ) return _op_error (OpMefNotSameFace);
      _op_last_msg = OpNoErrors;
    }

   SeBase *ne1, *ne2;
   EDG_CREATE ( ne1, ne2 );

   ne1->_vertex = s2->vtx();
   ne2->_vertex = s1->vtx();
   _new_edg(ne1); ne2->_edge=ne1->edg();
   _new_fac(ne2);
   ne1->_face=s1->fac(); s1->_face->_symedge=ne1;

   DEF_SPLICE_VARS;
   SPLICE ( s1->pri(), ne1 );
   SPLICE ( s2->pri(), ne2 );

   //   s2 = ne2->nxt();
   while ( s2!=ne2 ) { s2->_face=ne2->fac(); s2=s2->nxt(); }

   CHECKALL;
   SR_TRACE1("Ok.");

   return ne2;
 }

SeBase *SeMeshBase::kev ( SeBase *x )
 {
   SR_TRACE1("kev...");

   if ( _op_check_parms )
    { if ( _edges==1 ) return _op_error(OpKevLastEdge);
      if ( x->rot()!=x ) return _op_error(OpKevManyEdges);
      _op_last_msg = OpNoErrors;
    }

   SeBase *xn = x->nxt();
   xn->_vertex->_symedge = xn;
   xn->_face->_symedge = xn;

   _del_vtx ( x->vtx() );
   _del_edg ( x->edg() );

   SeBase *xsp = x->sym()->pri();
   DEF_SPLICE_VARS;
   SPLICE ( xsp, x );
   delete x->nxt(); delete x;

   CHECKALL;
   SR_TRACE1("Ok.");

   return xsp->nxt();
 }

SeBase *SeMeshBase::kev ( SeBase *x, SeBase **s )
 {
   SR_TRACE1("kev2...");

   SeBase *xs = x->sym();
   SeBase *xp = x->pri();
   SeBase *xsp = xs->pri();

   if ( _op_check_parms )
    { if ( _edges==1 ) return _op_error(OpKevLastEdge);
      if ( x->rot()==x ) return _op_error(OpKevOneEdge);
      _op_last_msg = OpNoErrors;
    }

   x->nxt()->vtx()->_symedge = x->nxt();
   x->fac()->_symedge = xp;
   x->sym()->fac()->_symedge = xsp;

   _del_vtx ( x->vtx() );
   _del_edg ( x->edg() );

   DEF_SPLICE_VARS;
   SPLICE ( x,  xs );
   SPLICE ( xsp, x );
   SPLICE ( xp, xs );

   delete x->nxt(); delete x;

   x=xp=xp->sym(); xs=xsp->sym();

   while ( x!=xs )
    { x->_vertex=xs->vtx();
      x=x->rot();
      if(x==xp) break;  // Prevents infinite loop by incoherent args
    }
   *s=xs;

   CHECKALL;
   SR_TRACE1("Ok.");

   return xp;
 }

SeBase *SeMeshBase::kef ( SeBase *x, SeBase **s )
 {
   SR_TRACE1("kef...");

   SeBase *xs  = x->sym();
   SeBase *xp  = x->pri();
   SeBase *xsp = xs->pri();

   if ( _op_check_parms )
    { if ( x->fac()==xs->fac() ) return _op_error(OpKefSameFace);
      _op_last_msg = OpNoErrors;
    }

   x->fac()->_symedge = xp;
   xs->fac()->_symedge = xsp;
   x->vtx()->_symedge = xs->nxt();
   xs->vtx()->_symedge = x->nxt();

   _del_edg ( x->edg() );
   _del_fac ( x->fac() );

   DEF_SPLICE_VARS;
   SPLICE ( xsp, x );
   SPLICE ( xp, xs );
   delete x->nxt(); delete x;

   xp=xp->nxt(); x=xs=xsp->nxt();

   while ( x!=xp ) { x->_face=xp->fac(); x=x->nxt(); }
   if (s) *s=xs; 

   CHECKALL;
   SR_TRACE1("Ok.");

   return xp;
 }

SeBase *SeMeshBase::mg ( SeBase *s1, SeBase *s2 ) // elems of s2 will be deleted
 {
   SR_TRACE1("mg...");

   if ( _op_check_parms )
    { if ( vertices_in_face(s1)!=vertices_in_face(s2) ) return _op_error(OpMgUncompatibleFaces);
      if ( s1->fac()==s2->fac() ) return _op_error(OpMgSameFace);
      _op_last_msg = OpNoErrors;
    }

   _del_fac ( s1->fac() );
   _del_fac ( s2->fac() );

   SrArray<SeBase*> contour(0,256);
   SeBase *x1, *x2, *s;

   // Not fully tested: it seems that some fixes will be required like vtx->_symedge=vtx, etc...

   x1=s1; x2=s2;
   do { _del_edg ( x2->edg() );
        _del_vtx ( x2->vtx() );
        x2->sym()->_edge = x1->edg();
        contour.push()=x1; contour.push()=x2;
        contour.push()=x1->sym(); contour.push()=x2->sym();
        x1 = x1->nxt();
        s=x2; do { s->_vertex=x1->vtx(); s=s->rot(); } while (s!=x2);
        x2 = x2->pri();
      } while ( x1!=s1 );

   for ( int i=0; i<contour.size(); i+=4 )
    { contour[i+2]->_next->_rotate=contour[i+3];
      contour[i+3]->_next->_rotate=contour[i+2];
      delete contour[i];
      delete contour[i+1];
    }

   CHECKALL;
   return s;
 }

SeBase *SeMeshBase::flip ( SeBase *x )
 {
   SR_TRACE1("flip...");

   SeBase *xs = x->sym();
   SeBase *xp = x->pri();
   SeBase *xsp = xs->pri();

   if ( _op_check_parms )
    { if ( x->fac()==xs->fac() ) return _op_error(OpFlipOneFace);
      _op_last_msg = OpNoErrors;
    }

   DEF_SPLICE_VARS;
   SPLICE ( xsp,  x );
   SPLICE ( xp,  xs );

   xp = xp->nxt();
   xsp = xsp->nxt();

   x->_vertex  = xp->nxt()->vtx();
   xs->_vertex = xsp->nxt()->vtx();
   xsp->_face  = xs->fac();
   xp->_face   = x->fac();

   xp->vtx()->_symedge  = xp;     
   xsp->vtx()->_symedge = xsp;
   xsp->fac()->_symedge = xsp;
   xp->fac()->_symedge  = xp;

   SPLICE ( xp,  xs );
   SPLICE ( xsp, x  );

   CHECKALL;
   SR_TRACE1("Ok.");

   return xs;
 }

SeBase *SeMeshBase::ecol ( SeBase *x, SeBase **s )
 {
   SR_TRACE1("ecol...");

   SeBase *s1, *s2, *z;

   if ( _op_check_parms )
    { if ( x->fac()==x->sym()->fac() ) return _op_error(OpEdgColOneFace);
      _op_last_msg = OpNoErrors;
    }

   s1 = kev ( x, &s2 );
   s1 = kef ( s1, &z );
   s2 = kef ( s2->sym()->nxt(), &z );

   *s = s2->rot();

   return s1;   
 }

SeBase *SeMeshBase::vsplit ( SeBase *s1, SeBase *s2 )
 {
   SR_TRACE1("vtxsplit using mev...");

   if ( _op_check_parms )
    { if ( s1==s2 ) return _op_error(OpVtxSplitParmsEqual);
      if ( s1->vtx()!=s2->vtx() ) return _op_error(OpVtxSplitNotSameVtx);
      _op_last_msg = OpNoErrors;
    }

   SeBase* s2s = s2->sym();
   mef ( s2s->nxt(), s2s );
   s1 = mef ( s1, s1->nxt() );

   return mev ( s1, s2 );
 }

SeBase* SeMeshBase::addv ( SeBase* x )
 {
   SR_TRACE1("addv...");

   if ( _op_check_parms )
    { _op_last_msg = OpNoErrors;
    }

   SeBase *y = mev ( x );

   x=x->nxt();
   do { y = mef ( y, x );
        x=x->nxt();
      } while ( x->nxt()!=y );

   return y;
 }

SeBase* SeMeshBase::delv ( SeBase* y )
 {
   SR_TRACE1("delv...");

   if ( _op_check_parms )
    { _op_last_msg = OpNoErrors;
    }

   SeBase *s2;

   while ( y!=y->rot() ) y = kef ( y, &s2 );

   return kev ( y );
 }

//============================ End of File ===============================
