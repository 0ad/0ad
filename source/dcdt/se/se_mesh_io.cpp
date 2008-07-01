#include "precompiled.h"
#include "0ad_warning_disable.h"

# include <string.h>
# include "se_mesh.h"

//# define SR_USE_TRACE1  // IO trace
# include "sr_trace.h"

//================================ IO =========================================

# define ID(se) intptr_t(se->_edge)

void SeMeshBase::_elemsave ( SrOutput& out, SeMeshBase::ElemType type, SeElement* first )
 {
   SrClassManagerBase* man;
   SeElement *e;
   int esize;
   const char* st;

   if ( type==TypeVertex )
    { esize=_vertices; man=_vtxman; st="Vertices"; }
   else if ( type==TypeEdge )
    { esize=_edges; man=_edgman; st="Edges"; }
   else
    { esize=_faces; man=_facman; st="Faces"; }

   out << srnl << st << srspc << esize << srnl;
   //fprintf ( f, "\n%s %d\n", st, esize );

   e = first;
   do { e->_index = 0;
        out << ID(e->_symedge) <<srspc;
        //fprintf ( f, "%d ", ID(e->_symedge) );
        man->output(out,e);
        out << srnl;
        //fprintf ( f, "\n" );
        e = e->nxt(); 
      } while (e!=first);

   SR_TRACE1 ( esize << " elements written !" );
 }

bool SeMeshBase::save ( SrOutput& out )
 {
   SeBase *se;
   SeElement *el, *eli;
   int symedges, j;
   intptr_t i;

   out << "SYMEDGE MESH DESCRIPTION\n\n";
   //fprintf ( f, "SYMEDGE MESH DESCRIPTION\n\n" );
   SR_TRACE1("Header written.");

   symedges = _edges*2;
   out << "SymEdges " << symedges << srnl;
   //fprintf ( f, "SymEdges %d\n", symedges );

   if ( empty() ) return true;

   SrArray<SeBase*> ses(symedges);
   SrArray<SeEdge*> edg(symedges);

   // get adjacency information of symedges to save:  
   SR_TRACE1("Mounting S array...");
   i=0; 
   el = eli = _first->edg();
   do { se = el->se();
        el->_index = i/2;
        for ( j=0; j<2; j++ )
         { if ( j==1 ) se = se->sym();
           ses[i] = se;
           edg[i] = se->_edge;     // save edge pointer
           se->_edge = (SeEdge*)i; // and use it to keep indices
           i++;
	     }
        el = el->nxt();
      } while ( el!=eli );

   // adjust vtx and fac indices:
   SR_TRACE1("Adjusting indexes...");

   i = 0;
   el = eli = _first->vtx();
   do { el->_index=i++; el=el->nxt(); } while (el!=eli);

   i = 0;
   el = eli = _first->fac();
   do { el->_index=i++; el=el->nxt(); } while (el!=eli);

   for ( i=0; i<ses.size(); i++ )
    { out << ID(ses[i]->_next) << srspc
          << ID(ses[i]->_rotate) << srspc
          << ses[i]->_vertex->_index << srspc
          << edg[i]->_index << srspc
          << ses[i]->_face->_index << srspc
          << srnl;
      /*fprintf ( f, "%d %d %d %d %d\n",
                ID(ses[i]->_next),
                ID(ses[i]->_rotate),
                ses[i]->_vertex->_index,
                edg[i]->_index,
                ses[i]->_face->_index );*/
    }

   SR_TRACE1("Symedges written.");

   _elemsave ( out, TypeVertex, _first->vtx() );
   _elemsave ( out, TypeEdge,   edg[0] );
   _elemsave ( out, TypeFace,   _first->fac() );

   _curmark = 1;
   _marking = _indexing = false;
   for ( i=0; i<ses.size(); i++ ) ses[i]->_edge = edg[i];

   SR_TRACE1 ( "save OK !" );
   return true;
 }

# undef ID

//---------------------------------- load --------------------------------

/*
static int load_int ( FILE* f )
 {
   int i;
   fscanf ( f, "%d", &i );
   return i;
 }

static void skip ( FILE* f, int n )
 {
   char s[60];
   while ( n-- ) fscanf ( f, "%s", s );
 }
*/
void SeMeshBase::_elemload ( SrInput& inp, SrArray<SeElement*>& E,
                             const SrArray<SeBase*>& S, SrClassManagerBase* man )
 {
   int i, x;

   inp.get_token(); // skip elem type label
   //skip ( f, 1 ); // skip elem type label

   inp >> x; E.size(x);
   //x = load_int(f); E.size(x);
   
   for ( i=0; i<E.size(); i++ )
    { inp >> x; //x = load_int(f);
      E[i] = (SeElement*)man->alloc();
      E[i]->_symedge = S.get(x);
      man->input ( inp, E[i] );
      //man->read ( E[i], f );
      if ( i>0 ) E[0]->_insert(E[i]);
    }

   SR_TRACE1 ( e.size() << " elements loaded !" );
 }

bool SeMeshBase::load ( SrInput& inp )
 {
   //char buf[64];
   int i;
   intptr_t x;

   //fscanf ( f, "%s", buf ); if ( strcmp(buf,"SYMEDGE") ) return false;
   //fscanf ( f, "%s", buf ); if ( strcmp(buf,"MESH") ) return false;
   //fscanf ( f, "%s", buf ); if ( strcmp(buf,"DESCRIPTION") ) return false;
   inp.get_token(); if ( inp.last_token()!="SYMEDGE" ) return false;
   inp.get_token(); if ( inp.last_token()!="MESH" ) return false;
   inp.get_token(); if ( inp.last_token()!="DESCRIPTION" ) return false;
   SR_TRACE1 ( "Signature ok." );

   // destroy actual structure to load the new one:
   destroy ();

   // load indices and store them in an array:
   inp.get_token(); // skip SymEdges label
   //skip ( f, 1 ); // skip SymEdges label
   inp >> i; //i = load_int ( f );
   SrArray<SeBase*> S(i);
   for ( i=0; i<S.size(); i++ )
    { S[i] = new SeBase;
      inp>>x; S[i]->_next   = (SeBase*)x;
      inp>>x; S[i]->_rotate = (SeBase*)x;
      inp>>x; S[i]->_vertex = (SeVertex*)x;
      inp>>x; S[i]->_edge   = (SeEdge*)x;
      inp>>x; S[i]->_face   = (SeFace*)x;
    }
   SR_TRACE1 ( "Symedges loaded." );

   // load infos:
   SrArray<SeElement*> V;
   SrArray<SeElement*> E;
   SrArray<SeElement*> F;
   _elemload ( inp, V, S, _vtxman );
   _elemload ( inp, E, S, _edgman );
   _elemload ( inp, F, S, _facman );

   // convert indices to pointers:
   for ( i=0; i<S.size(); i++ )
    { S[i]->_next   = S[(intptr_t)(S[i]->_next)];
      S[i]->_rotate = S[(intptr_t)(S[i]->_rotate)];
      S[i]->_vertex = V[(intptr_t)(S[i]->_vertex)];
      S[i]->_edge   = E[(intptr_t)(S[i]->_edge)];
      S[i]->_face   = F[(intptr_t)(S[i]->_face)];
    }

   // adjust internal variables:
   _first     = S[0];
   _vertices  = V.size();
   _edges     = E.size();
   _faces     = F.size();
   _curmark   = 1;
   _marking = _indexing = false;

   SR_TRACE1 ( "load OK !" );
   return true;
 }
 
//=== End of File ===================================================================
