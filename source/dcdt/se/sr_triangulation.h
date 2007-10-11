
# ifndef SR_TRIANGULATION
# define SR_TRIANGULATION

# include "sr_array.h"

class SrTravel;
class SrTriangulation;

//=========================== SrVtx ==================================

/*! A vertex of the triangulation */
class SrVtx
 { private :
    double _x, _y;
    int _id;
   private :
    friend class SrTriangulation;
    void set ( double x, double y, int id ) { _x=x; _y=y; _id=id; }
   public :
    double x() const { return _x; }
    double y() const { return _y; }
    int id() const { return _id; }
    bool border() const { return _id<4? true:false; }
 };

//=========================== SrTri ==================================

/*! A triangle of the triangulation */
class SrTri
 { private :
    SrVtx* _vtx[3];  // The three vertices of this triangle
    SrTri* _side[3]; // The three adjacent triangles of this triangle
    char   _mark[3]; // Used for marking traversal elements
    int    _id;
   private :
    friend class SrTravel;
    friend class SrTriangulation;
    void   set ( SrVtx* v0, SrVtx* v1, SrVtx* v2, SrTri* t0, SrTri* t1, SrTri* t2, int id );
    int    sideindex ( int i );

   public :
    int id() const { return _id; }
    bool back() const { return _id==0||_id==1? true:false; }
    const SrVtx* v0 () { return _vtx[0]; }
    const SrVtx* v1 () { return _vtx[1]; }
    const SrVtx* v2 () { return _vtx[2]; }
 };

//=========================== SrTravel ==================================

/*! The traversal element of the triangulation always references one
    triangle, one edge and one vertex. */
class SrTravel
 { private :
    SrTri* _t;  // the referenced triangle
    int    _v;  // the referenced vertex id of the triangle
    friend class SrTriangulation;

   public :
    SrTravel () : _t(0), _v(0) {}
    SrTravel ( SrTri* t, int v ) : _t(t), _v(v) {}
    SrTravel ( const SrTravel& tr ) : _t(tr._t), _v(tr._v) {}

    void set ( SrTri* t, int v ) { _t=t; _v=v; }

    bool border () { return _t->back()^_t->_side[_v]->back(); }     /*! Returns true if the travel is referencing an edge in the border of the triangulation (^is xor) */
    bool back () { return _t->back(); }    /*! Returns true if the travel is referencing one of the two triangles describing the 'backfaces' of the triangulation */

    SrVtx* vtx () const { return _t->_vtx[_v]; } // Returns the referenced vertex
    SrVtx* vtx ( unsigned n ) const { return _t->_vtx[(_v+n)%3]; } // referenced vertex after 'n' nxt()
    int    vid () const { return _t->_vtx[_v]->id(); } // Returns the id of the referenced vertex
    int   vnid () const { return _t->_vtx[(_v+1)%3]->id(); } // Returns the id of the next vertex referenced in the triangle (as if nxt() was applied)
    double   x () const { return _t->_vtx[_v]->x(); }     /*! Returns the x coordinate of the referenced vertex */
    double   y () const { return _t->_vtx[_v]->y(); }    /*! Returns the y coordinate of the referenced vertex */

    SrTri* tri () const { return _t; }    /*! Returns the referenced triangle */
    SrTri* adt () const { return _t->_side[_v]; }
    int    tid () const { return _t->id(); }    /*! Returns the id of the referenced triangle */

    SrTravel sym () const;
    SrTravel nxt () const { return SrTravel ( _t, (_v+1)%3 ); }
    SrTravel pri () const { return SrTravel ( _t, (_v+2)%3 ); }
    SrTravel rot () const { return pri().sym(); }
    SrTravel ret () const { return sym().nxt(); }

    void mark   () const { _t->_mark[_v]=1; }
    void unmark () const { _t->_mark[_v]=0; }
    bool marked () const { return _t->_mark[_v]? true:false; }
    
    /*! true if the circle passing trough the triangle's vertices does not contain sym().pri().vtx() */
    bool is_delaunay () const;

    /*! true if the 'outside polygon formed by the two adjacent triangles' is convex, allowing a flip */
    bool is_flippable () const;

    bool operator == ( const SrTravel& tr ) const { return _t==tr._t && _v==tr._v? true:false; }
    bool operator != ( const SrTravel& tr ) const { return _t==tr._t && _v==tr._v? false:true; }
    void operator = ( const SrTravel& tr ) { _t=tr._t; _v=tr._v; }
 };


//============================= SrTriangulation ===================================

/*! This class constructs and maintains a Delaunay triangulation of given 2D points. */
class SrTriangulation
 { private :
    SrArray<SrVtx*> _vtx;
    SrArray<SrTri*> _tri;
    SrArray<SrTravel> _stack; 
    
   public :
    SrTriangulation ();
   ~SrTriangulation ();
    
    const SrArray<SrVtx*>& vertices() const { return _vtx; }
    const SrArray<SrTri*>& triangles() const { return _tri; }
    
    /*! Returns a reference to a temporary internal array that will become unusable
        if any other method changing the triangulation is called.
        The back edge is not returned in the array. */
    const SrArray<SrTravel>& edges();

    /*! Initializes with the triangulation of the square with lower left corner (ax,ay),
        and upper right corner (bx,by). It creates 4 vertices and 4 triangles: the 2
        triangles in the back have the CW orientation, and the two triangles in the
        front have CCW orientation. All points inserted after init() will be inserted in
        the CCW triangles and must be inside this bounding square.
        The 4 vertices created are: 0:(ax,ay), 1:(bx,ay), 2:(bx,by), 3:(ax,by), and the
        border travel is specified as the travel referencing vertex 1, in the back 
        triangle <1,0,2> */
    void init ( double ax, double ay, double bx, double by );

    /*! The border traverse element in the back triangle 0, 'from vertex 1 to vertex 0'.
        This method can only be called after init(). */
    SrTravel border ();
    
    /*! Sets all markers to 0. Markers can be used from a travel. */
    void reset_markers ();

    /*! tri becomes a travel referencing the new vertex (x,y) */
    void addv ( SrTravel& tri, double x, double y );
    
    /*! edge becomes a travel referencing the 'rotated edge' */
    void flip ( SrTravel& edge );

    /*! recover delaunay criterion by applying flips to adjacent triangles of vtx.
        Travel vtx remains adjacent to the original vertex */
    void recover_delaunay ( SrTravel& vtx );

    enum LocateResult { NotFound, TriangleFound, VertexFound };

    /*! Returns in s the triangle containing (x,y); if false is returned (x,y) is 
        already there and the found duplicated vertex is returned in s.
        Optional parameter prec specifies precision for considering points equal */
    LocateResult locate_point ( double x, double y, SrTravel& s, double prec=1.0E-5 );

    /*! Insert (x,y) if not duplicated, and returns the new or existing vertex containing (x,y).
        Optional parameter prec specifies precision for considering points equal.
        Check the returned vertex id and vertices array for knowing if a new vertex was inserted.
        false is returned in case (x,y) is outside the triangulation */
    bool insert_point ( double x, double y, SrTravel& t, double prec=1.0E-5 );

    /*! Output list of vertices and triangles for data inspection */
    void output ( SrOutput& out );
 };

//============================= EOF ===================================

# endif // SR_TRIANGULATION
