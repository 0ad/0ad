
# ifndef SR_POLYGONS_H
# define SR_POLYGONS_H

/** \file sr_polygons.h
 * maintains an array of polygons
 */

# include "sr_array.h"
# include "sr_polygon.h"
# include "sr_shared_class.h"

/*! \class SrPolygons sr_polygons.h
    \brief maintains an array of polygons

    SrPolygons keeps internally an array of polygons and provides
    methods for manipulating them. */   
class SrPolygons : public SrSharedClass
 { private :
    SrArray<SrPolygon*> _data;

   public :
    static const char* class_name;

    /*! Default constructor */
    SrPolygons ();

    /*! Copy constructor */
    SrPolygons ( const SrPolygons& p );

    /*! Virtual Destructor */
    virtual ~SrPolygons ();

    /*! Returns true if the array has no polygons, and false otherwise. */
    bool empty () const { return _data.empty(); }

    /*! Returns the capacity of the array. */
    int capacity () const { return _data.capacity(); }

    /*! Returns the current size of the array. */
    int size () const { return _data.size(); }

    /*! Changes the size of the array, filling new empty polygons in the new positions. */
    void size ( int ns );

    /*! Makes the array empty; equivalent to size(0) */
    void init () { size(0); }

    /*! Changes the capacity of the array. */
    void capacity ( int nc );

    /*! Makes capacity to be equal to size. */
    void compress () { _data.compress(); }

    /*! Swaps polygons with positions i and j, which must be valid positions. */
    void swap ( int i, int j );

    /*! Returns a reference to polygon index i, which must be a valid index */
    SrPolygon& get ( int i ) { return *_data[i]; }

    /*! Returns a const reference to polygon index i, which must be a valid index */
    const SrPolygon& const_get ( int i ) const { return *_data[i]; }

    /*! Returns a const reference to the vertex j of polygon i. Indices must be valid. */
    const SrVec2& const_get ( int i, int j ) const { return _data[i]->const_get(j); }

    /*! Returns a reference to the vertex j of polygon i. Indices must be valid. */
    SrVec2& get ( int i, int j ) { return _data[i]->get(j); }

    /*! Equivalent to get(i) */
    SrPolygon& operator[] ( int i ) { return get(i); }

    /*! Equivalent to get(i,j) */
    SrVec2& operator() ( int i, int j ) { return get(i,j); }

    /*! Copy polygon p into position i */
    void set ( int i, const SrPolygon& p ) { get(i)=p; }

    /*! Returns the last polygon */
    SrPolygon& top () { return *_data.top(); }

    /*! Pop and frees last polygon if the array is not empty */
    void pop () { delete _data.pop(); }

    /*! Allocates and appends one empty polygon */
    SrPolygon& push () { return *(_data.push()=new SrPolygon); }

    /*! Inserts one polygon using copy operator */
    void insert ( int i, const SrPolygon& x );

    /*! Allocates and inserts n empty polygons at position i */
    void insert ( int i, int n );

    /*! Removes n polygons at position i */
    void remove ( int i, int n=1 );

    /*! Extract (without deletion) and returns the pointer at position i */
    SrPolygon* extract ( int i );

    /*! Returns true if there is a vertex closer to p than epsilon. In this case
        the indices of the closest vertex to p are returned in pid and vid.
        If no vertices exist, false is returned. */
    bool pick_vertex ( const SrVec2& p, float epsilon, int& pid, int& vid ) const;

    /*! Returns the index of the first polygon containing p, or -1 if not found */
    int pick_polygon ( const SrVec2& p ) const;

    /*! Returns true if there is an edge closer to p than epsilon. In this case
        the indices of the first vertex of the closest edge are returned in pid and vid.
        If no edge exist, false is returned. */
    bool pick_edge ( const SrVec2& p, float epsilon, int& pid, int& vid ) const;

    /*! Returns the first polygon intersecting the given segment, or -1 otherwise. */
    int intersects ( const SrVec2& p1, const SrVec2& p2 ) const;

    /*! Returns the first polygon intersecting p, or -1 otherwise. */
    int intersects ( const SrPolygon& p ) const;

    /*! Returns the bounding box of the set of polygons */
    void get_bounding_box ( SrBox &b ) const;

    /*! Copy operator */
    void operator = ( const SrPolygons& p );

    /*! Outputs all elements of the array. */
    friend SrOutput& operator<< ( SrOutput& o, const SrPolygons& p );

    /*! Inputs elements. */
    friend SrInput& operator>> ( SrInput& in, SrPolygons& p );
 };

//================================ End of File =================================================

# endif // SR_SCENE_POLYGONS_H
