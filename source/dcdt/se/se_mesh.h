
# ifndef SE_MESH_H
# define SE_MESH_H

/** \file se_mesh.h
 * Symmetrical Edge Mesh Data Structure */

# include "sr_array.h"
# include "sr_input.h"
# include "sr_output.h"
# include "sr_class_manager.h"

# include "se.h"

// ====================================== SeMeshBase ======================================

// SeMeshBase documentation in the end of this file
class SeMeshBase 
 { public :
    /*! Enumerator with the possible results of check_all(). */
    enum ChkMsg { ChkOk,        //!< The check_all() method had success
                  ChkNxtError,  //!< Some face loop defined by the nxt() operators is wrong
                  ChkRotError,  //!< Some vertex loop defined by the rot() operators is wrong
                  ChkVtxError,  //!< Vertex referencing is wrong
                  ChkEdgError,  //!< Edge referencing is wrong
                  ChkFacError   //!< Face referencing is wrong
                };

    /*! Enumerator with possible errors raised during the use of the topological operators. */
    enum OpMsg  { OpNoErrors,           //!< No Errors occured with operators
                  OpMevParmsEqual,      //!< Parms in mev are equal
                  OpMevNotSameVtx,      //!< Parms in mev not in the same vertex
                  OpMefNotSameFace,     //!< Parms in mef not in the same face
                  OpKevLastEdge,        //!< Cannot delete the last edge with kev
                  OpKevManyEdges,       //!< More then one edge in parm vertex in kev
                  OpKevOneEdge,         //!< Vertex of parm has only one edge in kev
                  OpKefSameFace,        //!< Parm in kef must be between two different faces
                  OpMgUncompatibleFaces,//!< Parms are adjacent to faces with diferent number of vertices
                  OpMgSameFace,         //!< Parm in mg must be between two different faces
                  OpFlipOneFace,        //!< Parm in flip must be between two different faces
                  OpEdgColOneFace,      //!< Parm in EdgCol must be between two different faces
                  OpVtxSplitNotSameVtx, //!< Parms in vtxsplit are not in the same vertex
                  OpVtxSplitParmsEqual  //!< Parms in VtxSplit are equal
                };

    /*! Enumerator with the type of elements used in many methods arguments. */
    enum ElemType { TypeVertex,
                    TypeEdge,
                    TypeFace
                  };

   private : // Data ===================================================================================

    SeBase* _first;  // A pointer to the symedge of the mesh that is considered the first
    char _op_check_parms;   // Flag to tell if must check operators parameters (default false)
    char _op_last_msg;      // Keeps the last result of an operator (OpMsg type)
    char _output_op_errors; // Flag that outputs all errors ocurred during operators (default true)
    int _vertices, _edges, _faces; // Elem counters
    semeshindex _curmark;              // Current values for marking
    char _marking, _indexing;      // flags to indicate that marking or indexing is on
    SrClassManagerBase *_vtxman, *_edgman, *_facman; // keep used managers
 
  private : // Private methods ==========================================================================
    void _defaults ();
    SeBase* _op_error ( OpMsg m );

   public : // General mesh functions ===================================================================

    /*! Inits an empty mesh. It requires to receive three manager classes
        dynamically allocated with operator new. These classes will manage data
        stored in vertices, edges and faces. If no associated data is required,
        the user can simply pass as argument "new SrClassManagerMeshElementManager", otherwise
        an allocated user-derived object must be created and passed to this 
        constructor. SeMeshBase destructor will automatically delete them. */
    SeMeshBase ( SrClassManagerBase* vtxman, SrClassManagerBase* edgman, SrClassManagerBase* facman );

    /*! Deletes all associated data: internal buffer, all mesh and its infos. */
    virtual ~SeMeshBase ();

    /*! Returns the initial symedge of the mesh, can be 0. */
    SeBase* first () const { return _first; }

    /*! Returns true if the mesh is empty, what happens when first() is null. */
    bool empty () const { return _first? false:true; }

    /*! Returns v-e+f. */
    int euler () const; 

    /*! Returns (v-e+f-2)/(-2). */
    int genus () const;

    /*! Returns the number of elements of the given type. */
    int elems ( ElemType type ) const;

    /*! Returns the number of vertices in the mesh. */
    int vertices () const { return _vertices; }

    /*! Returns the number of edges in the mesh. */
    int edges () const { return _edges; }

    /*! Returns the number of faces in the mesh. */
    int faces () const { return _faces; }

    /*! Invert orientation of all faces. */
    void invert_faces ();

    /*! Returns the number of vertices in the face adjacent of the given symedge e. */
    int vertices_in_face ( SeBase* e ) const;

    /*! Returns the number of edges incident to the vertex adjacent of the given symedge e,
        ie, the degree of the vertex. */
    int vertex_degree ( SeBase* e ) const;

    /*! Returns the mean vertex degree of the whole mesh */
    float mean_vertex_degree () const;

    /*! Returns true if the face incident to the given symedge e has exactly three edges. */
    bool is_triangle ( SeBase* e ) const { return e->nxt()->nxt()->nxt()==e? true:false; }

   private : // Marking and Indexing =====================================================
    void _normalize_mark ();

   public :

    /*! Allows the user to safely mark elements with mark(), marked() and unmark()
        methods. Marking elements is necessary for many algorithms.
        SeMeshBase mantains a current marker value, so that each time begin_marking()
        is called, the marker value is incremented, guaranteing that old marked
        elements have a different mark value.
        When the max marking value is reached, a global normalization is done.
        Attention: Marking and indexing can not be used at the same time. Also, 
        end_marking() must be called when marking operations are finished! */
    void begin_marking ();

    /*! Must be called after a begin_marking() call, when the user finishes using the 
        marking methods. Note: There is no problem if end_marking() is called without
        a previously call to begin_marking() */
    void end_marking ();

    /*! See if an element is marked (begin_marking() should be called first) */
    bool marked ( SeElement* e );

    /*! Unmark an element (begin_marking() should be called first) */
    void unmark ( SeElement* e );

    /*! Mark an element (begin_marking() should be called first) */
    void mark ( SeElement* e );

    /*! Indexing methods can be used only after a call to begin_indexing(), which permits
        the user to associate any index to any element. Note however that after a call to
        begin_indexing() the existant indices will have undefined values. This happens
        because indices and markers share the same variables, so that both can not be used
        at a  same time. Method end_marking() must always be called after */
    void begin_indexing ();

    /*! To finish indexing mode, end_indexing() must be called. The method will do a global
        normalization of the internal indices values, preparing them to be used for a later
        marking or indexing session. It can be called without a previous begin_marking()
        call, but note that it will always peform the global normalization. */
    void end_indexing ();

    /*! Retrieves the index of an element (begin_indexing() should be called first) */
    semeshindex index ( SeElement* e );

    /*! Sets the index of an element (begin_indexing() should be called first) */
    void index ( SeEdge* e, semeshindex i );

   public : // Operators Control ===========================================================================

    /*! Get safe mode status. When it is true, error msgs are generated, but
        operations become a little slower because some minor checkings are 
        performed. The default mode is true only for the
        debug version of the library (compiled with _DEBUG macro defined).*/
    bool safe_mode () { return _op_check_parms? true:false; } 

    /*! Change the safe mode status. The default mode is false. */
    void safe_mode ( bool b ) { _op_check_parms=(char)b; }

    /*! Say if error messages should be printed or not (using sr_out), default is false. */
    void output_errors ( bool b ) { _output_op_errors=(char)b; }

    /*! Whenever an operator returns null, an error message is generated and
        can be retrieved here. Each time an operator is called, a new message
        is generated. And whenever the operator finishes succesfully OpNoErrors
        is generated. */
    OpMsg last_op_msg () { return (OpMsg)_op_last_msg; } 

    /*! Get a string description of the given message. */
    static const char *translate_op_msg ( OpMsg m );

    /*! Makes a global consistency check, for debug purposes, should always return ChkOk. */
    ChkMsg check_all () const;

   private :
    void _new_vtx(SeBase* s); void _new_edg(SeBase* s); void _new_fac(SeBase* s);
    void _del_vtx(SeElement* e); void _del_edg(SeElement* e); void _del_fac(SeElement* e);

   public : // Operators ==================================================================================

    /*! Destroy all the mesh and associated data. */
    void destroy ();

    /*! Initialize the mesh. See also SeMeshBase documentation. */
    SeBase* init ();

    /*! Make edge and vertex operator. See also SeMeshBase documentation. */
    SeBase* mev ( SeBase* s );

    /*! Make edge and vertex operator, splitting a vertex loop. See also SeMeshBase documentation. */
    SeBase* mev ( SeBase* s1, SeBase* s2 );

    /*! Make edge and face operator. See also SeMeshBase documentation. */
    SeBase* mef ( SeBase* s1, SeBase* s2 );

    /*! Kill edge and vertex operator. See also SeMeshBase documentation. */
    SeBase* kev ( SeBase* x );

    /*! Kill edge and vertex operator that can join two vertex loops. See also SeMeshBase documentation. */
    SeBase* kev ( SeBase* x, SeBase* *s );

    /*! Kill edge and face operator. See also SeMeshBase documentation. */
    SeBase* kef ( SeBase* x, SeBase* *s );

    /*! Make genus by joining two face contours and deleting two faces, 
        vertices and edges of s2 face. s1 and s2 must be symmetrically 
        placed, each one on its contour to join. (not fully tested) */
    SeBase* mg ( SeBase* s1, SeBase* s2 );

    /*! Flip the edge of a face. See also SeMeshBase documentation. */
    SeBase* flip ( SeBase* x );

    /*! Edge collide operator. See also SeMeshBase documentation. */
    SeBase* ecol ( SeBase* x, SeBase* *s );

    /*! Vertex split operator. See also SeMeshBase documentation. */
    SeBase* vsplit ( SeBase* s1, SeBase* s2 );

    /*! Add vertex operator. See also SeMeshBase documentation. */
    SeBase* addv ( SeBase* x );

    /*! Delete vertex operator. See also SeMeshBase documentation. */
    SeBase* delv ( SeBase* y );

   private : // IO ========================================================================================
    void _elemsave ( SrOutput& out, ElemType type, SeElement* first );
    void _elemload ( SrInput& inp, SrArray<SeElement*>& e, const SrArray<SeBase*>& s, SrClassManagerBase* man );

   public :

    /*! Writes the mesh to the given output. save() will invoke the 
        virtual methods output() of the associated element managers.
        Note: indexing is used during save().  */
    bool save ( SrOutput& out );

    /*! Reads the mesh from the given input. load() will invoke the 
        virtual methods input() of the associated element managers.
        Note: indexing is used during load().  */
    bool load ( SrInput& inp );
 };

/*! This is the template version of the SeMeshBase class, that redefines some methods
    of SeBase only for the purpose of including type casts to user defined classes.
    All methods are implemented inline just calling the corresponding method of
    the base class but correctly applying type casts to convert default types
    to user types. It must be used together with SeTpl. */
template <class V, class E, class F> //, class VM, class EM, class FM>
class SeMesh : public SeMeshBase
 { public :
    typedef Se<V,E,F> S;
    typedef SrClassManager<V> VM;
    typedef SrClassManager<E> EM;
    typedef SrClassManager<F> FM;

    SeMesh () : SeMeshBase ( new VM, new EM, new FM ) {}

    S* first () const { return (S*)SeMeshBase::first(); }

    S* init ()                 { return (S*)SeMeshBase::init(); }
    S* mev ( S* s )            { return (S*)SeMeshBase::mev(s); }
    S* mev ( S* s1, S* s2 )    { return (S*)SeMeshBase::mev(s1,s2); }
    S* mef ( S* s1, S* s2 )    { return (S*)SeMeshBase::mef(s1,s2); }
    S* kev ( S* x )            { return (S*)SeMeshBase::kev(x); }
    S* kev ( S* x, S* *s )     { return (S*)SeMeshBase::kev(x,(SeBase**)s); }
    S* kef ( S* x, S* *s )     { return (S*)SeMeshBase::kef(x,(SeBase**)s); }
    S* mg ( S* s1, S* s2 )     { return (S*)SeMeshBase::mg(s1,s2); }
    S* flip ( S* x )           { return (S*)SeMeshBase::flip(x); }
    S* ecol ( S* x, S* *s )    { return (S*)SeMeshBase::ecol(x,(SeBase**)s); }
    S* vsplit ( S* s1, S* s2 ) { return (S*)SeMeshBase::vsplit(s1,s2); }
    S* addv ( S* x )           { return (S*)SeMeshBase::addv(x); }
    S* delv ( S* y )           { return (S*)SeMeshBase::delv(y); }
 };

/*! \class SeMeshBase se_mesh.h
    \brief manages topology and attached information of a symedge mesh.

    SeMeshBase uses SeBase as argument for many methods, specially for the
    topological operators. Each SeBase instance has only one vertex, edge
    and face adjacent (or associated) and so it can be used to reference any
    of these elements. SeMeshBase destructor calls destroy(), that will call also
    each free() method of registered SeElementManagers in the mesh. To attach info
    to a mesh, the user must derive SrClassManagerBase, suply the required virtual
    methods, and push it in SeMeshBase with push_info() method.

    Operators error handling is performed when the checking mode is true (the
    default), all operators may then return a 0 (null) pointer when their 
    parameters are illegal. In such cases, a call to last_op_msg() will return
    the error occured. Next is a schema explaining each operator: \code

   *========================================================================
   * Operator init()
   * 
   * x=init()             v1 o      o v2      ->      v1 o------o v2
   * destroy()                                            <-----
   * destroy all the previous structure.                   x
   * 

   *========================================================================
   * Operator mev(s)
   * 
   *                        s                                    x
   *                     <-----                               <-----
   * x=mev(s)           o------o      o v    ->       o------o------o v
   * s=kev(x)                / |                           / |
   *                       /   |                         /   |
   * To kev(x), only     o     o                       o     o
   * one edge in v;
   * 

   *========================================================================
   * Operator mev(s1,s2)
   *                           o                          o
   *                           |                            \
   *                           |  s1                          \   
   *                           | ---->                          \
   * x=mev(s1,s2)       o------o---------o   ->    o------o------o--o
   * s1=kev(x,&s2)       <---- |      o                   | <--- v
   *                       s2  |      v                   |   x
   * To mev, s1 & s2 on        o                          o
   *   the same vertex
   *   and s1!=s2
   * 
   * To kev(x), more than one edge of x must be incident to v
   * 

   *=========================================================================
   * Operator mef(s1,s2)
   * 
   * x=mef(s1,s2)       o-------o-------o           o-------o-------o
   * s1=kef(x,&s2)      | <----         |           |       |       |
   *                    |   s1          |           |       |       |
   *                    |               |           |       ||      |
   *    s1 & s2 on      |               |    ->     |      e||x     |
   *    same face       |               |           |       |V      |
   *                    |          s2   |           |       |   f   |
   * in kef(x), x->sym  |         ----> |           |       |       |
   * must be in a diff  o-------o-------o           o-------o-------o
   * face than x
   * 

   *========================================================================
   * Operator flip(x)
   *
   * y=flip(x)                  o---o---o                   o---o---o
   *                          / |       |                 /         |
   *                        /   |       |               /           |
   * x & x->sym must      /     ||      |             /             |
   * be in diff faces   o       ||x     o    ->     o---------------o
   * and share the      |       |V    /             |   <------   /
   * same edge          |       |   /               |      y    /
   *                    |       | /                 |         /
   *                    o---o---o                   o---o---o
   * 
   * This operator acts as an "edge rotation"; that in triangulations is the
   * same as the well known flip (or swap) operator.
   * 

   *========================================================================
   * Operator ecol(x,&s2)
   *                                                      
   * s1=ecol(x,&s2)                     o                o 
   * x=vsplit(s1,s2)                    / |              s1| 
   *                                  /   |               ^|  
   *                        \       / x   | /          \  || /
   * x and x->sym must        \   /  -->  |/             \ |/
   * be in diff faces           o---------o v     ->       o v
   * and share the            /   \       |\             / |\
   * same edge              /       \     | \          /   | \
   *                      /           \   |          / /   |
   *                    /               \ |        / /s2   |
   *                                      o         V      o 
   * 
   * In the process, 3 edges, 2 faces and 1 vertex are created/destroyed.
   * This operator acts as an "edge collapse", and has
   * the "vertex split" operator as inverse.
   * The three destroyed edges are incident to x->vtx(), and not v.
   *

   *========================================================================
   * Operator addv()
   *                                                      
   * y=addv(x)            -----------         ----------- 
   * x=delv(y)            |   <--   |         |\       /| 
   *                      |    x    |         | \   y^/ | 
   *                      |         |         |  \  //  | 
   *                      |         |         |   \ /   |
   *                      |    o    |   ->    |    o    |
   *                      |         |         |   / \   |
   *                      |         |         |  /   \  |
   *                      |         |         | /     \ |
   *                      |         |         |/       \|
   *                      -----------         -----------
   * 
   *
   \endcode */

//============================ End of File ===============================

# endif // SE_MESH_H

