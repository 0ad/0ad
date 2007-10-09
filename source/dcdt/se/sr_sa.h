
# ifndef SR_SA_H
# define SR_SA_H

/** \file sr_sa.h 
 * scene action base class
 */

# include "sr_sn.h"

class SrSnGroup;
class SrSnEditor;
class SrSnMatrix;
class SrSnShapeBase;

/*! \class SrSa sr_sa.h
    \brief scene action base class

    Defines a base class for applying actions to the scene graph */
class SrSa
 { protected :
    SrArray<SrMat> _matrix_stack;

   protected :

    /*! Constructor . An id matrix is pushed into the matrix stack. */
    SrSa ();

    /*! Virtual destructor. */
    virtual ~SrSa ();

   public :

    /*! Makes the node to start applying the action. If the action is not
        applied to the entire scene, false is returned.
        If init is true (default), init_matrix(Id) is called. */
    bool apply ( SrSn* n, bool init=true );

   protected : // virtual methods

    /*! Put in mat the current top matrix */
    virtual void get_top_matrix ( SrMat& mat );

    /*! returns the size of the matrix stack */
    virtual int matrix_stack_size ();

    /*! Sets the matrix stack size to 1 and put the identity mat in the top position. */
    virtual void init_matrix ();

    /*! Called right after the apply method is called */
    virtual void apply_begin () {}

    /*! Called when the apply method is finished */
    virtual void apply_end () {}

    /*! Multiply mat to the topmost matrix in the matrix stack. */
    virtual void mult_matrix ( const SrMat& mat );

    /*! Take the current topmost matrix in the matrix stack, and push a copy
        into a new matrix position in the stack. */
    virtual void push_matrix ();

    /*! Pop one matrix from the matrix stack. */
    virtual void pop_matrix ();

    /*! Simply calls multi_matrix(), if m->visible() state is true. 
        False can be returned to stop the traverse, otherwise true must be returned. */
    virtual bool matrix_apply ( SrSnMatrix* m );

    /*! Apply the action to each group child, pushing/popping the current
        matrix if the group is acting as a separator.
        False can be returned to stop the traverse, otherwise true must be returned. */
    virtual bool group_apply ( SrSnGroup* g );

    /*! Apply the action to a shape, SrSa implementation simply returns true.
        False can be returned to stop the traverse, otherwise true must be returned. */
    virtual bool shape_apply ( SrSnShapeBase* s );

    /*! Apply the action to the child and helpers, pushing/popping the current
        matrix. If the manipulator is not visible, only the child receives the action.
        False can be returned to stop the traverse, otherwise true must be returned. */
    virtual bool editor_apply ( SrSnEditor* e );
 };

//================================ End of File =================================================

# endif  // SR_SA_H

