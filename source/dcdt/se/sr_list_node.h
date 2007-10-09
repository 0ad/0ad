
# ifndef SR_LIST_NODE_H
# define SR_LIST_NODE_H

/** \file sr_list_node.h 
 * A double linked circular list node */

# include "sr.h"

/*! \class SrListNode sr_list_node.h
    \brief Elements of SrList must derive SrListNode

    SrListNode is a node of a double linked circular list. 
    It does not have virtual destructors 
    in order to save the space overhead of virtual functions.

    We do not use splice() to insert/remove, and this implies:
    1. A node that is inserted must not be inside any list.
    2. A node that is removed do not form a single circular list (call init() after for this)
    3. But insert / remove methods are faster.

   \code
    splice guide :
    l->remove_next()  == l->splice(l->next());
    l->remove_prior() == l->prior->splice(l->prior()->prior());
    l->remove()       == l->splice(l->prior());
    splice(a,b) implementation :
     { swap(a->next()->prior(),b->next()->prior()); swap(a->next(),b->next()); } 
    Example of a splice usage: 
     l1: A->B->C, l2: X->Y->Z <=> splice(A,X) <=> l3: A->Y->Z->X->B->C
   \endcode */
class SrListNode
 { 
   private :
    SrListNode* _next, *_prior;

   public :

    /*! Constructs a node n where the next and prior pointers refers to n. */
    SrListNode () { _next=_prior=this; }

    /*! Returns the next node of the list. */
    SrListNode* next () const { return _next; }

    /*! Returns the prior node of the list. */
    SrListNode* prior () const { return _prior; }

    /*! Makes the next and prior pointers refers to itself. */
    void init () { _next=_prior=this; }

    /*! Returns true if the node is only, ie, iff next==this. */
    bool only () const { return _next==this? true:false; }

    /*! Removes the next node from the list and return it. */
    SrListNode* remove_next ();

    /*! Removes the prior node from the list and return it. */
    SrListNode* remove_prior ();

    /*! Removes itself from the list and returns itself. */
    SrListNode* remove ();

    /*! Replaces itself by n on the list, and returns itself. */
    SrListNode* replace ( SrListNode* n );

    /*! Inserts n in the list after itself and returns n. */
    SrListNode* insert_next ( SrListNode* n );

    /*! Inserts n in the list before itself and returns n. */
    SrListNode* insert_prior ( SrListNode* n );

    /*! Operator splice(), does the same as: 
        swap(a->next->prior,b->next->prior); swap(a->next,b->next); */
    void splice ( SrListNode* n );

    /*! Swaps the position of itself and n in the list by readjusting their pointers. */
    void swap ( SrListNode* n );
 };

//============================== end of file ===============================

# endif // SR_LIST_NODE_H
