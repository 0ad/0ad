
# ifndef SE_DCDT_H
# define SE_DCDT_H

/** \file se_dcdt.h
 * Dynamic Constrained Delaunay Triangulaiton
 */

# include "sr_set.h"
# include "sr_vec2.h"
# include "sr_polygon.h"

# include "se_triangulator.h"

//DJD: Including definition of abstraction and search node classes {

//#define EXPERIMENT
#define DEBUG
#define FIRST_PATH_FAST

#include "Abstraction.h"
#include "SearchNode.h"
#if defined EXPERIMENT
#include "Experiments.h"
#endif

//Including definition of abstraction and search node classes }

//============================ DCDT Mesh definitions ==================================

class SeDcdtVertex; // contains 2d coordinates
class SeDcdtEdge;   // contains constraints ids

//DJD: changing definition of SeDcdtFace {

//class SeDcdtFace;   // a default element

//changing definition of SeDcdtFace }

typedef Se < SeDcdtVertex, SeDcdtEdge, SeDcdtFace > SeDcdtSymEdge;
typedef SeMesh < SeDcdtVertex, SeDcdtEdge, SeDcdtFace > SeDcdtMesh;

//DJD: defining SeLinkFace {

//SE_DEFINE_DEFAULT_ELEMENT(SeDcdtFace,SeDcdtSymEdge);

template <class T>
class SeLinkFace : public SeElement
{
public:
	//the number of new faces that haven't been dealt with
	static int Faces() {return processing.size();}
	//retrieve a particular unprocessed face
	static SeLinkFace<T> *Face(int n) {return ((n < 0) || (n >= processing.size())) ? NULL : processing[n];}
	//clear unprocessed face list, increment the global update
	static void Clear() {while (!processing.empty()) processing.pop(); current++;}
	//link to external information
	T *link;
	SE_ELEMENT_CASTED_METHODS(SeLinkFace, SeDcdtSymEdge);
	//default constructor - set the link to null and initialize the face
	SeLinkFace() : link (NULL) {Initialize();}
	//copy constructor - initialize the link to that of the other face, and initialize it
	SeLinkFace(const SeLinkFace& f) : link (f.link) {Initialize();}
	//constructor - initialize the link to the value passed and initialize the face
	SeLinkFace(T *l) : link (l) {Initialize();}
	//destructor - deletes a non-null link, and nulls its entry in the unprocessed list if it's in there
	~SeLinkFace() {if (link != NULL) delete link; if (update == current) processing[index] = NULL;}
	friend SrOutput& operator<<(SrOutput& out, const SeLinkFace<T>& f) {return out;}
	friend SrInput& operator>>(SrInput& inp, SeLinkFace<T>& f) {return inp;}
	friend int sr_compare(const SeLinkFace<T>* f1, const SeLinkFace<T>* f2) {return 0;}
protected:
	//the index of this face in the unprocessed list
	int index;
	//the update when this face was inserted in the unprocessed list
	int update;
	//initializes the face - puts it in the unprocessed list and sets its index and update
	void Initialize() {index = processing.size(); processing.push() = this; update = current;}
	//the unprocessed list
	static SrArray<SeLinkFace<T> *> processing;
	//the global update
	static int current;
};

//defining SeLinkFace }

class SeDcdtVertex : public SeElement
 { public :
    SrPnt2 p; // 2d coordinates
   public :
    SE_ELEMENT_CASTED_METHODS(SeDcdtVertex,SeDcdtSymEdge);
    SeDcdtVertex () {}
    SeDcdtVertex ( const SeDcdtVertex& v ) : p(v.p) {}
    SeDcdtVertex ( const SrPnt2& pnt ) : p(pnt) {}
    void set ( float x, float y ) { p.x=x; p.y=y; }
    void get_references ( SrArray<int>& ids ); // get all constr edges referencing this vertex
    friend SrOutput& operator<< ( SrOutput& out, const SeDcdtVertex& v );
    friend SrInput& operator>> ( SrInput& inp, SeDcdtVertex& v );
    friend int sr_compare ( const SeDcdtVertex* v1, const SeDcdtVertex* v2 ) { return 0; } // not used
 };

class SeDcdtEdge : public SeElement
 { public :
    SrArray<int> ids; // ids of all constraints sharing this edge
   public :
    SE_ELEMENT_CASTED_METHODS(SeDcdtEdge,SeDcdtSymEdge);
    SeDcdtEdge () { }
    SeDcdtEdge ( const SeDcdtEdge& e ) { ids=e.ids; }
    bool is_constrained() const { return ids.size()>0? true:false; }
    void set_unconstrained () { ids.size(0); }
    bool has_id ( int id ) const;
    bool has_other_id_than ( int id ) const;
    bool remove_id ( int id );
    void add_constraints ( const SrArray<int>& ids );
    friend SrOutput& operator<< ( SrOutput& out, const SeDcdtEdge& e );
    friend SrInput& operator>> ( SrInput& inp, SeDcdtEdge& e );
    friend int sr_compare ( const SeDcdtEdge* e1, const SeDcdtEdge* e2 ) { return 0; } // not used
 };

//================================ DCDT internal classes =====================================

class SeDcdtTriManager: public SeTriangulatorManager
 { public :
    virtual void get_vertex_coordinates ( const SeVertex* v, double& x, double & y );
    virtual void set_vertex_coordinates ( SeVertex* v, double x, double y );
    virtual bool is_constrained ( SeEdge* e );
    virtual void set_unconstrained ( SeEdge* e );
    virtual void get_constraints ( SeEdge* e, SrArray<int>& ids );
    virtual void add_constraints ( SeEdge* e, const SrArray<int>& ids );
 };

class SeDcdtInsPol: public SrArray<SeDcdtVertex*> // keeps one "inserted polygon"
 { public :
    char open;
   public :
    SeDcdtInsPol () { open=0; }
    SeDcdtInsPol ( const SeDcdtInsPol& ob ) : SrArray<SeDcdtVertex*>((const SrArray<SeDcdtVertex*>&)ob) { open=0; }
    friend SrOutput& operator<< ( SrOutput& o, const SeDcdtInsPol& ob );
    friend SrInput& operator>> ( SrInput& in, SeDcdtInsPol& ob );
    friend int sr_compare ( const SeDcdtInsPol* c1, const SeDcdtInsPol* c2 ) { return 0; } // not used
 };

//=================================== DCDT class ========================================

/*! Mantains a dynamic constrained Delaunay triangulation of given polygons
    for the purpose of path planning and other queries. */
class SeDcdt
 { private :
    SeDcdtMesh*       _mesh;            // the mesh
    SeTriangulator*   _triangulator;    // the used triangulator
    SeDcdtSymEdge*    _first_symedge;   // symedge at the border, but not at the back face
    SeDcdtFace*       _cur_search_face; // a face near the last change in the mesh, 0 if not valid
    SrSet<SeDcdtInsPol> _polygons;      // inserted polygons, polygon 0 is always the domain
    SrArray<SeDcdtVertex*>  _varray;    // internal buffer
    SrArray<SeDcdtSymEdge*> _earray;    // internal buffer
    SrArray<SeDcdtSymEdge*> _stack;     // internal buffer
    SrArray<SeDcdtVertex*>  _varray2;   // internal buffer
    SrArray<int> _ibuffer;
    float _radius;
    bool _using_domain;
    float _xmin, _xmax, _ymin, _ymax;

   public :
    /*! Default constructor */
    SeDcdt ();

    /*! Destructor */
   ~SeDcdt ();

    /*! Returns a pointer to the internal maintained mesh. The user can then
        use low level methods of SeMesh for additional computation. However it
        is the user responsability to not conflict with SeDcdt methods. */
    const SeDcdtMesh* mesh() const { return _mesh; }

    /*! Put in the given arrays the coordinates of the constrained and
        unconstrained edges endpoints. Each two consecutive points in the
        returned arrays give the first and end points of one dge.
        Parameters can be null indicating that no data of that type is requested.
        The two parameters are also allowed to point to the same array */
    void get_mesh_edges ( SrArray<SrPnt2>* constr, SrArray<SrPnt2>* unconstr );

    /*! Save the current dcdt by saving the list of all inserted obstacles.
        Note that the polygons ids are preserved. */
    bool save ( SrOutput& out );

    /*! Destructs the current map and loads a new one */
    bool load ( SrInput& inp );
    
    /*! Initializes the triangulation with a domain polygon.
        The domain is considered to be the constraint polygon with id 0; and can
        be retrieved again by calling get_polygon(0).
        An external box is automatically computed as an expanded bounding box of
        the domain, where the expansion vector length is 1/5 of the bounding box sides.
        This external box defines the border of the triangulation and its coordinates
        can be retrieved with get_bounds().
        Note that all polygons inserted afterwards must be inside the external box.
        Parameter radius can be used, for instance, to have a security margin in order
        to allow growing polygons without intersecting with the external box.
        If parameter radius is >0, it is used as minimum length for the
        expansion vector used to compute the external box.
        Special Case: If radius is 0, the domain polygon is not inserted, and the
        triangulation is initialized with border equal to the bounding box of domain.
        Parameter epsilon is simply passed to the triangulator. */
    void init ( const SrPolygon& domain, float epsilon, float radius=-1 );

    /*! Internally, the border is generated containing the domain polygon.
        This method allows to retrieve the coordinates of the border rectangle. */
    void get_bounds ( float& xmin, float& xmax, float& ymin, float& ymax ) const;

    /*! Inserts a polygon as a constraint in the CDT, returning its id.
        In case of error, -1 is returned. Polygons can be open.
        The returned id can be used to remove the polygon later on.
        All kinds of intersections and overllapings are handled.
        Collinear vertices are inserted. If not desired, method
        SrPolygon::remove_collinear_vertices() should be called prior insertion. */
    int insert_polygon ( const SrPolygon& polygon );

    /*! Returns the max id currently being used. Note that a SrSet controls
        the ids, so that the max id may not correspond to the number of 
        polygons inserted; ids values are not changed with polygon removal */
    int polygon_max_id () const;

    /*! Returns the number of inserted polygons, including the domain (if inserted). */
    int num_polygons () const;

    /*! Remove a previoulsy inserted polygon. false may be returned if the id is
        not valid or if some internal error occurs. The domain cannot be removed with
        this method. Steiner points may stay as part of other polygons if the
        triangulation is in conforming mode. */
    void remove_polygon ( int polygonid );

    /*! Retrieves the original vertices of the given polygon (without collinear vertices).
        If the returned polygon is empty, it means that polygonid is invalid. */
    void get_polygon ( int polygonid, SrPolygon& polygon );

    /*! Returns the vertices and edges used by the polygon in the triangulation.
        Elements may be out of order, specially when they have intersections.
        If an argument is a null pointer, nothing is done with it. */
    void get_triangulated_polygon ( int polygonid, SrArray<SeDcdtVertex*>* vtxs, SrArray<SeDcdtEdge*>* edgs );

    /*! Returns a list with the ids of the polygons having some edge traversed by the segment
        [(x1,y1),(x2,y2)]. The length of the returned array will not be more than depth,
        corresponding to 'depth' edges being crossed. Note: the id of a polygon will
        appear both when the ray enters the polygon and when it leaves the polygon.
        If depth is <0, no depth control is used.
        This routine can also be used to fastly determine if a point is inside a polygon
        by looking if the number of intersections is odd or even. */
    void ray_intersection ( float x1, float y1, float x2, float y2, SrArray<int>& polygons, int depth );

    /*! Returns all polygons describing the contours of an "eating virus" starting at x,y.
        Array pindices contains, for each contour, the starting and ending vertex index,
        which are sequentially stored in array vertices. */
    void extract_contours ( SrPolygon& vertices, SrArray<int>& pindices, float x, float y );

    /*! Returns the id of the first found polygon containing the given point (x,y),
        or -1 if no polygons are found. The domain polygon, if used in init(), will not
        be considered. The optional parameter allpolys can be sent to return all polygons
        containing the point, and only the first one.
        Note: this method does a linear search over each polygon, alternativelly, the
        ray_intersection() method might also be used to detect polygon containement. */
    int inside_polygon ( float x, float y, SrArray<int>* allpolys=0 );

    /*! Returns the id of one polygon close to the given point (x,y), or -1 otherwise.
        This method locates the point (x,y) in the triangulation and then takes the
        nearest polygon touching that triangle.
        The domain polygon, if used in init(), will not be considered. */
    int pick_polygon ( float x, float y );

    /*! Search for the channel connecting x1,y1 and x2,y2.
        It simply calls SeTriangulator::search_path(), however here parameter iniface is optional */
    bool search_path ( float x1, float y1, float x2, float y2,
                       const SeDcdtFace* iniface=0, bool vistest=false );

    /*! Returns a reference to the list with the interior edges of the last channel
        determined by a sussesfull call to search_path */
    const SrArray<SeBase*>& get_channel_interior_edges () const
          { return _triangulator->get_channel_interior_edges(); }

    /*! Returns a polygon describing the current channel, and thus, method find_path must
        be succesfully called before to determine the channel to consider. */
    void get_channel_boundary ( SrPolygon& channel ) { _triangulator->get_channel_boundary(channel); }

    /*! Returns the canonical path, which is the path passing through the midpoint of
        the channel interior edges. Method search_path must be succesfully called before
        in order to determine the channel. The path is returned as an open polygon. */
    void get_canonical_path ( SrPolygon& path ) { _triangulator->get_canonical_path(path); }

    /*! Returns the shortest path inside the current channel using the funnel algorithm. */
    void get_shortest_path ( SrPolygon& path ) { _triangulator->get_shortest_path(path); }

//DJD: prototypes for functions used in abstraction {

public:
#if defined DEBUG
	//the triangle on which the mouse pointer currently resides
	SeDcdtFace *cursor;
#endif
	//adds abstraction information to the triangulation
	void Abstract();
	//deletes all abstraction information from the triangulation
	void DeleteAbstraction();
private:
	//the "outside" face of the triangulation
	SeDcdtFace *outside;
	//abstract an acyclic component of the triangulation graph
	void TreeAbstract(SeDcdtFace *first, int component);
	//collapse an acyclic portion into the degree-2 root given
	void TreeCollapse(SeDcdtFace *root, SeDcdtFace *currentFace, int component);

//prototypes for functions used in abstraction }

//DJD: prototypes for functions used in width calculation {

private:
	//calculates all widths through a triangle
	void CalculateWidths(SeDcdtFace *face);
	//calculates a particular width through a triangle
	float TriangleWidth(SeBase *s);
	//search across an edge for the width through the triangle
	float SearchWidth(float x, float y, SeBase *s, float CurrentWidth);
	//checks if the position given in the face given is at least r from any obstacle
	bool ValidPosition(float x, float y, SeDcdtFace *face, float r);
	//determines if angles (x, y)-(x1, y1)-(x2, y2) and (x, y)-(x2, y2)-(x1, y1) are accute
	bool Consider(float x, float y, float x1, float y1, float x2, float y2);
	//determines the minimum distance between the point (x, y) and the line going through (x1, y1)-(x2, y2)
	float PointLineDistance(float x, float y, float x1, float y1, float x2, float y2);

//prototypes for functions used in width calculation }

//DJD: protptypes for functions used in point location {

public:
	//perform initialization for sector-based point location on the unprocessed triangles
	void InitializeSectors();
	//checks if the given point is inside the given triangle
	bool InTriangle(SeDcdtFace *face, float x, float y);
	//use sector-based point location to find the face in which the given point resides
	SeTriangulator::LocateResult LocatePoint(float x, float y, SeBase* &result);
	//use regular point location to perform the above
	SeTriangulator::LocateResult LocatePointOld(float x, float y, SeBase* &result);
	//generate a random valid x value for testing point location
	float RandomX();
	//generate a random valid y value for testing point location
	float RandomY();
private:
	//the number of sectors in the vertical dimension
	const static int ySectors = 10;
	//the number of sectors in the horizontal dimension
	const static int xSectors = 10;
	//the width of a single sector
	float sectorWidth;
	//the height of a single sector
	float sectorHeight;
	//the array of sectors
	SeDcdtFace *sectors[ySectors][xSectors];
	//return the maximum of the 3 numbers passed
	float Max(float a, float b, float c);
	//return the maximum of the 3 numbers passed
	float Min(float a, float b, float c);
	//return a random value between those given
	float RandomBetween(float min, float max);

//protptypes for functions used in point location }

//DJD: prototypes for functions used in abstracted space searching {

public:
#if defined EXPERIMENT
	//find given path with TRA* and return experimental data
	bool SearchPathFast(float x1, float y1, float x2, float y2, float r, Data *data, int numData);
	//find given path with TA* and return experimental data
	bool SearchPathBaseFast(float x1, float y1, float x2, float y2, float r, Data *data, int numData);
#else
	//find given path with TRA*
	bool SearchPathFast(float x1, float y1, float x2, float y2, float r);
	//find given path with TA*
	bool SearchPathBaseFast(float x1, float y1, float x2, float y2, float r);
#endif
	//get the shortest path between the given points within the given channel of triangles
	float GetShortestPath(SrPolygon& path, SrArray<SeBase *> Channel, float x1, float y1, float x2, float y2, float r);
	//return the polygon around the current channel
	SrPolygon GetChannelBoundary();
	//return the current path
	SrPolygon &GetPath();
private:
#if defined EXPERIMENT
	//search for a path in an acyclic portion of the triangulation graph and count the triangles searched
	bool Degree1Path(SrArray<SeBase *>& path, float x1, float y1, SeDcdtFace *startFace,
        float x2, float y2, SeDcdtFace *goalFace, float r, int &nodes);
#else
	//search for a path in an acyclic portion of the triangulation graph
	bool Degree1Path(SrArray<SeBase *>& path, float x1, float y1, SeDcdtFace *startFace,
        float x2, float y2, SeDcdtFace *goalFace, float r);
#endif
	//search for a path along a degree-2 corridor
	bool Degree2Path(SrArray<SeBase *>& path, SeDcdtFace *startFace, SeDcdtFace *nextFace,
		SeDcdtFace *goalFace, float r);
	//search for a path along a degree-2 corridor, crossing a degree-3 node
	bool FollowLoop(SrArray<SeBase *>& path, SeDcdtFace *startFace, SeDcdtFace *nextFace,
		SeDcdtFace *goalFace, SeDcdtFace *degree3, float r);
	//walk from one face to a given adjacent face, in a direction specified
	bool WalkBetween(SrArray<SeBase *>& path, SeDcdtFace *sourceFace, SeDcdtFace *destinationFace, float r, int direction = INVALID);
	//determine if an object of given radius can cross the middle triangle between the adjacent ones given
	bool CanCross(SeBase *first, SeBase *second, SeBase *third, float r);
	bool CanCross(SeDcdtFace *middle, SeBase *end1, SeBase *end2, float r);
	//checks if a unit of radius r can enter the path from the start and goal
	bool ValidEndpoints(SrArray<SeBase *>& path, float x1, float y1, float x2, float y2, float r);
	//checks if a unit has to cross the middle of the triangle to enter the path from the point given
	bool ValidEndpoint(SeBase *s, float x, float y, bool left);
	//determines if a unit can fit through the middle of the given triangle to get to the path
	bool ValidEndpoint(SeDcdtFace *first, SeDcdtFace *second, float x, float y, float r);
	//checks if a given path is valid for a unit of radius r
	bool ValidPath(SrArray<SeBase *>& path, float x1, float y1, float x2, float y2, float r);
	//returns the closest point in a triangle to a given point for a unit of given radius
	SrPnt2 ClosestPointTo(SeDcdtFace *face, float x, float y, float r);
	//given a search node, construct the channel it represents
	SrArray<SeBase *> ConstructBaseChannel(SearchNode *goal);
	//given a search node, construct a channel, filling in degree-1 and 2 nodes between those searched
	void ConstructBasePath(SrArray<SeBase *> &path, SearchNode *goalNode, SeDcdtFace *start, SeDcdtFace *goal, int direction = INVALID);
	//the current channel
	SrArray<SeBase *> currentChannel;
	//the current path
	SrPolygon currentPath;
	//the current start position
	SrPnt2 startPoint;
	//the current goal position
	SrPnt2 goalPoint;

//prototypes for functions used in abstracted space searching }

//DJD: prototypes for utility functions {

public:
	//retrieves the vertices and whether the edges are constrained, for a given triangle
	void TriangleInfo(SeDcdtFace *face, float &x1, float &y1, bool &e1, 
		float &x2, float &y2, bool &e2, float &x3, float &y3, bool &e3);
	void TriangleInfo(SeBase *s, float &x1, float &y1, bool &e1, 
		float &x2, float &y2, bool &e2, float &x3, float &y3, bool &e3);
	//revireves the vertices for a given triangle
	void TriangleVertices(SeDcdtFace *face, float &x1, float &y1,
		float &x2, float &y2, float &x3, float &y3);
	void TriangleVertices(SeBase *s, float &x1, float &y1,
		float &x2, float &y2, float &x3, float &y3);
	//retrieves whether the edges are constrained, for a given triangle
	void TriangleEdges(SeDcdtFace *face, bool &e1, bool &e2, bool &e3);
	void TriangleEdges(SeBase *s, bool &e1, bool &e2, bool &e3);
	//returns the midpoint of a given triangle
	void TriangleMidpoint(SeDcdtFace *face, float &x, float &y);
	SrPnt2 TriangleMidpoint(SeDcdtFace *face);
	//returns whether a given edge is either constrained, or borders the "outside" polygon
	bool Blocked(SeBase *s);
	//returns the degree of a triangle's node (UNABSTRACTED if it has not been mapped)
	int Degree(SeBase *s);
	int Degree(SeFace *face);
private:
	//returns whether the given angle is accute
	bool IsAccute(float theta);
	//returns whether the given angle is obtuse
	bool IsObtuse(float theta);
	//returns the length of the line segment (x1, y1)-(x2, y2)
	float Length(float x1, float y1, float x2, float y2);
	//returns the smaller of the two values passed
	float Minimum(float a, float b);
	//returns the larger of the two values passed
	float Maximum(float a, float b);
	//determines the angle between (x1, y1)-(x2, y2)-(x3, y3)
	float AngleBetween(float x1, float y1, float x2, float y2, float x3, float y3);
	//returns 0 if the points are colinear, <0 if they are in clockwise order, and >0 for counterclockwise
	float Orientation(float x1, float y1, float x2, float y2, float x3, float y3);
	//returns the minimum distance between the line segments (A1x, A1y)-(A2x, A2y) and (B1x, B1y)-(B2x, B2y)
	float SegmentDistance(float A1x, float A1y, float A2x, float A2y, float B1x, float B1y, float B2x, float B2y);
	//returns the minimum distance between the line segments (ls1x, ls1y)-(ls2x, ls2y) and (ll1x, ll1y)-(ll2x, ll2y),
	//at least r away from the segments' end points
	float SegmentDistance(float ls1x, float ls1y, float ls2x, float ls2y, float ll1x, float ll1y, float ll2x, float ll2y, float r);
	//returns the minimum distance between point (x, y) and segment (x1, y1)-(x2, y2)
	float PointSegmentDistance(float x, float y, float x1, float y1, float x2, float y2);
	//returns the closest point on the segment (x1, y1)-(x2, y2) to point (x, y),
	//at least r away from th segments' midpoints
	SrPnt2 ClosestPointOn(float x1, float y1, float x2, float y2, float x, float y, float r);

//prototypes for utility functions }

   private :
    SeDcdtFace* _search_face() { return _cur_search_face? _cur_search_face:_first_symedge->fac(); }
    void _find_intermediate_vertices_and_edges ( SeDcdtVertex* vini, int oid );
    bool _is_intersection_vertex ( SeDcdtVertex* v, int oid, SeDcdtVertex*& v1, SeDcdtVertex*& v2 );
    void _remove_vertex_if_possible ( SeDcdtVertex* v, const SrArray<int>& vids );
    void _add_contour ( SeDcdtSymEdge* s, SrPolygon& vertices, SrArray<int>& pindices );
    SeDcdtSymEdge* _find_one_obstacle ();
 };

//================================== End of File =========================================

# endif // SE_DCDT_H
