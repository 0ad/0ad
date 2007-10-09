
# ifndef SR_SCENE_MANIPULATOR_H
# define SR_SCENE_MANIPULATOR_H

/** \file sr_scene.h 
 * basic scene elements
 */

# include "sr_quat.h"
# include "sr_plane.h"
# include "sr_sn_editor.h"
# include "sr_sn_shape.h"
# include "sr_polygons.h"

class SrBox;
class SrLines;
class SrSphere;

//==================================== SrSnManipulator ====================================

/*! \class SrSnManipulator sr_sn_manipulator.h
    \brief edit polygons

    SrSnManipulator puts a bounding box around the child object and lets
    the user manipulate the box, updating the transformation matrix
    accordingly. */
class SrSnManipulator : public SrSnEditor
 { public :
    enum Mode { ModeWaiting, ModeTranslating, ModeRotating };
    static const char* class_name;

   private :
    Mode _mode;
    SrSnBox* _box;
    SrSnLines* _dragsel;
    SrPnt _firstp;      // 1st pt selected (center of the blue cross)
    int _corner;
    SrPnt _bside[4];
    SrPlane _plane;
    SrMat _initmat;
    SrQuat _rotation;   // local rot after _initmat
    SrVec _translation; // local transl after _initmat
    float _rx, _ry, _rz;
    SrPnt _center;
    float _radius;
    float _precision_in_pixels;
    void (*_user_cb) ( SrSnManipulator*, const SrEvent&, void* );
    void* _user_cb_data;
    bool  _translationray;

   protected :
    /*! Destructor only accessible through unref() */
    virtual ~SrSnManipulator ();

   public :
    /*! Constructor */
    SrSnManipulator ();

    /*! Defines the child node */
    void child ( SrSn* sn );

    /*! Retrieves the child node */
    SrSn* child () const { return SrSnEditor::child(); }

    /*! Get a reference to the manipulator matrix. */
    SrMat& mat () { return SrSnEditor::mat(); }

    /*! Set a initial matrix to the manipulator. This matrix will be saved
        and all manipulations performed will be combined to it. 
        Note: the final transformation is obtained with mat() */
    void initial_mat ( const SrMat& m );

    /*! Put into waiting mode, ie, no selections appearing */
    void init ();

    /*! To be called to update the manipulation box */
    void update ();

    /*! Set a user callback that is called each time an event is processed
        by the manipulator */
    void callback ( void(*cb)(SrSnManipulator*,const SrEvent&,void*), void* udata )
      { _user_cb=cb; _user_cb_data=udata; }

    /*! Returns the associated callback data */
    void* user_cb_data () const { return _user_cb_data; }

    /*! Set the translation DOFs */
    void translation ( const SrVec& t );

    /*! Returns the translation DOFs */
    SrVec translation () const { return _translation; }

    /*! Handles mouse drag events, and few keys:
        qawsed = rotation around xyz axis (shift and alt change the step)
        esc: put back the original location
        x: switch to/from the "translation ray" mode
        p: print the current global matrix of the manipulator */
    virtual int handle_event ( const SrEvent &e );

   private :
    void _transform ( const SrPnt& p, const SrVec& r );
    void _set_drag_selection ( const SrPnt& p );
 };

//================================ End of File =================================================

# endif // SR_SN_MANIPULATOR_H


