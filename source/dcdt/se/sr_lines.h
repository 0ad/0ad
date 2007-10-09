
# ifndef SR_LINES_H
# define SR_LINES_H

/** \file sr_lines.h 
 * manages a set of lines
 */

# include "sr_vec.h"
# include "sr_box.h"
# include "sr_color.h"
# include "sr_array.h"
# include "sr_polygon.h"

/*! \class SrLines sr_lines.h
    \brief a set of lines

    Keeps the description of segments or polygonal lines.
    Vertices are stored in array V. If no extra information is given, V is supposed
    to contain a sequence of vertices forming independent line segments.
    In order to specify multiple polygonal lines and colors, the array of
    indices 'I' can be used in the following way:
    Each pair of indices (i,j) indicates that V[i] to V[j] form a polygonal
    line and not independent line segments. If j<0, then it means that the current
    color for all vertices with indices >=i will be C[-j-1].
    The indices i appearing in the sequence of pairs (i,j) must appear ordered, so that
    'I' is just read once when drawing the lines. */   
class SrLines
 { public :
    SrArray<SrPnt>   V; //<! Array of used vertices
    SrArray<SrColor> C; //<! Array of possibly used colors
    SrArray<int>     I; //<! Each pair indicates: polyline start,end or -colorid-1,startingid
    static const char* class_name;

   public :

    /* Default constructor. */
    SrLines ();

    /* Destructor . */
   ~SrLines ();

    /*! Set the sizes of arrays V, C, and I to zero. */
    void init ();

    /*! Returns true if V array is empty; false otherwise. */
    bool empty () const { return V.size()==0? true:false; }

    /*! Compress array V, C, and I. */
    void compress ();

    /*! Push in V the two points defining a new segment of line. */
    void push_line ( const SrVec &p1, const SrVec &p2 );
    void push_line ( const SrVec2 &p1, const SrVec2 &p2 );
    void push_line ( float ax, float ay, float az, float bx, float by, float bz );
    void push_line ( float ax, float ay, float bx, float by );

    /*! Starts a definition of a polyline by pushing in 'I' the index V.size() */
    void begin_polyline ();

    /*! Finishes a definition of a polyline by pushing in 'I' the index V.size()-1 */
    void end_polyline ();

    /*! Push in V a new vertex. */
    void push_vertex ( const SrVec& p );
    void push_vertex ( const SrVec2& p );
    void push_vertex ( float x, float y, float z=0 );

    /*! Push a new color to be considered for the new vertices that will be defined.
        This method pushes in I the pair ( V.size(), -C.size() ), and then
        pushes in C the given color c */
    void push_color ( const SrColor &c );

    /*! Create a 2 dimensional cross of radius r and center c */
    void push_cross ( SrVec2 c, float r );

    /*! Creates axis with desired parameters:
        orig gives the position of the axis center;
        len gives the length of all axis;
        dim can be 1, 2 or 3, indicating which axis (X,Y,or Z) will be drawn;
        string let must contain the letters to draw, e.g, "xyz" will draw all letters;
        rule (witch has default value of true) indicates wether to add marks in each unit;
        box, if !=0, will give precise min/max for each of the axis */
    void push_axis ( const SrPnt& orig, float len, int dim, const char* let,
                     bool rule=true, SrBox* box=0 );

    /*! Creates the 12 lines forming box b. If multicolor is true,
        colors red, green and blue are set to lines parallel to axis x, y, and z.
        Colors can be later on modified in the C array if required. */
    void push_box ( const SrBox& box, bool multicolor=false );

    /*! Push an array of points forming a polyline */
    void push_polyline ( const SrArray<SrVec2>& a );

    /*! Push an array of points forming a polygon */
    void push_polygon ( const SrArray<SrVec2>& a );

    /*! Push an array of points assumin each 2 consecutive points denote one line */
    void push_lines ( const SrArray<SrVec2>& a );

    /*! Calls push_polygon() or push_polyline() according to p.open() */
    void push_polygon ( const SrPolygon& p )
     { if (p.open()) push_polyline( (const SrArray<SrVec2>&)p );
         else push_polygon( (const SrArray<SrVec2>&)p );
     }

    /*! Approximates a circle with a polyline, where:
        center is the center point, center+radius gives the first point,
        normal is orthogonal to radius, and nvertices gives the number of
        vertices in the polyline */
    void push_circle_approximation ( const SrPnt& center, const SrVec& radius,
                                     const SrVec& normal, int nvertices );

    /*! Returns the bounding box of all vertices used. The returned box can be empty. */
    void get_bounding_box ( SrBox &b ) const;
 };


//================================ End of File =================================================

# endif  // SR_LINES_H

