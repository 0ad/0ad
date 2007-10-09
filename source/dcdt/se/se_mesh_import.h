
# ifndef SE_MESH_IMPORT_H
# define SE_MESH_IMPORT_H

/** \file sr_mesh_import.h 
 * Topology recovery from a list of triangles
 */

# include "se_mesh.h"

/*! \class SeMeshImport sr_mesh_import.h
    \brief Recovers adjacencies from a triangle list
    Note: SeMeshImport uses the indexing methods of SeMesh. */
class SeMeshImport
 { public :
    enum Msg { MsgOk,                
               MsgNullShell,         
               MsgNonManifoldFacesLost, 
               MsgFinished           
             };

    /*! This array will contain the recovered shells, and the user is reuired to
        get and take control of the stored pointers and after that, putting the size
        of the array to 0. If the array size is not 0, the destructor of SeMeshImport
        will delete each shell in the array. */
    SrArray<SeMeshBase*> shells;

   private :
    class Data;
    Data *_data;
    
   public :
    /*! Default constructor */
    SeMeshImport ();

    /*! The destructor will delete all meshes stored in the shells array.
        So that if the user wants to become responsible to maintain the
        imported meshes, he/she must take the pointers in the shell array
        and set the array size to 0. */
    virtual ~SeMeshImport ();
 
    /*! Compress some used internal buffers */
    void compress_buffers ();

    /*! Defines the information describing the triangle-based description to
        be imported. The list of triangles contains indices to the vertices.
        Geometrical information can be attached to the mesh by re-writing
        the availble virtual methods, which allow transforming indices
        into any kind of user-related data. */
    Msg start ( const int *triangles, int numtris, int numvtx );

    /*! Join another face to the mesh, expanding the current active contour.
        This method should be called until MsgFinished or errors occur */
    Msg next_step ();

   public : 

    /* This virtual method must be derived to return a valid SeMesh for
       each new shell encountered in the data being analysed. */
    virtual SeMeshBase* get_new_shell ()=0;

    /*! Optional virtual method to attach vertex information.
        The default implementation does nothing. */
    virtual void attach_vtx_info ( SeVertex* v, int vi );

    /*! Optional virtual method to attach edge information.
        The default implementation does nothing. */
    virtual void attach_edg_info ( SeEdge* e, int a, int b ); 

    /*! Optional virtual method to attach face information.
        The default implementation does nothing. */
    virtual void attach_fac_info ( SeFace* f, int fi );

    /*! Optional virtual method to attach face information of holes found.
        The default implementation does nothing. */
    virtual void attach_hole_info ( SeFace* f );
 };

//============================== end of file ===============================

# endif // SE_MESH_IMPORT_H
