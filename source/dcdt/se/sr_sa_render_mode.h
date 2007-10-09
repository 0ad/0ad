
# ifndef SR_SA_RENDER_MODE_H
# define SR_SA_RENDER_MODE_H

/** \file sr_sa_render_mode.h 
 * changes the render mode
 */

# include "sr_sa.h"
# include "sr_sn_shape.h"

/*! \class SrSaRenderMode sr_sa_render_mode.h
    \brief changes the render mode

    changes the render mode of all nodes visited by the action */
class SrSaRenderMode : public SrSa
 { private :
    bool _override;
    srRenderMode _render_mode;

   public :

    /*! Constructor that initializes the action to override the render mode to m */
    SrSaRenderMode ( srRenderMode m ) { set_mode(m); }

    /*! Constructor that initializes the action to restore the original render mode */
    SrSaRenderMode () { _render_mode=srRenderModeSmooth; _override=false; }

    /*! Set the mode m to be overriden */
    void set_mode ( srRenderMode m ) { _render_mode=m; _override=true; }

    /*! Set the action to restore the original render mode */
    void restore_mode () { _override=false; }

   private : // virtual methods
    virtual void mult_matrix ( const SrMat& mat ) {}
    virtual void push_matrix () {}
    virtual void pop_matrix () {}
    virtual bool shape_apply ( SrSnShapeBase* s );
    virtual bool matrix_apply ( SrSnMatrix* m ) { return true; }
 };

//================================ End of File =================================================

# endif  // SR_SA_RENDER_MODE_H

