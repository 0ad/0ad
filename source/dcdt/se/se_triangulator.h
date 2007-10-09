
# ifndef SE_TRIANGULATOR_H
# define SE_TRIANGULATOR_H

/** \file se_triangulator.h 
 * A triangulator based on symedges
 */

# include "sr_polygon.h"
# include "se_mesh.h"

/*! SeTriangulatorManager contains virtual methods that the user needs to provide
    to the triangulator, allowing the triangulator to:
    1.access the coordinates of a vertex, which is a user-denided type.
    2.notify the user of some events: vertices, edges created, etc.
    3.access the constrained information of edges. This information should be
      stored as an array with the constraints ids sharing the constrained edge.
      However, if constraints are guaranteed to not overllap, the user can
      simply maintain a boolean variable.
    Other utility methods are available in the class.
    Note that the required virtual methods to be implemented are only those 
    related to the triangulator functions being used.
 */
class SeTriangulatorManager : public SrSharedClass
 { public :

    /*! Allows retrieving the coordinates of a vertex. This method is pure 
        virtual, so it needs to be implemented in all cases. */
    virtual void get_vertex_coordinates ( const SeVertex* v, double& x, double& y )=0;

    /*! Allows setting the coordinates of a vertex. This method is pure 
        virtual, so it needs to be implemented in all cases. */
    virtual void set_vertex_coordinates ( SeVertex* v, double x, double y )=0;

    /*! This method is called by SeTriangulator::triangulate_face() to notify
        new edges created during the triangulation process, before the optimization
        phase, which is based on edge swap. The default implementation simply does nothing.*/
    virtual void triangulate_face_created_edge ( SeEdge* e );

    /*! This method is called by the triangulator to notify when vertices which
        are intersection of two constrained edges are inserted in the triangulation.
        The default implementation simply does nothing. */
    virtual void new_intersection_vertex_created ( SeVertex* v );

    /*! This method is called by the triangulator to notify when steiner vertices are
        inserted when recovering conforming line constraints. The default implementation
        simply does nothing. */
    virtual void new_steiner_vertex_created ( SeVertex* v );

    /*! This method is called to track vertices which are found when inserting
        edge constraints with insert_line_constraint(). Meaning that the found
        vertices v which are in the segment v1, v2 are passed here. The default
        implementation simply does nothing. */
    virtual void vertex_found_in_constrained_edge ( SeVertex* v );

    /*! Should return true if the passed edge is constrained. The default
        implementation always returns false. */
    virtual bool is_constrained ( SeEdge* e );

    /*! Requires that the edge becomes unconstrained. This is called during 
        manipulation of the triangulation, when edges need to be temporary
        unconstrained for consistent edge flipping. The default implementation
        does nothing. */
    virtual void set_unconstrained ( SeEdge* e );

    /*! Used to see if an edge is constrained. The passed array of ids is empty.
        If it is returned as empty, it means that the edge is not constrained;
        and this is the case of the default implementation. */
    virtual void get_constraints ( SeEdge* e, SrArray<int>& ids );

    /*! This method is called when the edge needs to be set as constrained.
        The passed array contains the ids to be added to the edge. So that
        the user can keep track of many constraints sharing the same constrained
        edge. The passed array will never be empty. The default implementation 
        does nothing. */
    virtual void add_constraints ( SeEdge* e, const SrArray<int>& ids );

    /*! Retrieves the coordinates and call sr_ccw() */
    bool ccw ( SeVertex* v1, SeVertex* v2, SeVertex* v3 );

    /*! Retrieves the coordinates and call sr_in_triangle() */
    bool in_triangle ( SeVertex* v1, SeVertex* v2, SeVertex* v3, SeVertex* v );

    /*! Retrieves the coordinates and call sr_in_triangle() */
    bool in_triangle ( SeVertex* v1, SeVertex* v2, SeVertex* v3, double x, double y );

    /*! Retrieves the coordinates and call sr_in_segment() */
    bool in_segment ( SeVertex* v1, SeVertex* v2, SeVertex* v, double eps );

    /*! Retrieves the coordinates and call sr_in_circle() */
    bool is_delaunay ( SeEdge* e );

    /*! Uses sr_ccw() and sr_in_circle() */
    bool is_flippable_and_not_delaunay ( SeEdge* e );

    /*! Check if (x,y) lies in the interior, edge, or vertex of (v1,v2,v3).
        A SeTriangulator::LocateResult enum is returned, and s will be adjacent to
        the found element, if any (sr_next() and sr_in_segment() are used). */
    int test_boundary ( SeVertex* v1, SeVertex* v2, SeVertex* v3,
                        double x, double y, double eps, SeBase*& s );

    /*! Retrieves the coordinates and call sr_segments_intersect() */
    bool segments_intersect ( SeVertex* v1, SeVertex* v2, 
                              SeVertex* v3, SeVertex* v4, double& x, double& y );

    /*! Retrieves the coordinates and call sr_segments_intersect() */
    bool segments_intersect ( SeVertex* v1, SeVertex* v2, SeVertex* v3, SeVertex* v4 );

    /*! Retrieves the coordinates and call sr_segments_intersect() */
    bool segments_intersect ( double x1, double y1, double x2, double y2,
                          SeVertex* v3, SeVertex* v4 );

    /*! Returns the midpoint of segment (v1,v2) */
    void segment_midpoint ( SeVertex* v1, SeVertex* v2, double& x, double& y );
 };

/*! \class SeTriangulator sr_triangulator.h
    \brief Delaunay triangulation methods

    SeTriangulator provides several methods to construct and update
    triangulations, including Delaunay triangulations with support 
    to constrained and conforming versions.
    To use it, a SeTriangulatorManager class is required in order
    to tell how the triangulator can access the data in the user mesh.
    The triangulator methods were made to be of flexible use for different
    applications, so that they only perform the basic algorithms work,
    leaving many decisions to the user. In this way it is left as user
    responsability to correctly use them. For instance, the associated mesh
    must be in fact a triangulation with counter-clockwise triangles, etc,
    otherwise several algorithms will fail.
    Most of the used techniques are described in the following paper:
       M. Kallmann, H. Bieri, and D. Thalmann, "Fully Dynamic Constrained
       Delaunay Triangulations", In Geometric Modelling for Scientific 
       Visualization, G. Brunnett, B. Hamann, H. Mueller, L. Linsen (Eds.),
       ISBN 3-540-40116-4, Springer-Verlag, Heidelberg, Germany, pp. 241-257, 2003.
    However, some solutions presented in the paper may not be available in the current
    version of the code, which favoured to maintain simpler approaches.
    All used geometric primitives are implemented in sr_geo2.cpp */
class SeTriangulator
 { public :

    /*! Note: ModeConforming is not robust when several constraints intersect */
    enum Mode { ModeUnconstrained, ModeConforming, ModeConstrained };

    /*! Used by locate_point() method */
    enum LocateResult { NotFound, TriangleFound, EdgeFound, VertexFound };

   private :
    Mode _mode;
    double _epsilon;
    SeMeshBase* _mesh;
    SeTriangulatorManager* _man;
    SrArray<SeBase*> _buffer;
    SrArray<int> _ibuffer;
    struct ConstrElem { SeVertex* v; SeBase* e;
                        void set(SeVertex* a, SeBase* b) {v=a; e=b;}
                      };
    SrArray<ConstrElem> _elem_buffer;
    class PathNode;
    class PathTree;
    PathTree* _ptree;
    bool _path_found;
    SrArray<SeBase*> _channel;
    double _xi, _yi, _xg, _yg;
    class FunnelDeque;
    FunnelDeque* _fdeque;

   public :

    /*! Three modes of triangulations can be created: unconstrained, conforming,
        or constrained. Depending on the mode some internal methods will
        behave differently. In particular, for unconstrained triangulations
        it is not required to reimplement the methods of SeTriangulatorManager
        dealing with is/set/get/add edge constraints.
        The documentation of each method should tell which methods are used from
        both the associated manager and geometric primitives.
        The required manager should be allocated with operator new and can be shared.
        The epsilon is used in the geometric primitives. */ 
    SeTriangulator ( Mode mode, SeMeshBase* m, SeTriangulatorManager* man, double epsilon );

    /*! The destructor unreferences the associated SeTriangulatorManager,
        but it does not delete the associated mesh. */
   ~SeTriangulator ();

    /*! Set a new epsilon */
    void epsilon ( double eps ) { _epsilon=eps; }

    /*! Get the currently used epsilon */
    double epsilon () const { return _epsilon; }

    /*! Returns the associated mesh pointer */
    SeMeshBase* mesh () const { return _mesh; }

    /*! Returns the associated manager pointer */
    SeTriangulatorManager* manager () const { return _man; }

    /*! Set the desired mode */
    void mode ( Mode m ) { _mode=m; }

    /*! Get the current triangulator mode */
    Mode mode () const { return _mode; }

   public :

    /*! This method destroys the associated mesh and initializes it as
        a triangulated square with the given coordinates. Points must
        be passed in counter clockwise order. The returned SeBase
        is adjacent to vertex 1, edge (1,2) and triangle (1,2,4).
        To construct a square with given maximum and minimum coordinates,
        points should be set as follows (respectivelly):
        xmin, ymin, xmax, ymin, xmax, ymax, xmin, ymax.
        Alternatively, the user may initialize the associated mesh directly
        by calling the construction operators of SeMesh (such as mev and mef).
        Normally the mesh should be initialized as a triangulated convex polygon. */
    SeBase* init_as_triangulated_square 
                               ( double x1, double y1,
                                 double x2, double y2,
                                 double x3, double y3,
                                 double x4, double y4 );

    /*! Triangulates a counter-clockwise (ccw) oriented face. It is the
        user responsability to provide a face in the correct ccw orientation.
        False is returned in case the face cannot be triangulated. However,
        this migth only happen if the face is not simple.
        It uses the so called "ear" algorithm, that has worst case complexity
        of O(n^2), but has a very simple implementation.
        It uses the geometric primitives sr_ccw() and sr_in_triangle() and
        each time an edge is inserted during the triangulation construction,
        the method new_edge_created() of the associated manager is called.
        If optimization is set to true (the default), the algorithm will
        flip the created edges to satisfy the Delaunay criterium, and for
        this, the geometric primitive sr_in_circle() is also used. */
    bool triangulate_face ( SeFace* f, bool optimize=true );

    /*! This method searches for the location containing the given point.
        The enumerator LocateResult is returned with the result of the
        query. If the point is coincident to an existing vertex, VertexFound
        is returned, and result is incident to the vertex.
        If the point is found on an edge, EdgeFound is returned, and result
        is incident to the edge. If the point is inside a triangle, result
        is incident to the found triangle. In the case that the point is
        not found to be inside the triangulation, NotFound is returned.
        The algorithm starts with the given iniface, and continously
        skips to the neighbour triangle which shares an edge separating the
        current triangle and the point to reach in two different semi spaces.
        To avoid possible loops, triangle marking is used.
        The "distance" of the given iniface and the point to search 
        dictates the performance of the algorithm. The agorithm is O(n)
        (n = number of triangles). In general the pratical performance
        is much better than O(n). However serious loop problems will occur
        if topological inconsistencies are detected.
        This algorithm may fail to find the point if the border of the 
        triangulation is not convex or if non triangular faces exist.
        It is mainly based on the geometric primitive sr_ccw(). But it also
        calls methods sr_points_are_equal() and sr_in_segment(), in order to
        decide if the point is in an existing edge or vertex. */
    LocateResult locate_point ( const SeFace* iniface,
                                double x, double y, SeBase*& result );

    /*! Insert a point in the given triangle, with the given coordinates,
        and then continously flips its edges to ensure the Delaunay criterium.
        The geometric primitive sr_in_circle() is called during this process.
        The new vertex inserted is returned and will be never 0 (null).
        Methods new_vertex_created() and new_edge_created() of the associated
        manager are called for the new three edges and one vertex created.
        If the triangulator is of type conforming, methods for managing the
        constraints ids of edges are called, and to maintain the correctness
        of the triangulation, new vertices might be automatically inserted
        in a recursive mid-point subdivision way.
        If the triangulator is of constrained type, points are inserted only
        at eventual intersection points. */
    SeVertex* insert_point_in_face ( SeFace* f, double x, double y );

    /*! Insert a point in the given edge, with the given coordinates.
        It might be a good practice to project the point to the segment
        defined by the edge before calling this method.
        After insertion, it continously flips edges to ensure the Delaunay
        criterium, as in insert_point_in_face */
    SeVertex* insert_point_in_edge ( SeEdge* e, double x, double y );

    /*! Insert point searchs the location of (x,y) and correclty insert it in case it
        is located in an edge or face. If a coincident vertex is found it is returned.
        If iniface is null, the search starts from mesh()->first()->fac().
        Null is returned in case the point cannot be located. */
    SeVertex* insert_point ( double x, double y, const SeFace* inifac=0 );

    /*! Removes a vertex from the Delaunay triangulation. It is the user responsibility
        to guarantee that the vertex being removed is inside a triangulation, and not
        at the border of a face which is not triangular.
        This method simply calls SeMesh::delv() and then retriangulates the created
        non-triangular polygon, so that no special actions are taken concerning
        constrained edges. */
    bool remove_vertex ( SeVertex* v );

    /*! Inserts a line constraint to the current Delaunay triangulation. The line 
        is defined by two existing vertices. This method has a different behavior
        depending on the actual mode of the triangulator:
        If the triangulator is in unconstrained mode, nothing is done.
        If the mode is conforming, Steiner points are inserted starting with the
        middle point of the missing constraint, and recursively inserted by binary 
        partition until the line becomes present in the triangulation.
        If the triangulator is in constrained mode, Steiner points are only inserted
        at the intersection of existing constraints, if there are any. Hence,
        constrained edges do not respect the Delaunay criterium.
        False is returned if the algorithm fails, what can occur if non ccw or non
        triangular cells are found, or in unconstrained mode.
        The id parameter allows the user to keep track of the created constraints,
        and it is passed to the corresponding manager method.
        Note that edges may be referenced by several
        constrained lines in the case overllap occurs, and thats why each edge
        of the triangulation should maintain an array of constraints ids.
        Methods new_vertex_created() and vertex_found_in_constrained_edge() of
        the manager are called to allow tracking the insertion of Steiner points. */
    bool insert_line_constraint ( SeVertex *v1, SeVertex *v2, int id );

    /*! Inserts the two points with insert_point() and then call insert_line_constraint().
        Returns the success or failure of the operation */
    bool insert_segment ( double x1, double y1, double x2, double y2, int id, const SeFace* inifac=0 );

    /*! Search for a sequence of triangles (e.g. a channel) connecting x1,y1 and x2,y2,
        without crossing edges marked as constrained.
        If true is returned, the channel and paths inside the channel can be retrieved
        with methods get_channel(), get_canonical_path(), get_shortest_path().
        Note that the channel may not be the shortest one available.
        Parameter iniface is required in order to feed the process of finding
        the triangle containing the initial point p1. If it is already the triangle
        containing p1, the search will be solved trivially.
        If vistest is true (the default if false) a direct line test is performed prior
        to the path search.
        The A* algorithm is used, with the simple heuristic "dist(cur_node,goal_node)" */
    bool search_path ( double x1, double y1, double x2, double y2,
                       const SeFace* iniface, bool vistest=false );

    /*! Returns a reference to the list with the interior edges of the last channel
        determined by a sussesfull call to search_path */
    const SrArray<SeBase*>& get_channel_interior_edges () const { return _channel; }

    /*! Returns a polygon describing the current channel, and thus, method find_path must
        be succesfully called before to determine the channel to consider. */
    void get_channel_boundary ( SrPolygon& channel );

    /*! Returns the canonical path, which is the path passing through the midpoint of
        the channel interior edges. Method search_path must be succesfully called before
        in order to determine the channel. The path is returned as an open polygon. */
    void get_canonical_path ( SrPolygon& path );

    /*! Returns the shortest path inside the current channel using the funnel algorithm. */
    void get_shortest_path ( SrPolygon& path );

   private :
    void _propagate_delaunay ();
    bool _conform_line  ( SeVertex*, SeVertex*, const SrArray<int>& );
    bool _constrain_line ( SeVertex*, SeVertex*, const SrArray<int>& );
    void _v_next_step ( SeBase* s, SeVertex* v1, SeVertex* v2, SeVertex*& v, SeBase*& e );
    void _e_next_step ( SeBase* s, SeVertex* v1, SeVertex* v2, SeVertex*& v, SeBase*& e );
    bool _can_connect ( SeBase* se, SeBase* sv );
    bool _blocked ( SeBase* s );
    void _ptree_init ( LocateResult res, SeBase* s, double xi, double yi, double xg, double yg );
    int  _expand_lowest_cost_leaf ();
    void _funnel_add ( bool intop, SrPolygon& path, const SrPnt2& p );
 };

//============================ End of File =================================

# endif // SE_TRIANGULATOR_H

