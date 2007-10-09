
# ifndef SR_LIST_H
# define SR_LIST_H

/** \file sr_list.h 
 * Manages a circular linked list */

# include "sr_class_manager.h" 
# include "sr_list_node.h" 
# include "sr_output.h"

/*! \class SrListBase sr_list.h
    \brief Base class for SrList

    SrListBase implements methods for managing a double linked 
    circular list. The user should however use the template
    class SrList for an implementation that includes automatic
    type casts for the user type. A manager to the user
    data is required, which must be a class deriving SrListNode. */
class SrListBase
 { private :
    SrListNode* _first;       // first element
    SrListNode* _cur;         // current element
    SrClassManagerBase* _man; // manager of user data, that derives SrListNode
    int _elements;            // the number of elements in the list

   public :

    /*! Initiates an empty list. The class manager must manage a user class
        deriving from SrListNode. */
    SrListBase ( SrClassManagerBase* m );

    /*! Copy constructor. The class manager of l is shared. */
    SrListBase ( const SrListBase& l );

    /*! Constructor from a given node. the list takes control of the nodes
        headed by n. The number of elements is set to zero, but it can be 
        adjusted with method elements() if required.
        The class manager must be compatible with the derived type of n. */
    SrListBase ( SrListNode* n, SrClassManagerBase* m );

    /*! The destructor deletes all nodes from memory. */
   ~SrListBase ();

    /*! Returns the manager of the user data */
    SrClassManagerBase* class_manager() const { return _man; }

    /*! Deletes all elements of the list. */
    void init ();

    /*! Returns true if there is no elements in the list. */
    bool empty () const { return _first? false:true; } 

    /*! Changes the internal elements counter maintained during list manipulations.
        If e<0, will count all nodes to correctly update the internal counter,
        otherwise will just take the new amount e. Having a wrong value has no
        consequences. */
    void elements ( int e );

    /*! Returns the current number of elements in the list. */
    int elements () const { return _elements; }

    /*! Returns the current element being pointed by the SrListBase internal pointer. */
    SrListNode* cur () const { return _cur; }

    /*! Sets the internal current element pointer to n. It is the user responsability 
        to ensure that n makes part of the list controlled by SrListBase. */
    void cur ( SrListNode* n ) { _cur = n; }

    /*! Returns the first element of the list. */
    SrListNode* first () const { return _first; }
  
    /*! Sets the internal first element pointer to n. Its the user responsability 
        to ensure that n makes part of the list controlled by SrListBase */
    void first ( SrListNode* n ) { _first = n; }
 
    /*! Returns the last element of the list. */
    SrListNode* last () const { return _first? _first->prior():0; }

    /*! Sets the last element of the list to be n by adjusting the internal first pointer.
        It is the user responsability to ensure that n makes part of the list controlled 
        by SrListBase. (as the list is circular, only the first pointer is mantained). */
    void last ( SrListNode* n ) { _first = n->next(); }

    /*! Puts the current position cur() pointing to the first node. */
    void gofirst () { _cur=_first; }

    /*! Puts the current position cur() pointing to the last node. */
    void golast () { if (_first) _cur=_first->prior(); }

    /*! Puts the current position cur() pointing to the next node curnext(); 
        Attention: this method cannot be called if the list is empty! */
    void gonext () { _cur=_cur->next(); }

    /*! Puts the current position cur() pointing to the prior node curprior();
        Attention: this method cannot be called if the list is empty! */
    void goprior () { _cur=_cur->prior(); }

    /*! Returns the next element of the current position cur().
        Attention: this method cannot be called if the list is empty! */
    SrListNode* curnext () const { return _cur->next(); }
    
    /*! Returns the prior element of the current position cur().
        Attention: this method cannot be called if the list is empty! */
    SrListNode* curprior () const { return _cur->prior(); }

    /*! This method calls gonext(), and returns true iff the last element was not reached.
        Use this to iterate over a hole list, like : \code 
        if ( !l.empty() )
         { l.gofirst();
           do { l.cur()->do_something(); 
              } while ( l.notlast() );
         } \endcode
        Attention: this method cannot be called if the list is empty! */
    bool notlast () { gonext();  return _cur==_first? false:true; }

    /*! This method calls goprior(), and returns true iff the first element was not reached.
        Use this to iterate over a hole list in backwards, like : \code 
        if ( !l.empty() )
         { l.golast();
           do { l.cur()->do_something(); 
              } while ( l.notfirst() );
         } \endcode
        Attention: this method cannot be called if the list is empty! */
    bool notfirst () { goprior(); return _cur->next()==_first? false:true; }

    /*! Returns true iff the current position is pointing to the last element.
        Attention: this method cannot be called if the list is empty! */
    bool inlast () const { return _cur->next()==_first? true:false; }

    /*! Returns true iff the current position is pointing to the first element, 
        or if the list is empty. */
    bool infirst () const { return _cur==_first? true:false; }

    /*! Extracts the first element of the list, the cur position is set to be
        the next element. */
    SrListNode* pop_front () { _cur=_first; return extract(); }

    /*! Extracts the last element of the list, the cur position is left as the 
        first element. */
    SrListNode* pop_back () { golast(); return extract(); }

    /*! Inserts n before the first element and makes n be the first element of 
        the list and also the current one. */
    void push_front ( SrListNode* n ) { _cur=_first; insert_prior(n); _first=_cur; }

    /*! Inserts n after the last element and makes n be the last element of the list 
        and also the current one. */
    void push_back ( SrListNode* n ) { golast(); insert_next(n); } 

    /*! Inserts n after the current element. The current element becomes n. */
    void insert_next ( SrListNode* n );

    /*! Allocates a new element, inserting it after the current one.
        The current element becomes the new one, and is returned. */
    SrListNode* insert_next ();

    /*! Inserts n prior to the current element. The current element becomes n. */
    void insert_prior ( SrListNode* n );

    /*! Allocates a new element, inserting it after the current one.
        The current element becomes the new one, and is returned. */
    SrListNode* insert_prior ();

    /*! Inserts a copy of list l after the cur position, list l stays unchanged. */
    void insert_list ( const SrListBase& l ) { insert_list(l._first); } 

    /*! Inserts a copy of the list pointed by l after the cur position, list l stays unchanged. */
    void insert_list ( const SrListNode *l );

    /*! Replaces the current element by n, only swaping their pointers.
        The original current element is not deleted but returned. */
    SrListNode* replace ( SrListNode* n );

    /*! Extract the current element and return it (without deleting it). If the list
        is empty, 0 is returned. The current element becomes the next one, and the
        same for the first element if it is removed. */
    SrListNode* extract ();

    /*! Removes the current element calling extract() and deletes it. */
    void remove ();

    /*! Does a selection sort. The current position stays at the first element. */
    void sort ();

    /*! Inserts the node in its sorted position. Return the last comparison result:
        >0 if it was inserted as the last element in the list, <0 if it was inserted
        in the middle of the list, and 0 if it was inserted just before a duplicated
        element. */
    int insort ( SrListNode* n );

    /*! Linear search considering that the list is sorted.
        Current position will point the found element if true is returned. */
    bool search ( const SrListNode *n );

    /*! Linear search that will test all elements in the list, case needed for
        when the list is not sorted.
        Current position will point the found element if true is returned. */
    bool search_all ( const SrListNode *n );

    /*! Get control of the nodes in list l, and set l to be an empty list.
        The data manager of l and SrList must be of the same type. */
    void take_data ( SrListBase& l );

    /*! Get control of the nodes headed by n. The number of elements is set to
        zero, but it can be adjusted with method elements() if required.
        The data manager of SrList must be compatible with the derived n type. */
    void take_data ( SrListNode* n );

    /*! Returns the first element of the list, and set the list to be empty. */
    SrListNode* leave_data ();

    /*! Outputs the list in the format: [e1 e2 en ]. */
    friend SrOutput& operator<< ( SrOutput& o, const SrListBase& l );
 };

/*! \class SrList sr_list.h
    \brief Manages a circular linked list of derived classes X of SrLink

    SrList defines automatic type casts to the user type, which must
    derive SrListNode. For documentation of the methods
    see the documentation of the base class SrListBase methods. */
template <class X>
class SrList : public SrListBase
 { public :
    /*! Default constructor that automatically creates a SrClassManager<X>. */
    SrList () : SrListBase ( new SrClassManager<X> ) {}

    /*! Constructor with a given class manager. */
    SrList ( SrClassManagerBase* m ) : SrListBase ( m ) {}

    /*! Copy constructor. Inititates the list as a copy of l, 
        duplicating all elements and sharing the class manager. */
    SrList ( const SrList& l ) : SrListBase ( l ) {}

    /*! Constructor from a given node. The list takes control of the nodes
        headed by n. The number of elements is set to zero, but it can be 
        adjusted later with method elements() if required. */
    SrList ( X* n ) : SrListBase ( n, new SrClassManager<X> ) {}

    X* cur () const            { return (X*)SrListBase::cur(); }
    void cur ( X* n )          { SrListBase::cur((SrListNode*)n); }
    X* first () const          { return (X*)SrListBase::first(); }
    void first ( X* n )        { SrListBase::first((SrListNode*)n); }
    X* last ()                 { return (X*) SrListBase::last(); }
    void last ( X* n )         { SrListBase::last((SrListNode*)n); }
    X* curnext () const        { return (X*)SrListBase::curnext(); }
    X* curprior () const       { return (X*)SrListBase::curprior(); }
    X* pop_front ()            { return (X*)SrListBase::pop_front(); }
    X* pop_back ()             { return (X*)SrListBase::pop_back(); }
    void push_front ( X* n )   { SrListBase::push_front(n); }
    void push_back ( X* n )    { SrListBase::push_back(n); }
    void insert_next ( X* n )  { SrListBase::insert_next(n); }
    X* insert_next ()          { return (X*)SrListBase::insert_next(); }
    void insert_prior ( X* n ) { SrListBase::insert_prior(n); }
    X* insert_prior ()         { return (X*)SrListBase::insert_prior(); }
    X* replace ( X* n )        { return (X*)SrListBase::replace(n); }
    X* extract ()              { return (X*)SrListBase::extract(); }
    void operator= ( const SrList& l ) { init(); insert_list(l); }
 };

/*! Base class for iterating over lists. */
class SrListIteratorBase
 { private :
    SrListNode* _cur;
    SrListNode* _first;
    SrListNode* _last;
    char _rcode;
    const SrListBase* _list;
    SrListNode* _node;

   public :
    /*! Constructor */
    SrListIteratorBase ( const SrListBase& l );

    /*! Constructor */
    SrListIteratorBase ( SrListNode* n );

    /*! Returns the current element being pointed by the iterator */
    SrListNode* get () const { return _cur; }

    /*! Returns the first element being pointed by the iterator */
    SrListNode* getfirst () const { return _first; }

    /*! Returns the last element being pointed by the iterator */
    SrListNode* getlast () const { return _last; }

    /*! Must be called each time the associate list is changed */
    void reset ();

    /*! Points the iterator to the first element. */
    void first () { _cur=_first; _rcode=1; }

    /*! Points the iterator to the last element. */
    void last () { _cur=_last; _rcode=1; }

    /*! Advances the current position of the iterator of one position */
    void next () { _cur=_cur->next(); }

    /*! Walk back the current position of the iterator of one position */
    void prior () { _cur=_cur->prior(); }

    /*! This method only makes sense when called in a loop, like in: the following
           for ( it.first(); it.inrange(); it.next() ) { ... }
        or for ( it.last(); it.inrange(); it.prior() ) { ... } */
    bool inrange ();

    /*! Returns true if the current position is pointing to the last element. */
    bool inlast () const { return _cur==_last; }

    /*! Returns true if the current position is pointing to the first element, 
        or if the list is empty. */
    bool infirst () const { return _cur==_first; }
 };

/*! Derives SrListIteratorBase providing correct type casts for the user type */
template <class X>
class SrListIterator : public SrListIteratorBase
 { public :
    SrListIterator ( const SrList<X>& l ) : SrListIteratorBase(l) {}
    SrListIterator ( X* n ) : SrListIteratorBase((SrListNode*)n) {}
    X* get () { return (X*)SrListIteratorBase::get(); }
    X* operator-> () { return (X*)SrListIteratorBase::get(); }
 };

//============================== end of file ===============================

# endif // SR_LIST_H

