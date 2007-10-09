
# ifndef SR_OUTPUT_WINDOW_H
# define SR_OUTPUT_WINDOW_H

/** \file sr_output_window.h 
 * Manages a fltk output window. */

class SrOutput;
class Fl_Window;
class Fl_Browser;

/*! \class SrOutputWindow sr_output_window.h
    \brief Fltk output window

    This class manages a fltk output window, that can capture and
    display any data sent to a SrOutput. */
class SrOutputWindow
 { private :
    Fl_Window* _window;
    Fl_Browser* _browser;
    char* _buffer;
    int _bufcur;
    int _bufcap;
    bool _activated;
    bool _already_shown;
    void _init ();
    int _buffer_len;

   public :

    /*! Default constructor */ 
    SrOutputWindow ();

    /*! Constructor which connects the window to the given output o */ 
    SrOutputWindow ( SrOutput& o );

    /*! Destructor */
   ~SrOutputWindow ();

    /*! Change the number of lines kept in the window, default is 512 */
    void buffer_len ( int len ) { _buffer_len=len; }

    /*! Activate or deactivate the output window. If the window is
        deactivated, all messages to be displayed are not displayed,
        and are simply ignored. */
    void activated ( bool b ) { _activated=b; }

    /*! Returns the activation state of the window. */
    bool activated () { return _activated; }

    /* Writes the content of the output window to a file. */
    void save ( const char* fname );

    /*! Display text in the output window */
    void output ( const char *s );

    /*! The window will automatically open with the first output, but
        it can be also showed using this function */
    void show ();

    /*! Returns true if the window is shown, and false otherwise */
    bool shown ();

    /* Will hide the window */
    void hide ();

    /* Clears all the window content */
    void clear ();

    /* Makes sure the first line is visible */
    void show_first_line ();

    /* Change the title of the window */
    void title ( const char* t );

    /*! Will reinitialize o to output to an internal function which sends the
        text output to SrOutputWindow. To disconnect, o should be initialized again */
    void connect ( SrOutput& o );

    /*! Will forever enter the fltk main loop, which will manage the window. */
    static void run ();

    /*! Updates all fltk widgets. */
    static void check ();
 };

//================================ End of File =================================================

# endif // SR_OUTPUT_WINDOW_H

