/** \file sr_bv_tree_query.h 
 * bounding volume tree */

// TO TEST:
// 1. use of double vs. float
// 2. use inline in SrBvMath functions

# ifndef SR_BV_TREE_QUERY_H
# define SR_BV_TREE_QUERY_H

# include "sr_mat.h"
# include "sr_bv_tree.h" 

/*! Encapsulates data for collision and proximity queries between
    trees of bounding volumes.
    Adapted from PQP, see the copyright notice in the source file. */
class SrBvTreeQuery
 { private :
    // common data for all tests:
    int _num_bv_tests;
    int _num_tri_tests;
    srbvmat _R;  // xform from model 1 to model 2
    srbvvec _T;
    // for collision queries only:
    SrArray<int> _pairs;
    // for both distance and tolerance queries:
    srbvreal _dist;
    SrPnt _srp1, _srp2;
    srbvvec _p1, _p2;
    // for distance queries only:
    srbvreal _rel_err; 
    srbvreal _abs_err; 
    // for proximity queries only:
    bool _closer_than_tolerance;   
    srbvreal _toler;

   public :
    /*! Constructor */
    SrBvTreeQuery ();

    /*! Initialize all values and frees all used memory */
    void init ();

    /*! Returns the number of bounding volume tests in last query */
    int num_bv_tests() const { return _num_bv_tests; }

    /*! Returns the number of triangle intersection tests in last query */
    int num_tri_tests() const { return _num_tri_tests; }

    /*! Returns the array with the indices of the colliding faces in
        the last collide() query. The indices pairs are sequentially
        stored in the array. */
    const SrArray<int>& colliding_pairs () const { return _pairs; }

    /*! Returns the points computed in the last proximity or distance
        query. For a proximity query, these are the points that
        stablished the distance smaller than the tolerance (otherwise
        these points are not meaningful).
        For a distance query, these points stablished the minimum
        distance, within the relative and absolute error bounds specified.*/
    const SrPnt& p1 () const { return _srp1; }
    const SrPnt& p2 () const { return _srp2; }

    /*! Returns the distance computed in the last proximity or distance
        query. For a proximity query, this value is only meaningful
        if the distance is smaller than the tolerance specified */
    float distance() const { return (float)_dist; }

    /*! The boolean says whether the models in the last proximity query
        are closer than tolerance distance specified */
    bool closer_than_tolerance() const { return _closer_than_tolerance; }

    /*! Find collisions between two models given as bv trees.
        m1 is the placement of model 1 in the world &
        m2 is the placement of model 2 in the world.
        Matrices are in column-major order (as OpenGL) and must specify
        only a rigid transformation (rotation and/or translation).
        In this version, the collision test stops when the first
        contact is found; which can be retrieved with colliding_pairs();
        True is returned if a collision was found, and false otherwise. */ 
    bool collide ( const SrMat& m1, const SrBvTree* t1,
                   const SrMat& m2, const SrBvTree* t2 );

    /*! Similar to collide(), but it finds all collisions between the
        two given models. Two times the number of collisions is
        returned, ie, the size of the colliding_pairs() array. */
    int collide_all ( const SrMat& m1, const SrBvTree* t1,
                      const SrMat& m2, const SrBvTree* t2 );

    /*! Computes the distance between two models given as bv trees.
        "rel_err" is the relative error margin from actual distance.
        "abs_err" is the absolute error margin from actual distance.
        The smaller of the two will be satisfied, so set one large
        to nullify its effect.
        Returns the distance between the two models.
        Methods distance(), p1() and p2() will return the distance and
        points computed during the query.
        Note: pointers t1 and t2 are not const only because the _last_tri
        member variable of SrBvTree is updated during the query. */
    float distance ( const SrMat& m1, SrBvTree* t1,
                     const SrMat& m2, SrBvTree* t2,
                     float rel_err, float abs_err );

    /*! Checks if distance between two models (given as bv trees) is <= tolerance
        The algorithm returns whether the true distance is <= or >
        "tolerance".  This routine does not simply compute true distance
        and compare to the tolerance - models can often be shown closer or
        farther than the tolerance more trivially.  In most cases this
        query should run faster than a distance query would on the same
        models and configurations.
        The returned parameter returns the result of the query, which can
        be also retrieved with method closer_than_tolerance().
        If the models are closer than ( <= ) tolerance, the points that
        established this can be retrieved with methods p1(), and p2(),
        and the distance can be retrieved with method distance() */
    bool tolerance ( const SrMat& m1, SrBvTree* t1,
                     const SrMat& m2, SrBvTree* t2, float tolerance );

    float greedy_distance ( const SrMat& m1, SrBvTree* t1,
                           const SrMat& m2, SrBvTree* t2, float delta );

   public : // lower level methods
   
    enum CollideFlag { CollideAllContacts=1, CollideFirstContact=2 };
    
    /*! Lower level call used by the two main collide methods.
        Collision results, if any, are stored in colliding_pairs(). */
    void collide ( srbvmat R1, srbvvec T1, const SrBvTree* o1,
                   srbvmat R2, srbvvec T2, const SrBvTree* o2, CollideFlag flag );

    /*! Lower level call called by the other tolerance() method */
    bool tolerance ( srbvmat R1, srbvvec T1, SrBvTree* o1,
                     srbvmat R2, srbvvec T2, SrBvTree* o2, float toler )
          { _tolerance ( R1, T1, o1, R2, T2, o2, toler );
            _srp1.set ( _p1 );
            _srp2.set ( _p2 );
            return _closer_than_tolerance;
          }

    /*! Computes a simple but fast lower bound on the distance between two models */
    float greedy_distance ( srbvmat R1, srbvvec T1, SrBvTree* o1,
			                srbvmat R2, srbvvec T2, SrBvTree* o2, float delta );

   private :

    void _collide_recurse ( srbvmat R, srbvvec T,
                            const SrBvTree* o1, int b1, 
                            const SrBvTree* o2, int b2, int flag );

    void _distance ( srbvmat R1, srbvvec T1, SrBvTree* o1,
                     srbvmat R2, srbvvec T2, SrBvTree* o2,
                     srbvreal rel_err, srbvreal abs_err );

    void _distance_recurse ( srbvmat R, srbvvec T,
                             SrBvTree* o1, int b1,
                             SrBvTree* o2, int b2 );

    void _tolerance ( srbvmat R1, srbvvec T1, SrBvTree* o1,
                      srbvmat R2, srbvvec T2, SrBvTree* o2,
                      srbvreal tolerance );

    void _tolerance_recurse ( srbvmat R, srbvvec T,
                              SrBvTree* o1, int b1,
                              SrBvTree* o2, int b2 );

    srbvreal _greedy_distance_recurse ( srbvmat R, srbvvec T,
                                        SrBvTree* o1, int b1,
                                        SrBvTree* o2, int b2, srbvreal delta );
 };

//============================== end of file ===============================

# endif // SR_BV_TREE_QUERY_H
