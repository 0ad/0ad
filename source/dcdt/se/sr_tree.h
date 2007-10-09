
/** \file sr_tree.h 
 * A red-black balanced search tree */

# ifndef SR_TREE_H
# define SR_TREE_H

# include "sr_class_manager.h"
# include "sr_output.h"

/*! \class SrTreeNode sr_tree_node.h
    \brief A red-black node for SrTree

    SrTreeNode is the node that classes should derive in order to be
    inserted in SrTree. This class has all its data members as public, 
    so that the user can use them as needed. There are no methods for 
    manipulation of the nodes inside this class, all such manipulation 
    methods are in SrTreeBase. */
class SrTreeNode
 { public :
    /*! The color type of the node. */
    enum Color { Red, Black };

    /*! SrTreeNode::null is initialized in sr_tree_node.cpp, and serves
        as a sentinel indicating a null node. SrTreeNode::null is a black
        node where its pointers are initialized as pointing to itself, but
        the parent pointer may keep arbitrary values during manipulation
        inside SrTreeBase. */
    static SrTreeNode* null;
   public :
    Color color;        //!> Color of the node.
    SrTreeNode* parent; //!> Pointer to node's parent.
    SrTreeNode* left;   //!> Pointer to node's left child.
    SrTreeNode* right;  //!> Pointer to node's righ child.
   public :
    /*! Default Constructor. */
    SrTreeNode ( Color c=Red ) : color(c), parent(null), left(null), right(null) {}
   public :
    /*! Sets all links to SrTreeNode::null, but leaves color unchanged. */
    void init () { parent=left=right=null; }
 };

/*! \class SrTreeBase sr_tree.h
    \brief red-black tree base class

    This class contains all methods for tree manipulation. The user should
    however use the template class SrTree for an implementation that
    includes automatic type casts for the user type. A manager to the
    user data is required, which must be a class deriving SrTreeNode.
    For details about red black trees, see:
    T. H. Cormen, C. E. Leiserson, and R. L. Rivest, "Introduction to Algorithms"
    1990, MIT, chapter 14, ISBN 0-262-03141-8
    <br><br>
    Note:
    A binary search tree is a red-black tree if it satisfies : <br>
    1. Every node is either red or black <br>
    2. Every null leaf is black <br>
    3. If a node is red then both its children are black <br>
    4. Every simple path from a node to a descendant leaf contains the same number of black nodes <br> */
class SrTreeBase
 { private :
    SrTreeNode *_root;        // the root of the tree
    SrTreeNode *_cur;         // the current element of the tree
    SrClassManagerBase* _man; // manager of user data, that derives SrListNode
    int _elements;            // number of elements of the tree
    int  _search_node ( const SrTreeNode* key );
    void _rotate_right ( SrTreeNode* x );
    void _rotate_left ( SrTreeNode* x );
    void _rebalance ( SrTreeNode* x );
    void _fix_remove ( SrTreeNode* x );

   public :

    /*! Initiates an empty tree. The class manager must manage a user class
        deriving from SrTreeNode. */
    SrTreeBase ( SrClassManagerBase* m );

    /*! Copy constructor. The class manager of t is shared. */
    SrTreeBase ( const SrTreeBase& t );

    /*! Destructor. */
   ~SrTreeBase ();

    /*! Deletes all elements of the tree. */
    void init ();

    /*! Returns true iff there is no nodes in the tree. */
    bool empty () const { return _root==SrTreeNode::null? true:false; } 
  
    /*! Returns the number of elements of the tree. */
    int elements () const { return _elements; }

    /*! Method to find the minimum node of the subtree rooted at the given node x. */
    SrTreeNode* get_min ( SrTreeNode* x ) const;

    /*! Method to find the maximum node of the subtree rooted at the given node x. */
    SrTreeNode* get_max ( SrTreeNode* x ) const;

    /*! Method to find the successor node of the given node x in the tree. */
    SrTreeNode* get_next ( SrTreeNode* x ) const;

    /*! Method to find the predecessor node of the given node x in the tree. */
    SrTreeNode* get_prior ( SrTreeNode* x ) const;

    /*! Returns the current element being pointed, that will be SrTreeNode::null
        if the tree is emptyis returned. */
    SrTreeNode* cur () const { return _cur; }

    /*! Sets the current element to be c, which must be a node of the tree. */
    void cur ( SrTreeNode* c ) { _cur = c; }

    /*! Returns the root of the tree, that will be SrTreeNode::null if the tree is empty. */
    SrTreeNode* root () { return _root; }

    /*! Returns the first element of the tree, ie, the minimum according
        to the comparison function. */
    SrTreeNode* first () const { return get_min(_root); }

    /*! Returns the last element of the tree, ie, the maximum. */
    SrTreeNode* last () const { return get_max(_root); }

    /*! Will put the current position cur() pointing to the node with minimum value.
        If the list is empty, cur will point to SrTreeNode::null. */
    void gofirst () { _cur = get_min(_root); }

    /*! Will put the current position cur() pointing to the node with maximum value.
        If the list is empty, cur will point to SrTreeNode::null. */
    void golast () { _cur = get_max(_root); }

    /*! Returns the next element of the current position cur().
        If cur points to null, SrTreeNode::Null is returned. */
    SrTreeNode* curnext () const { return get_next(_cur); }

    /*! Returns the prior element of the current position cur().
        If cur points to null, SrTreeNode::null is returned. */
    SrTreeNode* curprior () const { return get_prior(_cur); }

    /*! Will put the current position cur() pointing to the next
        node curnext(), cur can become SrTreeNode::null. */
    void gonext () { _cur = get_next(_cur); }

    /*! Will put the current position cur() pointing to the prior
        link curprior(), cur can become SrTreeNode::null. */
    void goprior () { _cur = get_prior(_cur); }

    /*! Returns a pointer to the item that is equal to the given key, or 0
        if it could not find the key in the tree. cur will be the last node
        visited during the search. */
    SrTreeNode* search ( const SrTreeNode *key );

    /*! If inserted, will return key, otherwise will return 0, and cur will 
        point to key. Duplication is not allowed. */
    SrTreeNode* insert ( SrTreeNode* key );

    /*! Tries to insert key using method insert(). In case of duplication
        key is not inserted, key is deleted, and 0 is returned. The returned
        node will be pointing to key in case of sucees, or pointing to the
        node in the tree that is equal to key in case of failure. In all cases,
        cur will be the last node visited during the search for key. */
    SrTreeNode* insert_or_del ( SrTreeNode* key );

    /*! Duplicates and inserts all elements of t in the tree. */
    void insert_tree ( const SrTreeBase& t );

    /*! Extracts node z that must be inside the tree; z is returned. */
    SrTreeNode* extract ( SrTreeNode* z );

    /*! Removes and delete node z, which must be inside the tree. */
    void remove ( SrTreeNode* z );

    /*! Extracts the item equal to the key and return it or 0 if the 
        key was not found. */
    SrTreeNode* search_and_extract ( const SrTreeNode* key );

    /*! Removes and deletes the item equal to the key. Returns true if a node
        is found and deleted, otherwise false is returned. */
    bool search_and_remove ( const SrTreeNode* key );

    /*! Take control of the tree in t, and set t to an empty tree. Both
        trees must manage the same derived class of SrTreeNode. */
    void take_data ( SrTreeBase& t );

    /*! Copy operator */
    void operator= ( const SrTreeBase& t );

    /*! Outputs the tree in the format: [e1 e2 en] */
    friend SrOutput& operator<< ( SrOutput& o, const SrTreeBase& t );
 };

/*! \class SrTree sr_tree.h
    \brief red-black balanced tree

    SrTree defines automatic type casts to the user type, which must
    derive SrTreeNode. To traverse the tree, the first, last, next,
    and prior keywords are related to the order defined by the comparison
    method in the class manager. For documentation of the methods
    see the documentation of the base class SrTreeBase methods. */
template <class X>
class SrTree : public SrTreeBase
 { public:

    /*! Default constructor that automatically creates a SrClassManager<X>. */
    SrTree () : SrTreeBase ( new SrClassManager<X> ) {}

    /*! Constructor with a given class manager. */
    SrTree ( SrClassManagerBase* m ) : SrTreeBase ( m ) {}

    /*! Copy constructor with class manager sharing. */
    SrTree ( const SrTree& t ) : SrTreeBase ( t ) {}

    X* cur () { return (X*)SrTreeBase::cur(); }
    void cur ( X* c ) { SrTreeBase::cur((SrTreeNode*)c); }
    X* root () { return (X*)SrTreeBase::root(); }
    X* first () { return (X*) SrTreeBase::first(); }
    X* last () { return (X*) SrTreeBase::last(); }
    X* curnext () const { return (X*)SrTreeBase::curnext(); }
    X* curprior () const { return (X*)SrTreeBase::curprior(); }
    X* search ( const X* key ) { return (X*)SrTreeBase::search(key); }
    X* insert ( X* key ) { return (X*)SrTreeBase::insert(key); } 
    X* insert_or_del ( X* key ) { return (X*)SrTreeBase::insert_or_del(key); }
    X* extract ( X* n ) { return (X*) SrTreeBase::extract(n); }
    void remove ( X* n ) { SrTreeBase::remove(n); }
    X* search_and_extract ( const X* key ) { return (X*) SrTreeBase::search_and_extract(key); }
    bool search_and_remove ( const X* key ) { return SrTreeBase::search_and_remove(key); }
 };

/*! Base class for iterating over trees. */
class SrTreeIteratorBase
 { private :
    SrTreeNode* _cur;
    SrTreeNode* _first;
    SrTreeNode* _last;
    const SrTreeBase& _tree;

   public :
    /*! Constructor */
    SrTreeIteratorBase ( const SrTreeBase& t );

    /*! Returns the current node being pointed by the iterator */
    SrTreeNode* cur () const { return _cur; }

    /*! Returns the first node in the associated tree */
    SrTreeNode* getfirst () const { return _first; }

    /*! Returns the last node in the associated tree */
    SrTreeNode* getlast () const { return _last; }

    /*! Must be called each time the associate tree is changed */
    void reset ();

    /*! Points the iterator to the first element. */
    void first () { _cur=_first; }

    /*! Points the iterator to the last element. */
    void last () { _cur=_last; }

    /*! Advances the current position of the iterator to the next one */
    void next () { _cur=_tree.get_next(_cur); }

    /*! Walk back the current position of the iterator of one position */
    void prior () { _cur=_tree.get_prior(_cur); }

    /*! Returns true if get() points to a valid position */
    bool inrange () { return _cur==SrTreeNode::null? false:true; }

    /*! Returns the current element, can return SrTreeNode::null */
    SrTreeNode* get () { return _cur; }

    /*! Returns true if the current position is pointing to the last element. */
    bool inlast () const { return _cur==_last? true:false; }

    /*! Returns true if the current position is pointing to the first element */
    bool infirst () const { return _cur==_first? true:false; }
 };

/*! Derives SrTreeIteratorBase providing correct type casts for the user type */
template <class X>
class SrTreeIterator : public SrTreeIteratorBase
 { public :
    SrTreeIterator ( const SrTree<X>& s ) : SrTreeIteratorBase(s) {}
    X* get () { return (X*)SrTreeIteratorBase::get(); }
    X* operator-> () { return (X*)SrTreeIteratorBase::get(); }
 };



//============================ End of File =================================

# endif // SR_TREE_H
