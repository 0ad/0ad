#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_cfg_planner.h"

//# define SR_USE_TRACE1    // not used
//# define SR_USE_TRACE2    // update
//# define SR_USE_TRACE3    // bridge
# include "sr_trace.h"

//============================= SrCfgPlannerBase ========================================

bool SrCfgPlannerBase::update_lazy ( float step, int tries, float prec )
 {
   SrCfgNode* n;          // new node added to the current tree
   SrCfgNode* nearest1;   // nearest node in tree1
   SrCfgNode* nearest2;   // nearest node in tree2
   float dist1;           // distance from crand to nearest1
   float dist2;           // distance from crand to nearest2
   srcfg* crand = _tmpc1; // the random configuration

   SR_TRACE2 ( "UPDT: expanding tree " << _curtree );

   if ( _juststarted )
    { _juststarted = false;
      if ( _test_bridge(_tree1.root(),_tree2.root(),prec) ) return true; // FOUND
    }

   _cman->random ( crand );
   nearest1 = _tree1.search_nearest ( crand, &dist1 );
   nearest2 = _tree2.search_nearest ( crand, &dist2 );

   float dist = _cman->dist(nearest1->cfg(),nearest2->cfg());
   if ( dist<=step )
    { if ( _test_bridge(nearest1,nearest2,prec) ) return true; // FOUND
    }

   SR_TRACE2 ( "UPDT: nearest1="<<nearest1->id()<<" nearest2="<<nearest2->id() );
   SR_TRACE2 ( "UPDT: expanding..." );
   if ( _curtree==1 )
    { n = _tree1.expand_node ( nearest1, crand, step, tries, dist1 );
      if ( n )
        { if ( _test_bridge(n,nearest2,prec) ) return true; // FOUND
        }
    }
   else
    { n = _tree2.expand_node ( nearest2, crand, step, tries, dist2 );
      if ( n )
        { if ( _test_bridge(nearest1,n,prec) ) return true; // FOUND
        }
    }

   _curtree = _curtree==1? 2:1;

   SR_TRACE2 ( "UPDT: not found." );

   return false; // not found
 }

void SrCfgPlannerBase::_heap_add_branch ( SrCfgTreeBase* tree, SrCfgNode* n )
 {
   HeapEdge e;
   SrCfgNode *parent;
   while ( n->parent() )
    { parent = n->parent();
      if ( !parent->safe(n->parentlink()) )
       { e.tree = tree;
         e.n1 = parent; // convention: e.n1 is parent of e.n2
         e.n2 = n;
         _heap.insert ( e, parent->level(n->parentlink()) );
       }
      n = parent; // move to the parent
    }
 }

/*! Test if the path formed by connecting node index n1 of tree 1 with
    node index n2 of tree2 is a valid path. The test performs collision
    detection in the edges of the path incrementing their levels. A
    priority queue is used to first test edges in lower levels.
    If all edges in the path become safe, a path is formed and true is returned.
    Otherwise, the edge found to be invalid is deleted, the two trees are
    updated to keep the remaining edges, and false is returned */
bool SrCfgPlannerBase::_test_bridge ( SrCfgNode* n1, SrCfgNode* n2, float prec )
 {
   // add the cfg of n2 to tree1:
   SrCfgNode* n12 = _tree1.add_node ( n1, n2->cfg() );
 
   // make priority heap where the cost is the edge level
   // and add all non-safe path edges of tree 1 and tree 2 to the heap
   SR_TRACE3 ( "BRIDGE: building heap..." );
   _heap.init();
   _heap_add_branch ( &_tree1, n12 );
   _heap_add_branch ( &_tree2, n2 );

   // test and increment the level of the edges in the heap
   int level;
   HeapEdge e;
   while ( _heap.size()>0 )
    { level = _heap.lowest_cost();
      e = _heap.top();
      
      SR_TRACE3 ( "BRIDGE: heap size="<<_heap.size()<<" level="<<level<<" ..." );

      if ( !e.tree->increment_edge_level ( e.n1, e.n2, prec ) ) break; // collision found

      // remove and reinsert edge if not yet safe:
      _heap.remove();
      if ( !e.n1->safe(e.n2->parentlink()) )
         _heap.insert ( e, level+1 );
    }

   if ( _heap.size()==0 ) // all edges were safe: path found
    { SR_TRACE3 ( "BRIDGE: path found!" );
      _solved = true;
      _path.init ();
      _tree1.get_branch ( n1, _path );
      _path.revert();
      _tree2.get_branch ( n2, _path );
      SR_TRACE3 ( "BRIDGE: path done." );
    }
   else // failed, reorganize trees
    { SR_TRACE3 ( "BRIDGE: failed, transferring subtrees..." );

      if ( e.tree==&_tree1 )
       { //sr_out<<"\nTRANSFER 1: "<<e.n1->id()<<srspc<< e.n2->id()<<srspc<< n12->id()<<srspc<< n2->id()<<srnl;
         _tree1.transfer_subtree ( e.n1, e.n2, n12/*joint1*/, _tree2, n2/*joint2*/ );
       }
      else
       {  //sr_out<<"\nTRANSFER 2: "<<e.n1->id()<<srspc<< e.n2->id()<<srspc<< n2->id()<<srspc<< n12->id()<<srnl;
         _tree2.transfer_subtree ( e.n1, e.n2, n2/*joint1*/, _tree1, n12/*joint2*/ );
       }

      SR_TRACE3 ( "BRIDGE: transfer done." );
      _solved = false;
    }
    
   return _solved;
 }

