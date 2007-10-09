/** \file sr_bv_id_pairs.h 
 * maintains a set of id pairs */

# ifndef SR_BV_ID_PAIRS_H
# define SR_BV_ID_PAIRS_H

# include "sr_array.h"

/*! Stores a set of pairs of integers. It is assumed that the data is sparse
    (i.e. the size of this set is small as compared to the n*(n-1)/2 possible pairs).
    A pair (id1, id2) exists <==> max(id1, id2) exists in the linked list pointed
    to by elements[min(id1, id2)].
    Adapted from the VCollide package. Their copyright is in the source file. */
class SrBvIdPairs
 { public:
     /*! The node of the linked list containing the 2nd ids of all pairs
        starting with the same id. It is a single connected list, with
        the last pointer as null */
    struct Elem { int id; Elem *next; };

   private :
    SrArray<Elem*> _arr; 

   public :
    /*! Constructor */
    SrBvIdPairs ();

    /*! Destructor */
   ~SrBvIdPairs ();

    /*! Returns the array of elements. Each element[i] points to the sorted list
        with all pairs (i,x) in the structure, i<x. If no pairs start with i,
        element[i] will be NULL, therefore some entries may have null pointers. */
    const SrArray<Elem*>& elements() const { return _arr; }

    /*! Add a pair of ids to the set */
    void add_pair ( int id1, int id2 );

    /*! Delete a pair from the set */
    void del_pair ( int id1, int id2 );

    /*! Delete all pairs containing id */
    void del_pairs_with_id ( int id );

    /*! Empty the set */
    void init ();
    
    /*! Check if a pair of ids exists (-1 ids return false) */
    bool pair_exists ( int id1, int id2 );
    
    /*! Counts and returns the number of existing pairs */
    int count_pairs ();
};

#endif // SR_BV_ID_PAIRS_H
