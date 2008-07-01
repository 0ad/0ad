#include "precompiled.h"
#include "0ad_warning_disable.h"

# include "sr_array.h"

# include "se_mesh.h"

# define IFTYPE(t,v,e,f) t==TypeVertex? (v) : t==TypeEdge? (e) : (f)
# define GETINFO(t,x)    t==TypeVertex? (SeElement*)x->vertex : t==TypeEdge? (SeElement*)x->edge : (SeElement*)x->face

//================================= static functions =====================================

static void fatal_error ( const char* msg )
 {
   sr_out.fatal_error ( msg );
 }

//============================= SeMeshBase Private Methods =====================================

void SeMeshBase::_defaults ()
 {
   _first = 0;
   _op_last_msg  = OpNoErrors;
   _curmark = 0;
   _marking = false;
   _indexing = false;
   _vertices = _edges = _faces = 0; 
 }

SeBase* SeMeshBase::_op_error ( OpMsg m )
 {
   _op_last_msg = (OpMsg)m; 

   if ( _output_op_errors ) 
    { sr_out << "Error in SeMeshBase operation: "
             << translate_op_msg(m) << srnl;
    }

   return (SeBase*) 0; 
 }

//============================ SeMeshBase general =====================================

semeshindex se_index_max_value = ((semeshindex)0)-1; // This is the unsigned greatest value;

SeMeshBase::SeMeshBase ( SrClassManagerBase* vtxman, SrClassManagerBase* edgman, SrClassManagerBase* facman )
 {
   _defaults ();
   _output_op_errors = 0;
   if ( !vtxman || !edgman || !facman )
     fatal_error ( "SeMeshBase::SeMeshBase(): null pointer received!" );
   _vtxman = vtxman;
   _edgman = edgman;
   _facman = facman;
   _op_check_parms = 0;
 }

SeMeshBase::~SeMeshBase ()
 {
   destroy ();
   _vtxman->unref();
   _edgman->unref();
   _facman->unref();
 }

/* from [mantyla] :
   v-e+f = h0-h1+h2 ( Betti numbers )
   h0 = shells
   h1 = 2 * genus ( every closed curve cut a part of the surface away )
   h2 = orientability = h0
   ==> v-e+f = s-2g+s = 2s-2g = 2(s-g), if s==1, => v-e+f=2-2g */
int SeMeshBase::euler () const
 {
   return _vertices - _edges + _faces;
 }

int SeMeshBase::genus () const
 {
   return (_vertices-_edges+_faces-2)/(-2);
 }

int SeMeshBase::elems ( ElemType type ) const
 {
   switch ( type )
    { case TypeVertex  : return _vertices;
      case TypeEdge    : return _edges;
      case TypeFace    : return _faces;
    }
   return 0;
 }

void SeMeshBase::invert_faces ()
 {
   SeBase *se;
   SeElement *edg, *edgi;
   int i, j, symedges = _edges*2;

   if ( !_first ) return;

   SrArray<SeBase*> nse(symedges,symedges);
   SrArray<SeBase*> nxt(symedges,symedges);
   SrArray<SeBase*> rot(symedges,symedges);
   SrArray<SeVertex*> vtx(symedges,symedges);

   i=0; 
   edg = edgi = _first->edg();
   do { se = edg->se();
        for ( j=0; j<2; j++ )
         { if ( j==1 ) se = se->sym();
           vtx[i] = se->nxt()->vtx();
           nxt[i] = se->pri();
           rot[i] = se->nxt()->sym();
           nse[i] = se;
           i++;
         }
        edg = edg->nxt();
      } while ( edg!=edgi );

   for ( i=0; i<symedges; i++ )
    { //fg_out<<i<<" "<<(int)nse[i]<<fgnl;
      nse[i]->_next   = nxt[i];
      nse[i]->_rotate = rot[i];
      nse[i]->_vertex = vtx[i];
      vtx[i]->_symedge = nse[i];
    }
 }

int SeMeshBase::vertices_in_face ( SeBase *e ) const
 {
   int i;
   SeBase *x=e->nxt();
   for ( i=1; x!=e; i++ ) x=x->nxt();
   return i;
 }

int SeMeshBase::vertex_degree ( SeBase *e ) const
 {
   int i=1;
   SeBase *x;
   for ( x=e->rot(); x!=e; i++ ) x=x->rot();
   return i;
 }

float SeMeshBase::mean_vertex_degree () const
 {
   if ( !_vertices ) return 0;

   SeVertex* vi = _first->vtx();
   SeVertex* v = vi;
   float deg=0;
   do { deg += vertex_degree(v->se());
        v = v->nxt();
      } while ( v!=vi );

   return deg / (float)_vertices;
 }

//============================ SeMeshBase marking =====================================

// replace _marking and _indexing by _mark_status !!

void SeMeshBase::_normalize_mark () // private method
 {
   SeElement *ei, *e;

   _curmark=1;

   if ( !_first ) return;

   ei=e=_first->vtx(); do { e->_index=0; e=e->nxt(); } while ( e!=ei );
   ei=e=_first->edg(); do { e->_index=0; e=e->nxt(); } while ( e!=ei );
   ei=e=_first->fac(); do { e->_index=0; e=e->nxt(); } while ( e!=ei );
 }

void SeMeshBase::begin_marking ()
 {
   if ( _indexing || _marking )
    fatal_error("SeMeshBase::begin_marking() not allowed as marking or indexing is already in use!");

   _marking = true;

   if ( _curmark==se_index_max_value )
    { _normalize_mark ();
    }
   else _curmark++;
 }

void SeMeshBase::end_marking ()
 {
   _marking = false;
 }

bool SeMeshBase::marked ( SeElement* e ) 
 {
   if ( !_marking ) fatal_error ( "SeMeshBase::marked(e): marking is not active!\n" );
   return e->_index==_curmark? true:false;
 }

void SeMeshBase::mark ( SeElement* e ) 
 { 
   if ( !_marking ) fatal_error ( "SeMeshBase::mark(e): marking is not active!\n" );
   e->_index = _curmark;
 }

void SeMeshBase::unmark ( SeElement* e ) 
 { 
   if ( !_marking ) fatal_error ( "SeMeshBase::unmark(e): marking is not active!\n");
   e->_index = _curmark-1;
 }

//============================ SeMeshBase indexing =====================================

void SeMeshBase::begin_indexing ()
 {
   if ( _marking || _indexing ) 
    fatal_error("SeMeshBase::begin_indexing() not allowed as marking or indexing is already in use!");
   _indexing = true;
 }

void SeMeshBase::end_indexing ()
 {
   _indexing = false;
   _normalize_mark ();
 }

semeshindex SeMeshBase::index ( SeElement* e )
 {
   if ( !_indexing ) fatal_error ("SeMeshBase::index(e): indexing is not active!");
   return e->_index;
 }

void SeMeshBase::index ( SeElement* e, semeshindex i ) 
 {
   if ( !_indexing ) fatal_error ("SeMeshBase::index(e,i): indexing is not active!");
   e->_index = i;
 }

//=== End of File ===================================================================
