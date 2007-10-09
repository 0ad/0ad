
# ifndef SR_VAR_TABLE_H
# define SR_VAR_TABLE_H

/** \file sr_var_table.h 
 * a table of SrVars
 */

# include "sr.h"
# include "sr_var.h"
# include "sr_shared_class.h"

/*! \class SrVarTable sr_var_table.h
    \brief a table of generic type variables

    SrVarTable has methods to manage a table of variables,
    which is always kept sorted by name. */
class SrVarTable : public SrSharedClass
 { private :
    SrArray<SrVar*> _table;
    char* _name;
   public :    

    /*! Constructor */
    SrVarTable () { _name=0; }

    /*! Copy constructor */
    SrVarTable ( const SrVarTable& vt );

    /*! Destructor */
    ~SrVarTable ();

    /*! Sets a name for the var table. Null can be passed to clear the name. */
    void name ( const char* n );

    /*! Returns the associated name. "" is returned if no name was set. */
    const char* name () const;

    /*! Clear the table */
    void init ();

    /*! Compress free spaces */
    void compress () { _table.compress(); }

    /*! Returns size */
    int size () const { return _table.size(); }

    /*! Get a const reference to a var from an index, which must be in valid range */
    const SrVar& const_get ( int i ) const { return *_table[i]; }

    /*! Get a reference to a var from an index, which must be in valid range */
    SrVar& get ( int i ) const { return *_table[i]; }

    /* Does a binary search for the name, and returns a pointer to the found var.
       If not found, null is returned. */
    SrVar* get ( const char* name ) const;

    /* Does a binary search for the name, and returns the value if the name was
       found. If not found, 0 or "" is returned. */
    int geti ( const char* name ) const { SrVar* v=get(name); return v? v->geti():0; }
    bool getb ( const char* name ) const { SrVar* v=get(name); return v? v->getb():0; }
    float getf ( const char* name ) const { SrVar* v=get(name); return v? v->getf():0; }
    const char* gets ( const char* name ) const { SrVar* v=get(name); return v? v->gets():""; }

    /* Does a binary search for the name and set the value if the var is found.
       Returns a pointer to the found var or null if no var was found.
       Automatic type casts are performed if required. */
    SrVar* set ( const char* name, int value, int index=0 );
    SrVar* set ( const char* name, bool value, int index=0 );
    SrVar* set ( const char* name, float value, int index=0 );
    SrVar* set ( const char* name, const char* value, int index=0 );
    SrVar* set ( const char* name, double value, int index=0 )
           { return set(name,float(value),index); }

    /*! Access operator. The index must be in a valid range */
    SrVar& operator [] (int i ) const { return get(i); }

    /* Does a binary search and returns the index of the
       found variable. Returns -1 if not found. */
    int search ( const char* name ) const;

    /*! Insert v in the table. Variable v should be created with operator
        new, and will be managed by the table after insertion.
        Returns the allocated position of v */
    int insert ( SrVar* v );

    /* Allocated a new SrVar and insert it to the table with method insert(). */
    int insert ( const char* name, int value ) { return insert(new SrVar(name,value)); }
    int insert ( const char* name, bool value ) { return insert(new SrVar(name,value)); }
    int insert ( const char* name, float value ) { return insert(new SrVar(name,value)); }
    int insert ( const char* name, double value ) { return insert(new SrVar(name,float(value))); }
    int insert ( const char* name, const char* value ) { return insert(new SrVar(name,value)); }

    /*! Remove variable with index i. If i is out of range nothing is done */
    void remove ( int i );

    /*! Take the vars of vt. If duplicated names are found, the values found
        in vt takes place. Var table vt becomes empty. */
    void merge ( SrVarTable& vt ); 

    /*! Extract the variable with index i from the table. The user becomes
        responsible of managing the deletion of the returned pointer.
        If i is out of range, 0 is returned. */
    SrVar* extract ( int i );

    /*! Output the SrVarTable (between delimiters [ ... ]) */
    friend SrOutput& operator<< ( SrOutput& o, const SrVarTable& v );

    /*! Input a SrVarTable */
    friend SrInput& operator>> ( SrInput& in, SrVarTable& v );
 };

//================================ End of File =================================================

# endif  // SR_VAR_TABLE_H

