
# ifndef SR_CFG_PLANNER_H
# define SR_CFG_PLANNER_H

//# include <SR/sr_cfg_path.h>
//# include <SR/sr_cfg_tree.h>
# include "sr_heap.h"
# include "sr_cfg_path.h"
# include "sr_cfg_tree.h"

/*! A single-query, bidirectional, lazy and sampling-based planner */
class SrCfgPlannerBase : public SrSharedClass
 { private :
    SrCfgTreeBase _tree1, _tree2;
    SrCfgPathBase _path;
    SrCfgManagerBase* _cman;
    srcfg* _tmpc1;
    srcfg* _tmpc2;
    int _curtree;
    bool _solved;
    bool _juststarted;
    struct HeapEdge { SrCfgTreeBase* tree; SrCfgNode* n1; SrCfgNode* n2; };
    SrHeap<HeapEdge,int> _heap;

   public :

    /*! The constructor requires a configuration manager. */
    SrCfgPlannerBase ( SrCfgManagerBase* cman );

    /*! Destructor frees all used internal data, and unref the associated
        configuration managers */
   ~SrCfgPlannerBase ();

    /*! Returns the roadmap tree rooted at the source configuration */
    SrCfgTreeBase& tree1 () { return _tree1; }

    /*! Returns the roadmap tree rooted at the destination configuration */
    SrCfgTreeBase& tree2 () { return _tree2; }

    /*! Returns the number of nodes in both trees */
    int nodes () const { return _tree1.nodes()+_tree2.nodes(); }

    /*! Clears everything */
    void init ();

    /*! Clears everything and define the source and goal configurations.
        Configurations c1 and c2 must be valid.
        If a time-varying problem will be solved, configuration c1 must correspond
        to the start and c2 to the goal, ie c1 happens before c2 */
    void start ( const srcfg* c1, const srcfg* c2 );

    /*! Returns true if a path to the goal was found, and false otherwise. */
    bool solved () const { return _solved; }

    /*! Returns the last path found by the planner */
    SrCfgPathBase& path () { return _path; }

    /*! Update one of the trees, returning true if a path was found
        Parameter step is the incremental step distance, and
        tries is the number of bisections to try in case of expantion failure */
    bool update_rrt ( float step, int tries, float prec );

    /*! Lazy version of the update method. */
    bool update_lazy ( float step, int tries, float prec );

   private :
    bool _test_bridge ( SrCfgNode* n1, SrCfgNode* n2, float prec );
    bool _try_to_join ( SrCfgNode* n1, SrCfgNode* n2, float prec );
    void _heap_add_branch ( SrCfgTreeBase* tree, SrCfgNode* n );
 };

/*! Planner template for user-defined configurations */
template <class C>
class SrCfgPlanner : public SrCfgPlannerBase
 { public :
    SrCfgPlanner ( SrCfgManagerBase* cman )
      : SrCfgPlannerBase(cman) { }

    SrCfgTree<C>& tree1 () { return (SrCfgTree<C>&) SrCfgPlannerBase::tree1(); }
    SrCfgTree<C>& tree2 () { return (SrCfgTree<C>&) SrCfgPlannerBase::tree2(); }

    SrCfgPath<C>& path () { return (SrCfgPath<C>&) SrCfgPlannerBase::path(); }
 };

//================================ End of File =================================================

# endif  // SR_CFG_PLANNER_H

