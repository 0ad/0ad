
# ifndef SR_VAR_TABLE_WINDOW_H
# define SR_VAR_TABLE_WINDOW_H

/** \file sr_var_table_window.h 
 * Fltk window for SrVarTable edition. */

class Fl_Window;
class Fl_Browser;
class SrVTWin;
class SrVarTable;

/*! \class SrVarTableWindow sr_var_table_window.h
    \brief Fltk window for SrVarTable edition. */
class SrVarTableWindow
 { private :
    SrVTWin* _ui;

   public :

    /*! Constructor */ 
    SrVarTableWindow ();

    /*! Destructor */
   ~SrVarTableWindow ();

    /*! Set window title */
    void title ( const char* s );

    /*! Access to the fltk window */
    Fl_Window* window ();

    /*! Calls window()->show() */
    void show ();

    /*! Start using the given vt, sharing it (vt can be null). */
    void use_var_table ( SrVarTable* vt );

    /*! Update all the user interface */
    void update ();

    /* Called when the window closes */
    virtual void window_closed ();

    /* Called each time a var (of index i) is changed */
    virtual void value_modified ( int i ) {};
 };

//================================ End of File =================================================

# endif // SR_VAR_TABLE_WINDOW_H

