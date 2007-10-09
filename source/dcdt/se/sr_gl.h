
# ifndef SR_GL_H
# define SR_GL_H

/** \file sr_gl.h 
 * Sr wrapper and extensions for OpenGL
 *
 * Overload of most used OpenGL functions to directly work with SR types,
 * and some extra utilities.
 */

# include <SR/sr.h>

# ifdef SR_TARGET_WINDOWS // defined in sr.h
# include <Windows.h>
# endif

# include <GL/gl.h>

class SrVec;
class SrMat;
class SrQuat;
class SrColor;
class SrLight;
class SrImage;
class SrOutput;
class SrMaterial;

//======================================= geometry ================================

void glNormal ( const SrVec &v );

void glVertex ( const SrVec &v );
void glVertex ( const SrVec &v1, const SrVec &v2 );
void glVertex ( const SrVec &v1, const SrVec &v2, const SrVec &v3 );
void glVertex ( const SrVec &v1, const SrVec &v2, const SrVec &v3, const SrVec &v4 );
void glVertex ( float x, float y, float z );
void glVertex ( float x, float y, float z, float a, float b, float c );
void glVertex ( float x, float y );
void glDrawBox ( const SrVec& a, const SrVec& b ); //!< Send quads with normals forming the box

//====================================== appearance ================================

void glClearColor ( const SrColor& c );
void glColor ( const SrColor& c );
void glLight ( int id, const SrLight& l ); //!< id = x E {0,...,7}, from GL_LIGHTx
void glMaterial ( const SrMaterial &m ); //!< Sets material for GL_FRONT_AND_BACK
void glMaterialFront ( const SrMaterial &m );
void glMaterialBack ( const SrMaterial &m );

//==================================== matrices ==============================

void glMultMatrix ( const SrMat &m );
void glLoadMatrix ( const SrMat &m );
void glTranslate ( const SrVec &v );
void glScale ( float s );
void glRotate ( const SrQuat &q );
void glLookAt ( const SrVec &eye, const SrVec &center, const SrVec &up );
void glPerspective ( float fovy, float aspect, float znear, float zfar );
void glGetViewMatrix ( SrMat &m );
void glGetProjectionMatrix ( SrMat &m );

//==================================== extras ==============================

void glSnapshot ( SrImage& img );

//==================================== info ==============================

void glPrintInfo ( SrOutput &o );

//================================ End of File ==================================

# endif // SR_GL_H
