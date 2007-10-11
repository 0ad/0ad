
# ifndef SR_CFG_TREE_H
# define SR_CFG_TREE_H

# include "sr_set.h"
//# include <SR/sr_cfg_manager.h>
# include "sr_cfg_manager.h"
# include "sr_cfg_path.h"

class SrCfgTreeBase;

/*! SrCfgNode is a node of SrCfgTree. Each edge has a 
    level based on the number of collision checks performed:
        Level TotTests NewTests Segments
          0      2        0        1
          1      3        1        2
          2      5        2        4
          3      9        4        8
          k  (2^k)+1   2^(k-1)    2^k
    Safe edge is achieved if dist/(2^k) < collision precision,
    and level is marked as safe when this occurs */
class SrCfgNode
 { private :
    int _bufferid;
    srcfg* _cfg;
    SrCfgNode* _parent;
    int _parentlink;
    struct Link { SrCfgNode* node; float dist; int level; };
    SrArray<Link> _children;
    friend class SrCfgTreeBase;
   public :
    SrCfgNode* parent () const { return _parent; }
    int parentlink () const { return _parentlink; } // -1 if root node
    int children () const { return _children.size(); }
    srcfg* cfg () const { return _cfg; }
    SrCfgNode* child ( int i ) const { return _children[i].node; }
    float prec ( int i ) const; // dist/2^abs(level)
    float dist ( int i ) const { return _children[i].dist; }
    int level ( int i ) const { return _children[i].level; } // <0 if safe
    bool safe ( int i ) const { return level(i)<0?true:false; }
    void get_subtree ( SrArray<SrCfgNode*>& nodes ); // add the node itself and all subtree
    int id () const { return _bufferid; }
   private :
    void _deledge ( int e );
    void _fixplinks ();
    void _reroot ();
 };

/*! The tree-based roadmap */
class SrCfgTreeBase
 { private :
    SrCfgManagerBase*   _cman;    // configuration manager
    SrCfgNode*          _root;    // the root of the tree
    SrArray<SrCfgNode*> _buffer;  // buffer of nodes
    SrArray<int>        _freepos; // free positions in buffer
    SrArray<SrCfgNode*> _nodes;   // for temporary use
   public :

    /*! The constructor requires a valid configuration manager for
        dealing with the user-defined configuration */
    SrCfgTreeBase ( SrCfgManagerBase* cman );

    /*! Destructor frees all used internal data, and unref the
        associated configuration manager */
   ~SrCfgTreeBase ();

    /* Returns a pointer to the used configuration manager */
    SrCfgManagerBase* cman () const { return _cman; }

    /*! Debug tool to test all internal pointers, should always return true */
    bool check_all ( SrOutput& o );

    /*! Set the tree as an empty tree */
    void init ();

    /*! Set the current tree to be a tree containing
        only the given node as the root of the tree. */
    void init ( const srcfg* cfg );

    /*! Add a node to the tree, as a child to the provided parent.
        If dist<0 (the default) the parent-cfg distance is retrieved
        by using the associated configuration manager.
        The level of the parent-cfg edge is set to 0.
        Returns the new node created. */
    SrCfgNode* add_node ( SrCfgNode* parent, const srcfg* cfg, float dist=-1 );

    /*! Returns the number of nodes in the tree */
    int nodes () const { return _buffer.size()-_freepos.size(); }

    /*! Returns the root node or null if tree empty */
    SrCfgNode* root () const { return _root; }

    /*! Performs a O(n) search over all nodes and returns the node
        with the closest configuration to c. Null is returned if the
        tree is empty. If given pointer d is not null, the nearest
        distance is returned in d */
    SrCfgNode* search_nearest ( const srcfg* c, float* d=0 );

    /*! Add a child node from node source, walking a distance of step in direction
        to configuration direction. If the node is not valid, step is divided
        by 2 until the node becomes valid, or until max_tries tentatives are performed.
        Note that here edges are not tested for validity, only nodes (lazy evaluation).
        The new node is returned, or null in case no node could be added.
        If parameter dist>0, it will be used as being dist(source->cfg(),direction). */
    SrCfgNode* expand_node ( SrCfgNode* source, const srcfg* direction, float step, int maxtries, float dist=-1 );

    /*! Same as expand node, but here the expansion is done only if the new edge is
        valid, therefore the edge visibility tests is called */
    SrCfgNode* expand_node_safe ( SrCfgNode* source, const srcfg* direction,
                                  float step, int maxtries, float prec, float dist=-1 );

    /*! Returns a list of nodes forming the tree branch joining node n to the root.
        The first element of the array is always n, and the last is always the root node.
        The array size is not set to zero, ie, the indices are apended to the array. */
    void get_branch ( SrCfgNode* n, SrArray<SrCfgNode*>& nodes );

    /*! Same as the other get_branch() method, but the result goes to a path object */
    void get_branch ( SrCfgNode* n, SrCfgPathBase& path );

    /*! Returns an unordered list with all nodes in the tree */
    void get_nodes ( SrArray<SrCfgNode*>& nodes );

    /*! Performs 2^(k-1) collision tests to check if the edge [n1,n2] can be promoted
        to level k = n1->level(n2->parentlink())+1.
        False is returned in case the level could not be promoted due to a collision.
        True is returned if the level could be promoted, and in this case, the edge
        will be marked as safe if the new level achieves the required precision
        with the following test: n1->dist(n2->parentlink())/2^k < prec */
    bool increment_edge_level ( SrCfgNode* n1, SrCfgNode* n2, float prec );

    /*! Removes the edge [n1,n2] and transfers the disconnected subtree to tree2, 
        by "identifying" joint1 with joint2 of tree2 and reorganizing the subtree
        so as to make joint2 the subtree root.
        Important: n1 must be parent of n2, and joint1 must be in n2 subtree */
    void transfer_subtree ( SrCfgNode* n1, SrCfgNode* n2, SrCfgNode* joint1,
                            SrCfgTreeBase& tree2, SrCfgNode* joint2 );
    
    /*! Output of the roadmap tree for inspection:
        - if printcfg is true, node data is also sent to output
        - n is the root of the subgraph to print (if 0, the real root is taken)*/
    void output ( SrOutput& o, bool printcfg=true, SrCfgNode* n=0 );
                                  
   private :
    SrCfgNode* _newnode ();
    void _delnode ( SrCfgNode* n );
    srcfg* _tmpnode ();
    void _nearest ( SrCfgNode* n, const srcfg* c, SrCfgNode*& nearest, float& mindist );
 };

/*! This template version performs all required type casts to bind the
    tree to the user-defined configuration class. Class C must be managed
    by the used configuration manager */
template <class C>
class SrCfgTree : public SrCfgTreeBase
 { public :
    /*! Constructor receiving a user-defined manager */
    SrCfgTree ( SrCfgManagerBase* cman ) : SrCfgTreeBase(cman) { }
    
    /*! Automatically allocates a manager using SrCfgManager<C> template */
    SrCfgTree () : SrCfgTreeBase ( new SrCfgManager<C> ) { }
    
    /*! Returns the configuration at node index n */
    C* cfg ( SrCfgNode* n ) const { return (C*)n->cfg(); }
 };

//================================ End of File =================================================

# endif  // SR_CFG_TREE_H

