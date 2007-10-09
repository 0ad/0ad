
# ifndef SR_SN_MATRIX_H
# define SR_SN_MATRIX_H

/** \file sr_sn_mat.h 
 * matrix transformation
 */

# include "sr_sn.h"

//======================================= SrSnMatrix ====================================

/*! \class SrSnMatrix sr_scene.h
    \brief Applies a matrix transformation

    SrSnMatrix accumulates the specified transformation matrix to the
    current transformation during traversals of the scene graph when
    an action is applied. */
class SrSnMatrix : public SrSn
 { private :
    SrMat _mat;

   public :
    static const char* class_name;

   protected :
    /*! Destructor only accessible through unref() */
    virtual ~SrSnMatrix ();

   public :
    /*! Default constructor */
    SrSnMatrix ();

    /*! Constructor receiving a matrix */
    SrSnMatrix ( const SrMat& m );

    /*! Set the matrix. */
    void set ( const SrMat& m ) { _mat=m; }

    /*! Get the matrix. */
    SrMat& get () { return _mat; }
 };


//================================ End of File =================================================

# endif  // SR_SN_MATRIX_H

