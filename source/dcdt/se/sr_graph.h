
# ifndef SR_GRAPH_H
# define SR_GRAPH_H

/** \file sr_graph.h 
 * Graph maintenance classes
 */

# include "sr_array.h"
# include "sr_list.h"

class SrGraphNode;
class SrGraphBase;

/*! SrGraphLink contains the minimal needed information for
    a link. To attach user-related information, SrGraphLink
    must be derived and as well a corresponding 
    SrClassManagerBased, which will manage only the user data. */
class SrGraphLink
 { private :
    SrGraphNode* _node; // node pointing to
    sruint _index;      // index for traversing
    float _cost;        // cost for minimum paths
    int _blocked;       // used as boolean or as a ref counter
    friend class SrGraphNode;
    friend class SrGraphBase;
   protected :
	SrGraphLink () { _index=0; _cost=0; _node=0; _blocked=0; }
   ~SrGraphLink () {}
   public :
    void cost ( float c ) { _cost=c; }
    float cost () const { return _cost; }
    SrGraphNode* node () const { return _node; }
    sruint index () const { return _index; }
    int blocked () const { return _blocked; }
    void blocked ( bool b ) { _blocked = b? 1:0; }
    void blocked ( int b ) { _blocked = b; }
  };

/*! SrGraphNode contains the minimal needed information for
    a node. To attach user-related information, SrGraphLink
    must be derived and as well a corresponding 
    SrClassManagerBased, which will manage only the user data. */
class SrGraphNode : public SrListNode
 { private :
    SrArray<SrGraphLink*> _links;
    sruint _index;
    int _blocked;       // used as boolean or as a ref counter
    SrGraphBase* _graph;
    friend class SrGraphBase;
   protected :
    SrGraphNode () { _index=0; _graph=0; _blocked=0; }
   ~SrGraphNode ();

   public :

    /*! Returns the next node in the global list of nodes */
    SrGraphNode* next() const { return (SrGraphNode*) SrListNode::next(); }

    /*! Returns the prior node in the global list of nodes */
    SrGraphNode* prior() const { return (SrGraphNode*) SrListNode::prior(); }

	/*! Returns the index of this node */
    sruint index () { return _index; }

    int blocked () const { return _blocked; }
    void blocked ( bool b ) { _blocked = b? 1:0; }
    void blocked ( int b ) { _blocked = b; }

	/*! Compress internal links array */
    void compress () { _links.compress(); }

	/*! Returns the new link created */
    SrGraphLink* linkto ( SrGraphNode* n, float cost=0 );

    /*! Remove link of index ni, with a fast remove: the order in the
	    links array is not mantained */
    void unlink ( int ni );

    /*! Remove the link pointing to n. Should be called ONLY when it is
        guaranteed that the link to n really exists! Uses method unlink(ni) */
    void unlink ( SrGraphNode* n ) { unlink ( search_link(n) ); }

    /*! Returns the index in the links array, or -1 if not linked to n */
    int search_link ( SrGraphNode* n ) const;

    /*! Returns the link pointing to n. Should be called ONLY when it is
        guaranteed that the link to n really exists! */
    SrGraphLink* link ( SrGraphNode*n ) { return _links[search_link(n)]; }

    /*! Returns the link index i, which must be a valid index */
    SrGraphLink* link ( int i ) { return _links[i]; }

    /*! Returns the last link */
    SrGraphLink* last_link () { return _links.top(); }

    /*! Returns the number of links in this node */
    int num_links () const { return _links.size(); }

	/*! Returns the links array */
    const SrArray<SrGraphLink*>& links() const { return _links; }

   private :
    void output ( SrOutput& o ) const;
 };

/*! The following define is to be used inside a user derived class
    of SrGraphLink, in order to easily redefine public SrGraphLink
    methods with correct type casts. */
# define SR_GRAPH_LINK_CASTED_METHODS(N,L) \
N* node() const { return (N*)SrGraphLink::node(); }

/*! The following define is to be used inside a user derived class
    of SrGraphNode, in order to easily redefine public SrGraphNode
    methods with correct type casts. */
# define SR_GRAPH_NODE_CASTED_METHODS(N,L) \
N* next() const { return (N*)SrListNode::next(); } \
N* prior() const { return (N*)SrListNode::prior(); } \
L* linkto ( N* n, float c ) { return (L*)SrGraphNode::linkto(n,c); } \
L* link ( N* n ) { return (L*)SrGraphNode::link(n); } \
L* link ( int i ) { return (L*)SrGraphNode::link(i); } \
const SrArray<L*>& links() const { return (const SrArray<L*>&) SrGraphNode::links(); }

class SrGraphPathTree;

/*! SrGraphBase maintains a directed graph with nodes and links. Links around
    a node do not have any order meaning.
    Note also that the user should avoid redundant links, as no tests are done
    (for speed purposes) in several methods.
    SrGraphBase is the base class, the user should use SrGraph template instead. */
class SrGraphBase
 { private :
    SrList<SrGraphNode> _nodes;
    SrArray<SrGraphNode*> _buffer;
    sruint _curmark;
    char _mark_status;
    SrGraphPathTree* _pt;
    SrClassManagerBase* _lman; // link manager for a class deriving SrGraphLink
    char _leave_indices_after_save;

   public :
    /*! Constructor requires managers for nodes and links */
    SrGraphBase ( SrClassManagerBase* nm, SrClassManagerBase* lm );

    /*! Destructor deletes all data in the graph */
   ~SrGraphBase ();

    /*! Returns a pointer to the node manager */
    SrClassManagerBase* node_class_manager() const { return _nodes.class_manager(); }

    /*! Returns a pointer to the link manager */
    SrClassManagerBase* link_class_manager() const { return _lman; }

    /*! Make the graph empty */
    void init ();

    /*! Compress all link arrays in the nodes */
    void compress ();

    /*! Returns the number of nodes in the graph */
    int num_nodes () const { return _nodes.elements(); }

    /*! Counts and returns the number of (directional) links in the graph */
    int num_links () const;

    /*! Methods for marking nodes and links */
    void begin_marking ();
    void end_marking ();
    bool marked ( SrGraphNode* n );
    void mark ( SrGraphNode* n );
    void unmark ( SrGraphNode* n );
    bool marked ( SrGraphLink* l );
    void mark ( SrGraphLink* l );
    void unmark ( SrGraphLink* l );

    /*! Methods for indexing nodes and links */
    void begin_indexing ();
    void end_indexing ();
    sruint index ( SrGraphNode* n );
    void index ( SrGraphNode* n, sruint i );
    sruint index ( SrGraphLink* l );
    void index ( SrGraphLink* l, sruint i );

    /*! Returns the list of nodes, that should be used with
        care to not invalidate some SrGraph operations. */
    SrList<SrGraphNode>& nodes () { return _nodes; }

    /*! Inserts node n in the list of nodes, as a new
        unconnected component. n is returned. */
    SrGraphNode* insert ( SrGraphNode* n );

    /*! Extract (without deleting) the node from the graph. Nothing
        is done concerning possible links between the graph and
        the node being extracted */
    SrGraphNode* extract ( SrGraphNode* n );

    /*! Removes and deletes the node from the graph. Its the user
        responsibility to ensure that the graph has no links with
        the node being deleted */
    void remove_node ( SrGraphNode* n );

    /*! Searches and removes the edge(s) linking n1 with n2 if any.
        Links are removed with a "fast remove process" so that indices
        in the links array of the involved nodes may be modified.
        Return the number of (directed) links removed. */
    int remove_link ( SrGraphNode* n1, SrGraphNode* n2 );

    /*! Links n1 to n2 and n2 to n1, with cost c in both directions */
    void link ( SrGraphNode* n1, SrGraphNode* n2, float c=0 );

    /*! Returns the first node of the list of nodes */
    SrGraphNode* first_node () const { return _nodes.first(); }

    /*! Get all directed edges of the graph. Note that if n1 is linked
        to n2 and n2 is linked to n1, both edges (n1,n2) and (n2,n1) will
        appear in the edges array */
    void get_directed_edges ( SrArray<SrGraphNode*>& edges );

    /*! Get all edges of the graph without duplications from the directional
        point of view. */
    void get_undirected_edges ( SrArray<SrGraphNode*>& edges );

    /*! Get all nodes which are in the same connected component of source */
    void get_connected_nodes ( SrGraphNode* source, SrArray<SrGraphNode*>& nodes );

    /*! Organize nodes by connected components. The indices in array components say each
        start and end position in array nodes for each component */
    void get_disconnected_components ( SrArray<int>& components, SrArray<SrGraphNode*>& nodes );

    /*! The returned path contains pointers to existing nodes in the graph.
        In the case the two nodes are in two different disconnected components
        an empty path is returned. If n1==n2 a path with the single node n1
        is returned. In all cases, returns the distance (cost) of path.
        In case no path is found, the optional parameters distfunc and udata
        can be used to return the path to the closest processed node to the goal.  */
    float get_shortest_path ( SrGraphNode* n1, SrGraphNode* n2, SrArray<SrGraphNode*>& path,
                              float (*distfunc) ( const SrGraphNode*, const SrGraphNode*, void* udata )=0,
                              void* udata=0 );

    /*! Performs a A* search from startn, until finding endn. The search is
        stopped if either maxnodes or maxdist is reached. If these parameters
        are <0 they are not taken into account. True is returned if endn could
        be reached, and parameters depth and dist will contain the final
        higher depth and distance (along the shortest path) reached. */
    bool local_search ( SrGraphNode* startn, SrGraphNode* endn,
                        int maxnodes, float maxdepth, int& depth, float& dist );

    /*! When set to true, graph search will consider a link blocked if any of its
        directions is blocked, i.e., blocking only one direction of a link will
        block both the directions. Afftected methods are get_shortest_path() and
        local_search(). Default is false. */
    void bidirectional_block_test ( bool b );

    bool are_near_in_graph ( SrGraphNode* startn, SrGraphNode* endn, int maxnodes );

    /*! If this is set to true, it will be the user responsibility to call
        end_indexing() after the next graph save. It is used to retrieve the indices
        used during saving to reference additional data to be saved in derived classes. */
    void leave_indices_after_save ( bool b ) { _leave_indices_after_save = b? 1:0; }

    /*! Returns the internal buffer, which might be usefull for some specific needs;
        see for instance the description of the input() method. */
    SrArray<SrGraphNode*>& buffer () { return _buffer; }

    /*! Outputs the graph in the format: [ (l1..lk)e1 (..)e2 (..)en ].
        Nodes are indexed (starting from 0), and after the output
        all indices of the nodes are set to 0. */
    void output ( SrOutput& o );

    /*! Initializes the current graph and load another one from the given input.
        Method buffer() can be used to retrieve an array with all nodes loaded,
        indexed by the indices used in the loaded file */
    void input ( SrInput& i );

    friend SrOutput& operator<< ( SrOutput& o, SrGraphBase& g ) { g.output(o); return o; }

    friend SrInput& operator>> ( SrInput& i, SrGraphBase& g ) { g.input(i); return i; }

   private :
    void _normalize_mark();

 };

/*! SrGraph is a template that includes type casts for the user
    derived types (of SrGraphNode and SrGraphLink) to correctly
    call SrGraphBase methods. */
template <class N, class L>
class SrGraph : public SrGraphBase
 { public :
    SrGraph () : SrGraphBase ( new SrClassManager<N>, new SrClassManager<L> ) {}

    SrGraph ( SrClassManagerBase* nm, SrClassManagerBase* lm ) : SrGraphBase ( nm, lm ) {}

    const SrList<N>& nodes () { return (SrList<N>&) SrGraphBase::nodes(); }

    N* insert ( N* n ) { return (N*) SrGraphBase::insert((SrGraphNode*)n); }
    N* extract ( N* n )  { return (N*) SrGraphBase::extract((SrGraphNode*)n); }
    N* first_node () const { return (N*) SrGraphBase::first_node(); }

    void get_undirected_edges ( SrArray<N*>& edges )
      { SrGraphBase::get_undirected_edges( (SrArray<SrGraphNode*>&)edges ); }

    void get_disconnected_components ( SrArray<int>& components, SrArray<N*>& nodes )
     { SrGraphBase::get_disconnected_components( components, (SrArray<SrGraphNode*>&)nodes ); }

    float get_shortest_path ( N* n1, N* n2, SrArray<N*>& path,
                              float (*distfunc) ( const SrGraphNode*, const SrGraphNode*, void* udata )=0,
                              void* udata=0  )
      { return SrGraphBase::get_shortest_path((SrGraphNode*)n1,(SrGraphNode*)n2,(SrArray<SrGraphNode*>&)path,distfunc,udata); }

    SrArray<N*>& buffer () { return (SrArray<N*>&) SrGraphBase::buffer(); }

    friend SrOutput& operator<< ( SrOutput& o, SrGraph& g ) { return o<<(SrGraphBase&)g; }
    friend SrInput& operator>> ( SrInput& i, SrGraph& g ) { return i>>(SrGraphBase&)g; }
 };

//================================ End of File =================================================

# endif  // SR_GRAPH_H

