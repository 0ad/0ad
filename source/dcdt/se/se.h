
# ifndef SE_H
# define SE_H

/** \file se.h
 * Sym Edge definitions
 \code
 ******************************************************************************************
 *
 *  SymEdge Mesh Data Structure
 *  Marcelo Kallmann 1996 - 2004
 *  
 *  SymEdge joins Guibas & Stolfi structure ideas with Mäntylä Euler operators.
 *  Several triangulation algorithms were implemented in the se toolkit
 *
 *  References:
 *  1. M. Kallmann, H. Bieri, and D. Thalmann, "Fully Dynamic Constrained Delaunay
 *     Triangulations", In Geometric Modelling for Scientific Visualization, 
 *     G. Brunnett, B. Hamann, H. Mueller, L. Linsen (Eds.), ISBN 3-540-40116-4,
 *     Springer-Verlag, Heidelberg, Germany, pp. 241-257, 2003. 
 *  2. M. Kallmann, and D. Thalmann, "Star-Vertices: A Compact Representation
 *     for Planar Meshes with Adjacency Information", Journal of Graphics Tools,
 *     2001. (In the comparisons table, the symedge was used and called as the
 *     "simplified quad-edge structure" )
 *  3. M. Mäntylä, "An Introduction to Solid Modeling", Computer Science Press, 
 *     Maryland, 1988.
 *  4. L. Guibas and J. Stolfi, "Primitives for the Manipulation of General 
 *     Subdivisions and the Computation of Voronoi Diagrams", ACM Transaction
 *     on Graphics, 4, 75-123, 1985.
 *
 *  The structure is based on circular lists of SeElement for vertex/edge/face nodes, 
 *  plus the connectivity information stored with SeBase class.
 *
 *  Implementation History:
 *
 *  10/2004 - Becomes again SE lib, but with dependendy on the SR lib.
 *
 *  10/2003 - Integration to the SR library, which is the newer verion of FG.
 *
 *  08/2002 - SeGeo becomes abstract, and SeGeoPoint, SeGeoFloat, SeGeoPointFloat appear.
 *          - SeTriangulator now covers both constrained and conforming triangulations
 *          - SrClassManagerBase signature is no longer used
 *
 *  06/2001 - The library becomes again a standalone library.
 *
 *  09/2000 - Changed to always keep v/e/f information lists
 *          - New names starting with Se
 *
 *  05/2000 - Finally operator mg was done to convert triangle soups with genus>0
 *
 *  10/1999 - Adaptation to the FG library, using FgArray, FgListNode, FgTree, I/O, etc.
 *
 *  06/1998 - Abandoned the idea of using a base algorithm class MeshAlg
 *
 *  07/1997 - Automatic calculation of MaxindexValue
 *          - Import() and related virtual callbacks created
 *          - Triangulation now is done with virtual callbacks
 *
 *  07/1996 - First implementation
 *
 ******************************************************************************************
\endcode */

//================================ Types and Constants ===============================

class SeMeshBase;
class SeBase;
class SeElement;

typedef unsigned int semeshindex;    //!< internally used to mark elements (must be unsigned)
typedef SeElement SeVertex;  //!< a vertex is an element
typedef SeElement SeEdge;    //!< an edge is an element
typedef SeElement SeFace;    //!< a face is an element

//=================================== SeElement =======================================

/*! SeElement contains the required information to be attached
    to all vertices, edges and faces:
    1. a reference to one (any) adjacent SeBase
    2. pointers maintaining a circular list of all elements of
       the same type (vertices, edges or faces)
    3. an index that can be used by SeMeshBase to mark elements
    To attach user-related information to an element:
    1. SeElement must be derived and all user data is declared
       inside the derived class.
    2. A corresponding SrClassManager must be derived and
       the required virtual methods must be re-written in order to
       manage the derived SeElement class. The compare method
       is not used. See also sr_class_manager.h. */
class SeElement
 { protected :
    SeElement ();
   public :
    SeBase*    se () const { return _symedge; }
    SeElement* nxt() const { return _next; }
    SeElement* pri() const { return _prior; }
   private :
    friend class SeMeshBase;
    friend class SrClassManagerBase;
    SeElement*  _next;
    SeElement*  _prior;
    SeBase*     _symedge;
    semeshindex _index;
    SeElement*  _remove ();
    SeElement*  _insert ( SeElement* n );
 };

/*! The following define can be called in a user derived class of SeElement
    to easily redefine public SeElement methods with correct type casts.
    E stands for the element type, and S for the sym edge type. */
# define SE_ELEMENT_CASTED_METHODS(E,S) \
     S* se() const { return (S*)SeElement::se(); } \
     E* nxt() const { return (E*)SeElement::nxt(); } \
     E* pri() const { return (E*)SeElement::pri(); } 

/*! The following define can be used to fully declare a default user derived
    class of SeElement, which contains no user data but correctly redefines
    public SeElement methods with type casts. */
# define SE_DEFINE_DEFAULT_ELEMENT(E,S) \
     class E : public SeElement \
      { public : \
        SE_ELEMENT_CASTED_METHODS(E,S); \
        E () {} \
        E ( const E& e ) {} \
        friend SrOutput& operator<< ( SrOutput& o, const E& e ) { return o; } \
        friend SrInput& operator>> ( SrInput& i, E& e ) { return i; } \
        friend int sr_compare ( const E* e1, const E* e2 ) { return 0; } \
      };

//================================== SeBase ========================================

/*! Used to describe all adjacency relations of the mesh topology. The mesh itself is 
    composed of a net of SeBase elements linked together reflecting the vertex and 
    face loops. SymEdge is a short name for symetrical edge, as each SeBase has a
    symetrical one incident to the same edge on the opposite face, and is given by
    sym(). SeBase has local traverse operators permitting to change to any adjacent
    symedge so to access any information stored on a vertex, edge or face. Symedges 
    are also used as parameters to the topological operators of SeMeshBase, which allow
    modifying the mesh. */
class SeBase
 { public :
    /*! Returns the next symedge adjacent to the same face. */
    SeBase* nxt() const { return _next; }

    /*! Returns the prior symedge adjacent to the same face. */
    SeBase* pri() const { return _rotate->_next->_rotate; }

    /*! Returns the next symedge adjacent to the same vertex. */
    SeBase* rot() const { return _rotate; }

    /*! Returns the prior symedge adjacent to the same vertex. */
    SeBase* ret() const { return _next->_rotate->_next; }

    /*! Returns the symmetrical symedge, sharing the same edge. */
    SeBase* sym() const { return _next->_rotate; }

    /*! Returns the element attached to the incident vertex. */
    SeVertex* vtx() const { return _vertex; }

    /*! Returns the element attached to the incident edge. */
    SeEdge* edg() const { return _edge; } 

    /*! Returns the element attached to the incident face. */
    SeFace* fac() const { return _face; } 

   private :  
    friend class SeMeshBase;
    friend class SeElement;
    SeBase* _next;
    SeBase* _rotate;
    SeVertex* _vertex;
    SeEdge* _edge;
    SeFace* _face;
 };

/*! This is the template version of the SeBase class, that redefines the methods
    of SeBase including all needed type casts to the user defined classes.
    All methods are implemented inline just calling the corresponding method of
    the base class but correctly applying type casts to convert default types
    to user types.
    Important Note: no user data can be stored in sym edges. This template class
    is only used as a technique to correctly perform type casts. */
template <class V, class E, class F>
class Se : public SeBase
 { public :
    Se* nxt() const { return (Se*)SeBase::nxt(); }
    Se* pri() const { return (Se*)SeBase::pri(); }
    Se* rot() const { return (Se*)SeBase::rot(); }
    Se* ret() const { return (Se*)SeBase::ret(); }
    Se* sym() const { return (Se*)SeBase::sym(); }
    V* vtx() const { return (V*)SeBase::vtx(); }
    E* edg() const { return (E*)SeBase::edg(); }
    F* fac() const { return (F*)SeBase::fac(); }
 };

//============================ End of File ===============================

# endif // SE_H

