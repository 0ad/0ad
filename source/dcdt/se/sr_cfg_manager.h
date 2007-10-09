
# ifndef SR_CFG_MANAGER_H
# define SR_CFG_MANAGER_H

# include "sr_input.h"
# include "sr_output.h"
# include "sr_class_manager.h"

typedef void srcfg;

class SrCfgManagerBase : protected SrClassManagerBase
 { 
   public : // accesible members from SrSharedClass
 
     /*! Returns the current reference counter value. */
    int getref () const { return SrSharedClass::getref(); }

    /*! Increments the reference counter. */
    void ref () { SrSharedClass::ref(); }

    /*! Decrements the reference counter (if >0), and if the
        counter becomes 0, the class is automatically self deleted. */
    void unref() { SrSharedClass::unref(); }

   public : // virtual callbacks

    /*! Returns a new configuration */
    virtual srcfg* alloc ()=0;

    /*! Returns a new configuration as a copy of c */
    virtual srcfg* alloc ( const srcfg* c )=0;

    /*! Deletes a configuration */
    virtual void free ( srcfg* c )=0;

    /*! Copy the contents of c2 into c1. Note: c1 and c2 will be equal 
        in several calls, so a test if(c1==c2) should be included here. */
    virtual void copy ( srcfg* c1, const srcfg* c2 )=0;

    /*! Outputs a configuration */
    virtual void output ( SrOutput& o, const srcfg* c )=0;

    /*! Inputs a configuration */
    virtual void input ( SrInput& i, srcfg* c )=0;

    /*! Returns a random configuration, not necessarily valid. */
    virtual void random ( srcfg* c )=0;

    /*! Returns if the configuration is valid. */
    virtual bool valid ( const srcfg* c )=0;

    /*! Returns a distance between the two configurations. */
    virtual float dist ( const srcfg* c1, const srcfg* c2 )=0;

    /*! Returns the interpolated configuration c between c1 and c2,
        according to t in [0,1]. */
    virtual void interp ( const srcfg* c1, const srcfg* c2, float t, srcfg* c )=0;
    
    /*! Returns true if all nodes between the interpolation of ct0 and ct1 are 
    valid according to the given precision prec. Configuration ct is to be
    used as a temporary variable. Note: ct0 and ct1 are already valid.
    The default implementation for method visible performs recursive binary
    subdivision untill reaching precision prec. */
    virtual bool visible ( const srcfg* ct0, const srcfg* ct1, srcfg* ct, float prec );

    /*! Returns true if the time encoded in c1 is smaller than in c2. This will be
        only relevant to planning in time-varying conditions, if this is not the
        case, true must always be returned */
    virtual bool monotone ( const srcfg* c1, const srcfg* c2 )=0;
    
    /*! This is called just to notify that node child has been added
        as a child of node parent */
    virtual void child_added ( srcfg* parent, srcfg* child )=0;
    
    /*! The time() method returns the time associated with the given configuration c.
        This method is only called when solving a time-varying problem */
    virtual float time ( srcfg* c )=0;
 };

/*! Template SrCfgManager makes automatic type casts to user-defined classes.
    If needed in special cases, it can be further derived to rewrite/extend
    method SrCfgManagerBase::visible().
    Here is an example of implementations of a configuration class and a class manager that
    can be directly used with template SrCfgManager<MyCfg,MyManager> : \code
class MyCfg
 { public :
    MyCfg ();
    MyCfg ( const MyCfg& c );
   ~MyCfg ();
    void operator = ( const MyCfg& c );
    void random ();
    bool valid () const;
    friend SrOutput& operator<< ( SrOutput& out, const MyCfg& c );
    friend SrInput& operator>> ( SrInput& inp, MyCfg& c );
    friend float dist ( const MyCfg& c1, const MyCfg& c2 );
    friend float interp ( const MyCfg& c1, const MyCfg& c2, float t, MyCfg& c );
 };

class MyManager
 { public :
    MyCfg* alloc () { return new MyCfg; }
    MyCfg* alloc ( const MyCfg* c ) { return new MyCfg(*c); }
    void free ( MyCfg* c ) { delete c; }
    void copy ( MyCfg* c1, const MyCfg* c2 ) { *c1=*c2; }
    void output ( SrOutput& o, const MyCfg* c ) { o<<(*c); }
    void input ( SrInput& i, MyCfg* c ) { i>>(*c); }
    void random ( MyCfg* c ) { c->random(); }
    bool valid ( const MyCfg* c ) { return c->valid(); }
    float dist ( const MyCfg* c1, const MyCfg* c2 ) { return ::dist(*c1,*c2); }
    void interp ( const MyCfg* c1, const MyCfg* c2, float t, MyCfg* c )
         { return ::interp(*c1,*c2,t,*c); }
    void child_added ( MyCfg* parent, MyCfg* child ) {}
 }; \endcode*/
template <class C, class M=SrCfgManagerBase>
class SrCfgManager : public SrCfgManagerBase, public M
 { public :
    virtual srcfg* alloc ()
     { return (srcfg*) M::alloc(); }
    virtual srcfg* alloc ( const srcfg* c )
     { return (srcfg*) M::alloc((const C*)c); }
    virtual void free ( srcfg* c )
     { M::free ( (C*)c ); }
    virtual void copy ( srcfg* c1, const srcfg* c2 )
     { M::copy ( (C*)c1, (const C*)c2 ); }
    virtual void output ( SrOutput& o, const srcfg* c )
     { M::output ( o, (const C*)c ); }
    virtual void input ( SrInput& i, srcfg* c )
     { M::input ( i, (C*)c ); }
    virtual void random ( srcfg* c )
     { M::random ( (C*)c ); }
    virtual bool valid ( const srcfg* c )
     { return M::valid ( (const C*)c ); }
    virtual float dist ( const srcfg* c1, const srcfg* c2 )
     { return M::dist ( (const C*)c1, (const C*)c2 ); }
    virtual void interp ( const srcfg* c1, const srcfg* c2, float t, srcfg* c )
     { M::interp ( (const C*)c1, (const C*)c2, t, (C*)c ); }
    virtual bool monotone ( const srcfg* c1, const srcfg* c2 )
     { return M::monotone ( (const C*)c1, (const C*)c2 ); }
    virtual void child_added ( srcfg* child, srcfg* parent )
     { M::child_added ( (C*)parent, (C*)child ); }
    virtual float time ( srcfg* c )
     { return M::time ( (C*)c ); }
 };

//============================== end of file ===============================

# endif  // SR_CFG_MANAGER_H

