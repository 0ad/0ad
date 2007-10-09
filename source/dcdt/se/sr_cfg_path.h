
# ifndef SR_CFG_PATH_H
# define SR_CFG_PATH_H

# include "sr_output.h"
# include "sr_array.h"
//# include "sr_cfg_manager.h"
# include "sr_cfg_manager.h"

/*! SrCfgPathBase manages a sequence of configurations, describing
    a path in configuration space. */
class SrCfgPathBase
 { private :
    SrArray<srcfg*> _buffer;
    SrCfgManagerBase* _cman;
    int _size;
    int _interp_start;
    float _interp_startdist;
    float _sprec, _slen;
    int _sbads;
    float _slastangmax;

   public :
    /*! Constructor requires a pointer to the configuration manager */
    SrCfgPathBase ( SrCfgManagerBase* cman );

    /*! Copy constructor sharing the configuration manager */
    SrCfgPathBase ( const SrCfgPathBase& p );

    /*! Destructor */
   ~SrCfgPathBase ();

    /*! Makes the path become empty */
    void init ();

    /*! Deletes all non-used internal buffers */
    void compress ();

    /*! Appends a configuration at the end of the path.
        Even if c is null, a new valid configuration is pushed. */
    void push ( const srcfg* c );

    /*! Removes the last configuration in the path */
    void pop ();

    /*! Removes the position i, which must be a valid position */
    void remove ( int i );
    
    /*! Removes dp positions at position i (parameters must be valid) */
    void remove ( int i, int dp );

    /*! Inserts a configuraton at position i; if i>=size() a push() is done. */
    void insert ( int i, const srcfg* c );

    /*! Inserts a copy of path p at position i; if i>=size() p is appended. */
    void insert_path ( int i, const SrCfgPathBase& p );

    /*! Appends p to the path; p will become an empty path after this call */
    void append_path ( SrCfgPathBase& p );

    /*! Swap the positions of the two nodes */
    void swap ( int i, int j );

    /*! Reverts the order of the nodes in the path */
    void revert ();

    /*! pushes or pops entries until reaching size s */
    void size ( int s );

    /*! Returns the number of configurations in the path */
    int size () const { return _size; }

    /*! Returns the length of the path from node i1 to node i2.
        The length is calculated by adding the distances
        of adjacent configurations in the path. */
    float len ( int i1, int i2 ) const;

    /*! Returns the length of the full path. */
    float len () const { return len(0,_size-1); }

    /*! Returns in c the interpolated configuration in the path according to parameter t,
        which must be in the closed interval [0,len]. */
    void interp ( float t, srcfg* c );
    
    /*! Returns in c the interpolated configuration in the path according to the time
        parameter t, which must be a valid time between the times associated with the
        first and last nodes of the path. The configuration manager method time() is
        used to retrieve the time associated with each configuration. */
    void temporal_interp ( float t, srcfg* c );

    /*! The smooth routine takes two random configurations interpolated along the path
        and replaces the portion between them by a direct interpolation if no collisions
        appear (up to precision prec). Parameter len should contain the current lenght of
        the path. A <0 value can be given if the length is not known in advance. In any
        case, after the smooth, len will contain the updated path length. */
    void smooth_random ( float prec, float& len );

    /*! tries to replace the subpath(t1,t2) by a "straight interpolation".
        Returns: updated len, 0:same edge, -1:collision, 1:done */
    int linearize ( float prec, float& len, float t1, float t2 );

    /*! Make one pass in all nodes, trying first to smooth nodes closer to the first and
        last nodes. Usefull because most often several nodes are created near the root
        of the two expanding trees. Usually is called once before the random smooths. */
    void smooth_ends ( float prec );

    void smooth_init ( float prec );
    bool smooth_step ();

    /*! Copy operator */
    void operator= ( const SrCfgPathBase& p ) { init(); insert_path(0,p); }

    /*! Returns the configuration of the last node in the path */
    srcfg* top () { return _buffer[_size-1]; }

    /*! Returns configuration index i */
    srcfg* get ( int i ) { return _buffer[i]; }
    
    /*! Const version of get() */
    const srcfg* const_get ( int i ) const { return _buffer.const_get(i); }
    
    /*! Outputs the path nodes for inspection */
    friend SrOutput& operator<< ( SrOutput& o, const SrCfgPathBase& p );
    
    private:
    float _diff ( int i, float prec );
 };

/*! This template provides automatic type casts for the user-defined
    configuration class C. */
template <class C>
class SrCfgPath : public SrCfgPathBase
 { public :
    SrCfgPath ( SrCfgManagerBase* cman ) : SrCfgPathBase(cman) {}
    SrCfgPath ( const SrCfgPath& p ) : SrCfgPathBase(p) {}

    C* operator[] ( int i ) { return get(i); }
    C* top () { return (C*)SrCfgPathBase::top(); }
    C* get ( int i ) { return (C*)SrCfgPathBase::get(i); }
    const C* const_get ( int i ) const { return (const C*)SrCfgPathBase::const_get(i); }
    void operator= ( const SrCfgPath<C>& p ) { SrCfgPathBase::init(); SrCfgPathBase::insert_path(0,p); }
 };


//================================ End of File =================================================

# endif  // SR_PATH_H

