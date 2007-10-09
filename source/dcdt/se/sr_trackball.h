
# ifndef SR_TRACKBALL_H
# define SR_TRACKBALL_H

/** \file sr_trackball.h 
 * trackball manipulation
 */

# include "sr_vec.h"
# include "sr_quat.h"

class SrMat;

/*! \class SrTrackball sr_trackball.h
    \brief trackball manipulation

    SrTrackball maintains a rotation with methods implementing a
    trackball-like manipulations, and etc. */
class SrTrackball
 { public :
    SrQuat rotation;    //!< current rotation
    SrQuat last_spin;

   public :
    
    /*! Initialize the trackball with the default parameters, see init(). */
    SrTrackball ();

    /*! Copy constructor. */
    SrTrackball ( const SrTrackball& t );

    /*! Set the parameters to their default values, which is a null rotation. */
    void init ();

    /*! Set m to be the equivalent transformation matrix. A reference to m
        is returned. */
    SrMat& get_mat ( SrMat& m ) const;

    /*! Gets the rotation induced by a mouse displacement,
        according to the trackball metaphor. Window coordinates must be
        normalized in [-1,1]x[-1,1]. */
    static void get_spin_from_mouse_motion ( float lwinx, float lwiny, float winx, float winy, SrQuat& spin );

    /*! Accumulates the rotation induced by a mouse displacement,
        according to the trackball metaphor. Window coordinates must be
        normalized in [-1,1]x[-1,1]. */
    void increment_from_mouse_motion ( float lwinx, float lwiny, float winx, float winy );

    /*! Accumulates the rotation with the given quaternion (left-multiplied) */
    void increment_rotation ( const SrQuat& spin );

    /*! Outputs trackball data for inspection. */
    friend SrOutput& operator<< ( SrOutput& out, const SrTrackball& tb );
 };

//================================ End of File =================================================

# endif // SR_TRACKBALL_H

