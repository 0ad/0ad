
# ifndef SR_MATRIX_WINDOW_H
# define SR_MATRIX_WINDOW_H

/** \file sr_matrix_window.h 
 * Fltk window for matrix edition. */

class Fl_Window;
class Fl_Browser;
class SrMat;
class SrMWin;

/*! \class SrMatrixWindow sr_matrix_window.h
    \brief Fltk window for matrix edition. */
class SrMatrixWindow
 { private :
    SrMWin* _ui;
    bool _ok_pressed;
   public :

    /*! Constructor */ 
    SrMatrixWindow ();

    /*! Destructor */
   ~SrMatrixWindow ();

    /*! Set window title */
    void title ( const char* s );

    Fl_Window* window ();

    /*! Put a new matrix in the window. Method matrix_changed() is called. */
    void mat ( const SrMat& m );

    const SrMat& mat () const;

    bool ok_was_pressed () const { return _ok_pressed; }

    /* Called when the ok button is pressed, and here it closes the
       window and set _ok_pressed to true; */
    virtual void ok_button ( const SrMat& m );

    /* Called when the cancel button is pressed, and here it closes the
       window and set _ok_pressed to false; */
    virtual void cancel_button ();

    /* Called each time the matrix is changed */
    virtual void matrix_changed ( const SrMat& m ) {};
 };

//================================ End of File =================================================

# endif // SR_MATRIX_WINDOW_H

