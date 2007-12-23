#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_cfg_planner.h"

//# define SR_USE_TRACE1    // start
//# define SR_USE_TRACE2    // update
//# define SR_USE_TRACE3    // bridge
# include "sr_trace.h"

//============================= SrCfgPlannerBase ========================================

SrCfgPlannerBase::SrCfgPlannerBase ( SrCfgManagerBase* cman )
          :_tree1 ( cman ), _tree2 ( cman ), _path ( cman )
 {
   _cman = cman;

   _tmpc1 = _cman->alloc();
   _tmpc2 = _cman->alloc();

   _solved = false;
   _juststarted = false;
   _curtree = 1;
 }

SrCfgPlannerBase::~SrCfgPlannerBase ()
 {
   init ();
   _cman->free ( _tmpc1 );
   _cman->free ( _tmpc2 );
 }

void SrCfgPlannerBase::init ()
 {
   _tree1.init ();
   _tree2.init ();
   _path.init ();
   _solved = false;
   _juststarted = false;
   _curtree = 1;
 }

void SrCfgPlannerBase::start ( const srcfg* c1, const srcfg* c2 )
 {
   SR_TRACE1 ( "Start...");
   init ();

   SR_TRACE1 ( "Tree Init...");
   _tree1.init ( c1 );
   _tree2.init ( c2 );
   _curtree = 1; // could be 1 or 2
   _juststarted = true;

   SR_TRACE1 ( "Start OK.");
 }

bool SrCfgPlannerBase::update_rrt ( float step, int tries, float prec )
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
      if ( _try_to_join(_tree1.root(),_tree2.root(),prec) ) return true; // FOUND
    }

   _cman->random ( crand );
   nearest1 = _tree1.search_nearest ( crand, &dist1 );
   nearest2 = _tree2.search_nearest ( crand, &dist2 );

   float dist = _cman->dist(nearest1->cfg(),nearest2->cfg());
   if ( dist<=step )
    { if ( _try_to_join(nearest1,nearest2,prec) ) return true; // FOUND
      SR_TRACE2 ( "UPDT: not found." );
      return false; // not found
    }

   SR_TRACE2 ( "UPDT: nearest1="<<nearest1->id()<<" nearest2="<<nearest2->id() );
   SR_TRACE2 ( "UPDT: expanding..." );
   if ( _curtree==1 )
    { if ( _cman->monotone ( nearest1->cfg(), crand ) )
       { n = _tree1.expand_node_safe ( nearest1, crand, step, tries, prec, dist1 );
         if ( n )
          { if ( _try_to_join(n,nearest2,prec) ) return true; // FOUND
          }
       }
    }
   else
    { if ( _cman->monotone ( crand, nearest2->cfg() ) )
       { n = _tree2.expand_node_safe ( nearest2, crand, step, tries, prec, dist2 );
         if ( n )
          { if ( _try_to_join(nearest1,n,prec) ) return true; // FOUND
          }
       }
    }

   _curtree = _curtree==1? 2:1;

   SR_TRACE2 ( "UPDT: not found." );

   return false; // not found
 }

bool SrCfgPlannerBase::_try_to_join ( SrCfgNode* n1, SrCfgNode* n2, float prec )
 {
   bool b;
   
   b = _cman->monotone ( n1->cfg(), n2->cfg() );
   if ( !b ) return false;
 
   b = _cman->visible ( n1->cfg(), n2->cfg(), _tmpc2, prec );
   if ( !b ) return false;

   _path.init ();
   _tree1.get_branch ( n1, _path );
   _path.revert();
   _tree2.get_branch ( n2, _path );

   _solved = true;
   return _solved;
 }

//============================= End of File ===========================================

