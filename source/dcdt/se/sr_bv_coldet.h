/** \file sr_bv_coldet.h 
 * multi model collision test */

# ifndef SR_BV_COLDET_H
# define SR_BV_COLDET_H


# include "precompiled.h"
# include "sr_set.h"
# include "sr_bv_nbody.h"
# include "sr_bv_tree_query.h"

/* SrBvColdet uses SrBvNBody in order to efficiently determine
   collisions between several dynamic objects.
   Adapted from the VCollide package. Their copyright is in the source file. */
class SrBvColdet
 { private:
    class Object;
    friend class SrBvColdetObjectManager;
    SrSet<Object>* _set;
    SrBvNBody _nbody;
    SrBvIdPairs _disabled;
    SrBvTreeQuery _query;
    SrArray<int> _pairs;

   public:
    /*! Constructor */
    SrBvColdet ();

    /*! Destructor (calls init()) */
   ~SrBvColdet ();
  
    /*! Clears everything */
    void init ();

	/*! Create a new object and returns its id. */
    int insert_object ( const SrModel& m );

    /*! Removes (and deletes) the object from the database. */
    void remove_object ( int id );

    /*! Returns true if id represents a valid id in the database, and false otherwise */
    bool id_valid ( int id ) { return _getobj(id)? true:false; }

	/*! Update the transformation applied to an object.
	    Only rotations and translations are supported. */
    void update_transformation ( int id, const SrMat& m );

	/*! Turn on collision detection for an object. */
    void activate_object ( int id );

	/*! Turn off collision detection for an object. */
    void deactivate_object ( int id );

	/*! Turn on collision detection between a specific pair of objects. */
    void activate_pair ( int id1, int id2 );
    
	/*! Turn off collision detection between a specific pair of objects. */
    void deactivate_pair ( int id1, int id2 );

    /*! Returns true if the given pair of objects is deactivated and
        false otherwise (-1 ids return false). */
    bool pair_deactivated ( int id1, int id2 ) { return _disabled.pair_exists(id1,id2); }

    /*! Counts and returns the number of deactivated pairs */
    int count_deactivated_pairs () { return _disabled.count_pairs(); }

    /*! Returns the array with the ids of the colliding objects in
        the last collide_all() or has_collisions() query. The id
        pairs are sequentially stored in the array. Therefore, the number
        of collisions = colliding_pairs.size()/2. */
    const SrArray<int>& colliding_pairs () const { return _pairs; }

    /*! Perform collision detection only untill finding a first collision.
        True is returned if a collision was found; false is returned otherwise. */
    bool collide ()
     { return _collide ( SrBvTreeQuery::CollideFirstContact ); }

    /*! Perform collision detection among all pairs of objects. The results can
        be retrieved with colliding_pairs() */
    bool collide_all ()
     { return _collide ( SrBvTreeQuery::CollideAllContacts ); }
    
    /*! Returns true as soon as the distance between one pair of models
        is found to be smaller than the given tolerance.
        False is returned when all pairs respects the minimum clearance. */
    bool collide_tolerance ( float toler ) { return _collide_tolerance(toler); }

   private :
    bool _collide ( SrBvTreeQuery::CollideFlag flag );
    bool _collide_tolerance ( float toler );
    Object* _getobj ( int id ) { return ( id<0 || id>_set->maxid() )? 0:_set->get(id); }
  };

#endif // SR_BV_COLDET_H
