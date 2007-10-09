
# ifndef SR_IMAGE_H
# define SR_IMAGE_H

/** \file sr_image.h 
 * A 24 bit image.*/

# include "sr_color.h"

//=========================== SrImage ============================

/*! \class SrImage sr_image.h
    \brief Non compressed 24 bit image
    SrImage stores pixel data as a sequence of rgb data. */
class SrImage
 { private :
    int _w, _h;    // total data size is 3*_w*_h
    int _tw;       // _tw = 3*_w
    srbyte* _data; // array of rgb values

   public :

    /*! Constructs an empty image */ 
    SrImage ();

    /*! Destructor */ 
   ~SrImage ();

    /*! Alloc the desired size in pixels of the image.
        A total of w*h*3 elements are allocated.
        Invalid dimensions deletes the image data */
    void alloc ( int w, int h );

    /*! Changes the image by appying a vertical mirroring */
    void vertical_mirror ();

    /*! Returns the width in pixels of the image */
    int w () const { return _w; }

    /*! Returns the height in pixels of the image */
    int h () const { return _h; }
 
    /*! Returns a pointer to the base image data */
    const srbyte* data () { return _data; }

    /*! Returns a pointer to the pixel color (3 bytes) at position (l,c) */
    srbyte* data ( int l, int c ) { return _data+(l*_tw)+(c*3); }

    /*! Returns a reference to the red component of the pixel at position (l,c) */
    srbyte& r ( int l, int c ) { return data(l,c)[0]; }

    /*! Returns a reference to the green component of the pixel at position (l,c) */
    srbyte& g ( int l, int c ) { return data(l,c)[1]; }

    /*! Returns a reference to the blue component of the pixel at position (l,c) */
    srbyte& b ( int l, int c ) { return data(l,c)[2]; }
 
    /*! Returns the base pointer of the line l of the image */
    srbyte* line ( int l ) { return _data+(l*_tw); }

    /*! Saves the image in a bmp file. Returns true if could write file and false otherwise. */
    bool save_as_bmp ( const char* filename );
 };

//============================= end of file ==========================

# endif // SR_IMAGE_H
