/** \file sr_bv_tree.h 
 * bounding volume tree */

# ifndef SR_BV_TREE_H
# define SR_BV_TREE_H

# include "sr_array.h"
# include "sr_bv_math.h" 
# include "sr_class_manager.h" 

class SrModel;
class SrBvTreeQuery;

/*! Encapsulates the coordinates of a triangle. */
struct SrBvTri
 { srbvvec p1, p2, p3; // the three coordinates
   int id;             // the id of the face of the original SrModel
 };

/*! Encapsulates a bounding volume node.
    Adapted from PQP, see the copyright notice in the source file. */
struct SrBv
 { srbvmat  R;      // orientation of RSS & OBB
   srbvvec  Tr;     // RSS: position of rectangle
   srbvreal l[2];   // RSS: side lengths of rectangle
   srbvreal r;      // RSS: radius of sphere summed with rectangle to form RSS
   srbvvec  To;     // OBB: position of obb
   srbvvec  d;      // OBB: (half) dimensions of obb
   int first_child; // positive value is index of first_child bv
                    // negative value is -(index + 1) of triangle
   SrBv() { first_child=0; }
   bool leaf() const { return first_child<0? true:false; }
   srbvreal size() const;
   void fit ( srbvmat O, SrBvTri* tris, int num_tris );
   static bool overlap ( srbvmat R, srbvvec T, const SrBv* b1, const SrBv* b2 );
   static srbvreal distance ( srbvmat R, srbvvec T, const SrBv* b1, const SrBv* b2 );
 };

/*! \class SrBvTree sr_bv_tree.h
    \brief bounding volume tree 
    Manages a hierarchy of bounding volumes, both OBB and RSS are
    maintained. OBBs are used for collision detection and RSSs are
    used for distance computation.
    Adapted from PQP, see the copyright notice in the source file. */
class SrBvTree
 { private:
    SrArray<SrBvTri> _tris;
    SrArray<SrBv> _bvs;
    SrBvTri* _last_tri; // closest tri on this model in last distance test
    friend class SrBvTreeQuery;

   public :
    /*! Constructor */
    SrBvTree ();

    /*! Destructor */
   ~SrBvTree ();

    /*! Returns the number of bounding volumes in the current tree */
    int nodes () const { return _bvs.size(); }

    /*! Returns the triangle with index i */
    const SrBvTri* tri ( int i ) const { return &_tris[i]; }

    /*! Returns a const copy of the list of triangles */
    const SrArray<SrBvTri>& tris () const { return _tris; }

    /*! Returns the bounding volume node with index i */
    SrBv* child ( int i ) { return &_bvs[i]; }
    const SrBv* const_child ( int i ) const { return &_bvs[i]; }

    /*! Empty the tree and frees all used memory */
    void init ();

    /*! Constructs the tree for the given model */
    void make ( const SrModel& m );

   private :
    void _build_recurse ( int bn, int first_tri, int num_tris );
    void _make_parent_relative ( int bn, const srbvmat parentR,
                                 const srbvvec parentTr,
                                 const srbvvec parentTo );
 };

//============================== end of file ===============================

# endif // SR_BV_TREE_H
