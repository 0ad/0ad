
/** \file sr_quat.h 
 * Quaternion for rotations */

# ifndef SR_QUAT_H
# define SR_QUAT_H

# include <math.h>
# include "sr_vec.h" 
# include "sr_mat.h"

/*! \class SrQuat sr_quat.h
    \brief Quaternion for rotations. 

    A quaternion is represented as a four dimensional vector. Here we keep
    one scalar float element w, and three floats for the imaginary vector.
    The quaternion is then: w + x i + y j + z k */
class SrQuat
 { public :
    float w; // scalar part
    float x, y, z; // imaginary vector part

   private :
    float norm2 () const { return w*w+x*x+y*y+z*z; }
    float norm () const  { return sqrtf(norm2()); } 
    SrQuat conjugate() const { return SrQuat(w,-x,-y,-z); }
    void operator *= ( float r ) { w*=r; x*=r; y*=r; z*=r; }
    SrQuat operator * ( float r ) const { return SrQuat(w*r,x*r,y*r,z*r); }
    SrQuat operator / ( float r ) const { return SrQuat(w/r,x/r,y/r,z/r); }
    SrQuat operator + ( const SrQuat &q ) const { return SrQuat(w+q.w,x+q.x,y+q.y,z+q.z); }
    float dot ( const SrQuat &q ) const { return w*q.w + x*q.x + y*q.y + z*q.z; }

   public :

    /*! A null rotation that represents a rotation around the axis (1,0,0)
        with angle 0, that generates the internal representation (1,0,0,0). */
    static const SrQuat null;

   public :

    /*! Initializes SrQuat as a null rotation. Implemented inline. */
    SrQuat () : w(1.0f), x(0), y(0), z(0) {}

    /*! Constructor from 4 floats (qw,qx,qy,qz). Implemented inline. */
    SrQuat ( float qw, float qx, float qy, float qz ) : w(qw), x(qx), y(qy), z(qz) {}

    /*! Constructor from 4 floats (w,x,y,z) from a float array. Implemented inline. */
    SrQuat ( const float* f ) : w(f[0]), x(f[1]), y(f[2]), z(f[3]) {}

    /*! Copy constructor. Implemented inline. */
    SrQuat ( const SrQuat& q ) : w(q.w), x(q.x), y(q.y), z(q.z) {}

    /*! Initializes SrQuat with the rotation around the given axis and angle in
        radians. The method set() is called inline in this constructor. */
    SrQuat ( const SrVec& axis, float radians ) { set(axis,radians); }

    /*! Initializes SrQuat with the given axis-angle rotation, where the vector
        is the rotation axis, and its norm is the angle of rotation.
        The method set() is called inline in this constructor. */
    SrQuat ( const SrVec& axisangle ) { set(axisangle); }

    /*! Initializes SrQuat with the rotation from v1 to v2.
         The method set() is called inline in this constructor. */
    SrQuat ( const SrVec& v1, const SrVec& v2 ) { set(v1,v2); }

    /*! Initializes SrQuat from a rotation matrix */
    SrQuat ( const SrMat& m ) { set(m); }

    /*! Set a random quaternion, using a uniform distribution method */
    void random ();
    
    /*! Set the four values */
    void set ( float qw, float qx, float qy, float qz ) { w=qw; x=qx; y=qy; z=qz; }

    /*! Set the four values from a float pointer */
    void set ( const float* f ) { w=f[0]; x=f[1]; y=f[2]; z=f[3]; }

    /*! Defines SrQuat as the rotation from v1 to v2. */
    void set ( const SrVec& v1, const SrVec& v2 );

    /*! Defines SrQuat as the rotation around axis of the given angle in radians.
        Axis is internaly normallized. */
    void set ( const SrVec& axis, float radians );

    /*! Initializes SrQuat with the given axis-angle rotation, where the given
        vector is the rotation axis, and its norm is the angle of rotation. */
    void set ( const SrVec& axisangle );

    /*! Defines SrQuat by extracting the rotation from the given rotation matrix. */
    void set ( const SrMat& m );

    /*! Gets the current axis and angle of rotation (in radians) that SrQuat defines. */
    void get ( SrVec& axis, float& radians ) const;

    /*! Returns the rotation axis. */
    SrVec axis () const;

    /*! Returns the angle in radians. */
    float angle () const;

    /*! Normalizes the quaternion and ensures w>=0 */
    void normalize ();

    /*! Returns the inverse quaternium. */
    SrQuat inverse() const;

    /*! Invert SrQuat. */
    void invert ();

    /*! Equivalent to get_mat(), but retrieves only the 3x3 portion
        of the transformation matrix. */
    void get_rot_mat ( float& a, float& b, float& c,
                       float& d, float& e, float& f,
                       float& g, float& h, float& i ) const;

    /*! Gets the equivalent transformation matrix. */
    SrMat& get_mat ( SrMat& m ) const;

    /*! Given a vector v and a quaternion q, the result of applying the rotation
       in q to v is returned (in mathematical notation this is q v q^-1) . */
    friend SrVec operator * ( const SrVec &v, const SrQuat &q );

    /*! rotation q1 followed by rotation q2 is equal to q2*q1 . */
    friend SrQuat operator * ( const SrQuat &q1, const SrQuat &q2 );

    /*! Comparison operator makes an exact comparison of the quaternion components. */
    friend bool operator == ( const SrQuat &q1, const SrQuat &q2 );

    /*! Comparison operator makes an exact comparison, of the quaternion components. */
    friend bool operator != ( const SrQuat &q1, const SrQuat &q2 );

    /*! Swaps the contents of q1 with q2. */
    friend void swap ( SrQuat &q1, SrQuat &q2 );

    /*! Returns the interpolation between q1 and q2 with parameter t.
        Parameter q1 is not const because it is normalized. */
    friend SrQuat slerp ( SrQuat &q1, const SrQuat &q2, float t );

    /*! Outputs data in "axis x y z ang a" format. */
    friend SrOutput& operator<< ( SrOutput& out, const SrQuat& q );

    /*! Input data from "axis x y z ang a" format. */
    friend SrInput& operator>> ( SrInput& in, SrQuat& q );
 };

//============================== end of file ===============================

# endif // SR_QUAT_H
