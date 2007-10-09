
# ifndef SR_MAT_H
# define SR_MAT_H

/** \file sr_mat.h 
 * A 4x4 matrix */

# include "sr_vec.h"

/*! \class SrMat sr_mat.h
    \brief A 4x4 matrix

    The transformation methods consider that the matrix will be 
    applied to a line vector at the left side of the matrix. So 
    that in all matrix transformations M should multiply v as vM. 
    When using the matrix as a geometric transformation, the
    first three elements of the fourth line represent the 
    translation vector. This is to follow the OpenGL design. */
class SrMat
 { private :
    float e[16];
   public :
    static const SrMat null; //!< A null matrix
    static const SrMat id;   //!< An identity matrix
    enum InitializeMode { NotInitialized };
   public :

    /*! Default constructor initializes SrMat as identity. */
    SrMat () { set(id.e); }

    /*! Copy constructor. */
    SrMat ( const SrMat &m ) { set(m.e); }

    /*! Constructor without initialization. SrMat values are simply not
        initialized when declaring SrMat m(SrMat::NotInitialized) */
    SrMat ( InitializeMode /*m*/ ) {}

    /*! Constructor from a float[16] pointer. */
    SrMat ( const float *p );

    /*! Constructor from a double[16] pointer. */
    SrMat ( const double *p );

    /*! Constructor from 16 values. */
    SrMat ( float a, float b, float c, float d, float e, float f, float g, float h, 
            float i, float j, float k, float l, float m, float n, float o, float p );

    /*! Copy operator. */
    void operator= ( const SrMat& m ) { set(m.e); }

    /*! float pointer type cast operator */
    operator float*() { return &e[0]; }

    /*! const float pointer type cast operator */
    operator const float*() const { return &e[0]; }

    /*! Returns a float pointer to the SrMat element with given index. */
    float* pt ( int i ) { return &e[i]; }

    /*! Returns a const float pointer to the SrMat element with given index. */
    const float* pt_const ( int i ) const { return &e[i]; }

    /*! Permits access to an element of the matrix as a vector of 16 elements.
        The given index must be from 0 to 15, no chekings are done.
        Implemented inline. */
    float& operator[] ( int i ) { return e[i]; }
    
    /*! const version of [] operator */
    float operator[] ( int i ) const { return e[i]; }

    /*! const version of operator[], implemented inline */
    float get ( int i ) const { return e[i]; }

    /*! set element i in [0,16], implemented inline */
    void set ( int i, float v ) { e[i]=v; }

    /*! Permits access to an element of the matrix using a line and column
        indices. The given indices can be from 0 to 3. No indices chekings
        are done. Implemented inline. */
    float& operator() ( int i, int j ) { return e[i*4+j]; }

    /*! const version of operator(), implemented inline */
    float get ( int i, int j ) const { return e[i*4+j]; }

    float& e11 () { return e[0]; } //!< Returns a reference to the element [0]
    float& e12 () { return e[1]; } //!< Returns a reference to the element [1]
    float& e13 () { return e[2]; } //!< Returns a reference to the element [2]
    float& e14 () { return e[3]; } //!< Returns a reference to the element [3]
    float& e21 () { return e[4]; } //!< Returns a reference to the element [4]
    float& e22 () { return e[5]; } //!< Returns a reference to the element [5]
    float& e23 () { return e[6]; } //!< Returns a reference to the element [6]
    float& e24 () { return e[7]; } //!< Returns a reference to the element [7]
    float& e31 () { return e[8]; } //!< Returns a reference to the element [8]
    float& e32 () { return e[9]; } //!< Returns a reference to the element [9]
    float& e33 () { return e[10]; } //!< Returns a reference to the element [10]
    float& e34 () { return e[11]; } //!< Returns a reference to the element [11]
    float& e41 () { return e[12]; } //!< Returns a reference to the element [12]
    float& e42 () { return e[13]; } //!< Returns a reference to the element [13]
    float& e43 () { return e[14]; } //!< Returns a reference to the element [14]
    float& e44 () { return e[15]; } //!< Returns a reference to the element [15]

    /*! Sets all elements of SrMat from the given float[16] pointer. */
    void set ( const float *p );

    /*! Sets all elements of SrMat from the given double[16] pointer. */
    void set ( const double *p );

    /*! Sets the four elements of line 1. Implemented inline. */
    void setl1 ( float x, float y, float z, float w ) { e[0]=x; e[1]=y; e[2]=z; e[3]=w; }
    
    /*! Sets the four elements of line 2. Implemented inline. */
    void setl2 ( float x, float y, float z, float w ) { e[4]=x; e[5]=y; e[6]=z; e[7]=w; }
    
    /*! Sets the four elements of line 3. Implemented inline. */
    void setl3 ( float x, float y, float z, float w ) { e[8]=x; e[9]=y; e[10]=z; e[11]=w; }
    
    /*! Sets the four elements of line 4. Implemented inline. */
    void setl4 ( float x, float y, float z, float w ) { e[12]=x; e[13]=y; e[14]=z; e[15]=w; }

    /*! Sets the first three elements of line 1. Implemented inline. */
    void setl1 ( const SrVec &v ) { e[0]=v.x; e[1]=v.y; e[2]=v.z; }
    
    /*! Sets the first three elements of line 2. Implemented inline. */
    void setl2 ( const SrVec &v ) { e[4]=v.x; e[5]=v.y; e[6]=v.z; }
    
    /*! Sets the first three elements of line 3. Implemented inline. */
    void setl3 ( const SrVec &v ) { e[8]=v.x; e[9]=v.y; e[10]=v.z; }
    
    /*! Sets the first three elements of line 4. Implemented inline. */
    void setl4 ( const SrVec &v ) { e[12]=v.x; e[13]=v.y; e[14]=v.z; }

    /*! Returns true if all elements are equal to 0.0, false otherwise. */
    bool isnull () const { return *this==null; }

    /*! Returns true if the matrix is identical to SrMat::id, false otherwise. */
    bool isid () const { return *this==id;   }

    /*! Makes SrMatrix be a null matrix. */
    void zero () { *this=null; }

    /*! Make elements in interval [-epsilon,epsilon] to become 0 */
    void round ( float epsilon );

    /*! Makes SrMatrix be an identity matrix. */
    void identity () { *this=id; }

    /*! Transpose SrMatrix. */
    void transpose ();

    /*! Transpose the 3x3 sub matrix. */
    void transpose3x3 ();

    /*! Makes SrMat be a translation transformation, that can be applied to a vector x as xM. */
    void translation ( float tx, float ty, float tz );

    /*! Makes SrMat be a translation transformation, that can be applied to a vector x as xM. */
    void translation ( const SrVec &v ) { translation(v.x,v.y,v.z); }

    /*! Pre-multiplies SrMat with a translation matrix constructed with the 
        vector v. The multiplication is optimized considering that SrMat has
        values only on its 3x3 sub matrix. */
    void left_combine_translation ( const SrVec &v );

    /*! Pos-multiplies SrMat with a translation matrix constructed with the
        vector v. Here the first three values of the 4th line are added by v. */
    void right_combine_translation ( const SrVec &v );

    /*! Combines in SrMat a scale factor of sx, sy and sz on each coordinate. */
    void combine_scale ( float sx, float sy, float sz );

    /*! Combines in SrMat a global scale factor of s. */
    void combine_scale ( float s ) { combine_scale(s,s,s); }

    /*! Makes SrMat be a scale transformation, that can be applied to a vector x as xM. */
    void scale ( float sx, float sy, float sz );

    /*! Makes SrMat be a scale transformation, that can be applied to a vector x as xM. */
    void scale ( float s ) { scale(s,s,s); }

    /*! Makes SrMat be a rotation transformation around x axis. The given parameters 
        are the sine and cosine of the desired angle. The transformation can then be 
        applied to a vector x as xM. */
    void rotx ( float sa, float ca );

    /*! Makes SrMat be a rotation transformation around y axis. The given parameters 
        are the sine and cosine of the desired angle. The transformation can then be 
        applied to a vector x as xM. */
    void roty ( float sa, float ca );

    /*! Makes SrMat be a rotation transformation around z axis. The given parameters 
        are the sine and cosine of the desired angle. The transformation can then be 
        applied to a vector x as xM. */
    void rotz ( float sa, float ca );

    /*! Makes SrMat be a rotation transformation around x of the given angle in radians. */
    void rotx ( float radians );

    /*! Makes SrMat be a rotation transformation around y of the given angle in radians. */
    void roty ( float radians );

    /*! Makes SrMat be a rotation transformation around z of the given angle in radians. */
    void rotz ( float radians );

    /*! Rotation around an axis. The transformation is obtained with vM  (Mesa version). */
    void rot ( const SrVec &vec, float sa, float ca );

    /*! Rotation around an axis given an angle in radians. */
    void rot ( const SrVec &vec, float radians );

    /*! Gives the rotation matrix that rotates one vector to another. */
    void rot ( const SrVec& from, const SrVec& to );

    /*! Sets SrMat to be the rigid transformation matrix that maps the three
        given vertices into the plane xy. The transformation can then be applied
        to each vertex with vM. After transformed, v1 will go to the origin,
        and v2 will lye in the X axis. The most time consuming operations are 
        3 square roots and 3 matrix multiplication. */
    void projxy ( SrVec v1, SrVec v2, SrVec v3 );

    /*! Sets SrMat to the OpenGL glut-like camera transformation. Note: fovy
        parameter must be set in radians. */
    void perspective ( float fovy, float aspect, float znear, float zfar );

    /*! Sets SrMat to the OpenGL gluLookAt like camera transformation. */
    void look_at ( const SrVec& eye, const SrVec& center, const SrVec& up );

    /*! Fast invertion by direct calculation, no loops, no gauss, no pivot searching, 
        but with more numerical errors. The result is put in the given SrMat parameter. */
    void inverse ( SrMat& inv ) const;

    /*! Fast invertion by direct calculation, no loops, no gauss, no pivot searching, 
        but with more numerical errors. The result is a new matrix returned by value. */
    SrMat inverse () const;

    /*! Makes SrMat to be its inverse, calling the inverse() method. */
    void invert () { *this=inverse(); }

    /*! Fast 4x4 determinant by direct calculation, no loops, no gauss. */
    float det () const;

    /*! Fast 3x3 determinant by direct calculation, no loops, no gauss. */
    float det3x3 () const;

    /*! Considers the matrix as a 16-dimensional vector and returns its norm raised
        to the power of two. */
    float norm2 () const;

    /*! Considers the matrix as a 16-dimensional vector and returns its norm. */
    float norm () const ;

    /*! Returns true if dist2(*this,m)<=ds*ds, and false otherwise. Implemented inline. */
    bool isnext ( const SrMat &m, float ds ) { return dist2(*this,m)<=ds*ds? true:false; }

    /*! Sets SrMat as the multiplication of m1 with m2. This method needs
        to use a temporary matrix to perform the calculations if m1 or m2
        is equal to to SrMat */
    void mult ( const SrMat &m1, const SrMat &m2 );

    /*! Sets SrMat to be the addition of m1 with m2. */
    void add ( const SrMat &m1, const SrMat &m2 );

    /*! Sets SrMat to be the difference m1-m2. */
    void sub ( const SrMat &m1, const SrMat &m2 );

    /*! Distance between two matrices, considering them as a 16-dimensional vector. */
    friend float dist ( const SrMat &a, const SrMat &b );

    /*! Distance between two matrices raised to two, considering them as a 16-dimensional vector. */
    friend float dist2 ( const SrMat &a, const SrMat &b );

    /*! Operator to multiply SrMat by a scalar. */
    void operator *= ( float r );

    /*! Operator to (right) multiply SrMat by another SrMat m. */
    void operator *= ( const SrMat &m );

    /*! Operator to add to SrMat another SrMat. */
    void operator += ( const SrMat &m );

    /*! Operator to multiply a SrMat to a scalar, returning another SrMat. */
    friend SrMat operator * ( const SrMat &m, float r );

    /*! Operator to multiply a scalar to a SrMat, returning another SrMat. */
    friend SrMat operator * ( float r, const SrMat &m );

    /*! Operator to multiply a SrMat to a SrVec, returning another SrMat. */
    friend SrVec operator * ( const SrMat &m,  const SrVec &v  );

    /*! Operator to multiply a SrVec to a SrMat, returning another SrMat. */
    friend SrVec operator * ( const SrVec &v,  const SrMat &m  );

    /*! Operator to multiply two SrMat, returning another SrMat. */
    friend SrMat operator * ( const SrMat &m1, const SrMat &m2 );

    /*! Operator to add two SrMat, returning another SrMat. */
    friend SrMat operator + ( const SrMat &m1, const SrMat &m2 );

    /*! Operator to compute the difference two SrMat, returning another SrMat. */
    friend SrMat operator - ( const SrMat &m1, const SrMat &m2 );

    /*! Comparison operator to check if two SrMat are equal. */
    friend bool operator == ( const SrMat &m1, const SrMat &m2 );

    /*! Comparison operator to check if two SrMat are different. */
    friend bool operator != ( const SrMat &m1, const SrMat &m2 );

    /*! Outputs 4 elements per line. */
    friend SrOutput& operator<< ( SrOutput& o, const SrMat& m );

    /*! Reads 16 float numbers from the input. */
    friend SrInput&  operator>> ( SrInput& in, SrMat& m );
  };

//============================== end of file ===============================

# endif // SR_MAT_H
