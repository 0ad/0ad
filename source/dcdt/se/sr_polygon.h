
# ifndef SR_POLYGON_H
# define SR_POLYGON_H

/** \file sr_polygon.h
 * A polygon described by an array of 2d points
 */

# include "sr_vec2.h"
# include "sr_array.h"

class SrBox;

/*! \class SrPolygon sr_polygon.h
    \brief Array of 2d points

    SrPolygon derives SrArray<SrPnt2> and provides methods for operations with
    polygons. */
class SrPolygon : public SrArray<SrPnt2>
 { private :
    char _open;

   public :
    static const char* class_name;

    /*! Constructor with a given size and capacity, whith default values of 0 */
    SrPolygon ( int s=0, int c=0 );

    /*! Copy constructor */
    SrPolygon ( const SrPolygon& p );

    /*! Constructor from a given buffer */
    SrPolygon ( SrPnt2* pt, int s, int c );

    /*! An open polygon is equivalent to a polygonal line */
    void open ( bool b ) { _open=b; }

    /*! Returns true if the polygon is open, and false otherwise */
    bool open () const { return _open? true:false; }
  
    /*! Sets the polygon from a float list. numv is the number of vertices, each
        vertex being represented by two floats. */
    void set_from_float_array ( const float* pt, int numv );

    /*! Returns true if the polygon is simple by testing if it is degenerated
       or if non adjacent edges intersect */
    bool is_simple () const;

    /*! Returns true if the polygon is convex */
    bool is_convex () const;

    /*! Returns true if the polygon has orientation counter-clockwise */
    bool is_ccw () const { return area()>0? true:false; }

    /*! Returns the oriented area, it will be >0 if the polygon is ccw */
    float area () const;

    /*! Point in polygon test */
    bool contains ( const SrPnt2& p ) const;

    /*! Check if all points of pol are inside the polygon */
    bool contains ( const SrPolygon& pol ) const;

    /*! Check if p is in the boundary of the polygon according to the precision ds.
        the function segment_contains_point() is used for each polygon edge. If p is not
        in the boundary, -1 is returned, othersise the returned index says which edge of
        the polygon contains the point. */
    int has_in_boundary ( const SrPnt2& p, float ds ) const;

    /*! Makes the polygon be a circle approximation with given center, radius, and 
        number of vertices. */
    void circle_approximation ( const SrPnt2& center, float radius, int nvertices );

    /*! Calculates the total length along edges */
    float perimeter () const;

    /*! Parameter t must be between 0 and perimeter */
    SrPnt2 interpolate_along_edges ( float t ) const;

    /*! Resample the polygon by subdividing edges to ensure that each edge has at 
        most maxlen as length */
    void resample ( float maxlen );

    /*! Remove adjacent vertices which are duplicated, according to epsilon */
    void remove_duplicated_vertices ( float epsilon );

    /*! Remove adjacent vertices which are collinear, according to epsilon */
    void remove_collinear_vertices ( float epsilon );

    /*! Grows the polygon by radius. CCW ordering is required. If radius<0,
        the obstacle will be contracted. Resulting arc circles are discretized
        by steps of maxangdeg */
    void grow ( float radius, float maxangrad );

    /*! Get centroid of polygon */
    SrPnt2 centroid () const;

    /*! Reverse the order of the elements */
    void reverse ();

    /*! Adds dv to each vertex of the polygon */
    void translate ( const SrVec2& dv );

    /*! Rotates the polygon around center about angle radians */
    void rotate ( const SrPnt2& center, float radians );

    /*! Returns the lowest vertex of the polygon. If a non-null int pointer
        is passed, it will contain the index of the found south pole */
    SrPnt2 south_pole ( int* index=0 ) const;

    /*! Returns the convex hull in pol */
    void convex_hull ( SrPolygon& pol ) const;

    /*! Returns the index of the vertex which is closer to p, and within
        an epsilon distance of p, or -1 if there is not such a vertex */
    int pick_vertex ( const SrPnt2& p, float epsilon );

    /*! Returns the index i of the edge (i,i+1) that is the closer edge
        to p, and within a distance of epsilon to p. The square of that
        distance is returned in dist2. -1 is returned if no edges are found */
    int pick_edge ( const SrPnt2& p, float epsilon, float& dist2 ) const;

    /*! Divides the polygon in triangles */
    void ear_triangulation ( SrArray<SrPnt2>& tris ) const;

    /*! Returns the bounding gox of the polygon */
    void get_bounding_box ( SrBox& b ) const;

    /*! Get the polygon configuration. x,y is the centroid, and a is the angle
        between the centroid and the vertex 0, in the range [0,360) */
    void get_configuration ( float& x, float& y, float& a ) const;

    /*! Puts the polygon in the configuration (x,y,a): x,y is the centroid, and
        a is the angle between the centroid and the vertex 0, in the range [0,360) */
    void set_configuration ( float x, float y, float a );

    /*! Tests if the polygon intersects with the given segment. */
    bool intersects ( const SrPnt2& p1, const SrPnt2& p2 ) const;

    /*! Test if the two polygons intersect */
    bool intersects ( const SrPolygon& p ) const;

    /*! Comparison function not implemented, just returns 1 */
    friend int sr_compare ( const SrPolygon* p1, const SrPolygon* p2 );

    /*! Outputs the open flag and calls the SrArray::method */
    friend SrOutput& operator<< ( SrOutput& out, const SrPolygon& p );

    /*! Check for the open flag and calls the SrArray::method */
    friend SrInput& operator>> ( SrInput& inp, SrPolygon& p );
 };

//================================ End of File =================================================

# endif // SR_POLYGON_H

