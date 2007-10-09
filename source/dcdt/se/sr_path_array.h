
# ifndef SR_PATH_ARRAY_H
# define SR_PATH_ARRAY_H

/** \file sr_path_array.h 
 * array of file paths */

# include "sr_string_array.h"

//============================== SrPathArray ===============================

/*! \class SrPathArray sr_path_array.h
    \brief array of file paths
    This class helps maintaining a list of file paths and
    opening input files according to the declared paths and
    absolute/relative path/filename cases. */
class SrPathArray : public SrStringArray
 { private:
    SrString _basedir; // the basedir of relative filenames (has to have a slash in the end)
    
   public:
    /*! Access to the base dir */
    SrString& basedir () { return _basedir; }
    
    /*! Set the basedir and ensures its validity, eg, adding a slash in the end if needed. */
    void basedir ( const char* bd )
     { _basedir.make_valid_path(bd); }
    
    /*! Set the basedir, extracting it from a filename and ensuring its validity */
    void basedir_from_filename ( const char* file );

    /*! Add a path to the array. No validity checks done. */
    void push ( const char* path ) { push_path(path); }

    /*! Tries to open an input file:
        If the given filename has an absolute path in it, the method simply
        tries to open it and returns right after. Otherwise:
        - First search in the paths stored in the array.
          If basedir exists (not null), relative paths in the array
          are made relative (concatenated) to basedir.
        - Second, try to open filename using only the basedir path (if given).
        - Finally tries to simply open filename.
        Path basedir, if given, has to be a valid path name (with a slash in the end).
        Returns true if the file could be open. In such case, the successfull full file
        name can be found in inp.filename(). False is returned in case of failure.
        Parameter mode is the fopen() mode: "rt", etc.  */
    bool open ( SrInput& inp, const char* filename ) { return open_file(inp,filename,"rt",_basedir); }

    /*! Checks if file exists by testing all declared paths using the rules in open().
        Returns false if the file was not found, otherwise file will contain the 
        succesfull file name and true is returned */ 
    bool adjust_path ( SrString& file );
 };

#endif // SR_PATH_ARRAY_H

//============================== end of file ===============================
