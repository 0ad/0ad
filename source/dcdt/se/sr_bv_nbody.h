/** \file sr_bv_nbody.h 
 * multibody AABB seep and prune */

# ifndef SR_BV_NBODY_H
# define SR_BV_NBODY_H

# include "sr_bv_math.h"
# include "sr_bv_id_pairs.h"

class SrModel;
class SrMat;

/*! Maintains a set of three linked lists and keeps updating them using the
    sweep and prune algorithm. At any point in execution, the class maintains
    a set of pairs of objects whose AABBs overlap. In general, this set would
    be very small compared to the n(n-1)/2 possible pairs.
    Adapted from the VCollide package. Their copyright is in the source file. */
class SrBvNBody
{ private:
   struct AABB;
   struct EndPoint;
   EndPoint *_elist[3]; //Pointers to the three linked lists, one per axis
   SrArray<AABB*> _aabb; //Element "i" points to the AABB of the object with id=i. If
                         // this object doesn't exist, then the pointer is NULL.
  public:
   /*! at any instance during execution, overlapping_pairs contains the set
       of pairs of ids whose AABBs overlap. */
   SrBvIdPairs overlapping_pairs;
   
  public:
   /*! Constructor */
   SrBvNBody();

   /*! Destructor (calls init()) */
  ~SrBvNBody();
  
   /*! Clears everything */
   void init ();
   
   /*! Add an object with the given id to the NBody data structure.
       If the id already exists, nothing is done and false is returned.
       Otherwise, the object is added and true is returned.
       Note: to change a model associated to an existing id, delete_object()
       must be called prior to add_object() */
   bool insert_object ( int id, const SrModel& m );
   
   /*! Update position and orientation of the associated object, and 
       the 3 per-axis linked lists maintaining the ordered AABB endpoints.
       False is returned if the id is invalid; true is returned otherwise. */
   bool update_transformation ( int id, const SrMat& m );
   
   /*! Delete the object from the data structure. Returns true if the
       object could be deleted, and false otherwise. False may only
       occur if the id is invalid or represents an already deleted object */
   bool remove_object ( int id );

  private:
   void _list_insert ( int coord, EndPoint* newpt );
   void _add_pair(int id1, int id2)
    { if (id1 != id2) overlapping_pairs.add_pair(id1, id2); }
   void _del_pair(int id1, int id2)
    { if (id1 != id2) overlapping_pairs.del_pair(id1, id2); }
};

#endif // SR_BV_NBODY_H
