
/** \file sr_fl.h
 * fltk utilities integrated with sr
 */

# ifndef SR_FL_H
# define SR_FL_H

# include <FL/Fl_Gl_Window.H>
# include <SR/sr_color.h>
# include <SR/sr_string_array.h>
# include <SR/sr_mat.h>

/*! Fltk color chooser utility receiving a sr parameter SrColor */
bool fl_color_chooser ( const char *title, SrColor& c );

/*! Returns true if ok was pressed and false otherwise. Parameter value
    is displayed and returns the chosen value*/
bool fl_value_input ( const char* title, const char* label, float& value );

/*! Returns true if ok was pressed and false otherwise. Parameter s
    is displayed for edition and returns the entered string */
bool fl_string_input ( const char* title, const char* label, SrString& s );

/*! Uses an internal instance of SrOutputWindow to display a text.
    The internal instance is only allocated when used for the first time. */
void fl_text_output ( const char* title, const char* text );

/*! Opens a matrix edition window, returning true if ok was pressed,
    and false otherwise. If title is null, the window title is not changed. */
bool fl_matrix_input ( const char* title, SrMat& m );

/*! Scan the directory dir, and put all found directories and files in
    dirs and files. String array ext may contain file extensions to
    filter the read files (e.g "cpp" "txt", etc).
    String dir must finish with a slash '/' character.
    File names are returned in their full path names */
void fl_scan_dir ( const SrString& dir,
                   SrStringArray& dirs,
                   SrStringArray& files,
                   const SrStringArray& ext );

/*! Recursively scans and put in file all files starting at basedir.
    String array ext may contain file extensions to filter the read
    files; extensions are set without the "dot".
    File names are returned in their full path names */
void fl_scan_files ( const SrString& basedir,
                     SrStringArray& files, 
                     const SrStringArray& ext );

//================================ End of File =================================================

# endif // SR_VIEWER_H

