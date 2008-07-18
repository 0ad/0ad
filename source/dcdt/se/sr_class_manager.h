
# ifndef SR_CLASS_MANAGER_H
# define SR_CLASS_MANAGER_H

/** \file sr_class_manager.h 
 * Generic way to allocate, io and compare classes */

# include "sr_input.h"
# include "sr_output.h"
# include "sr_shared_class.h"

class SrClassManagerBase : public SrSharedClass
 { protected :
    virtual ~SrClassManagerBase() {};

   public : // callbacks
    virtual void* alloc ()=0;
    virtual void* alloc ( const void* obj )=0;
    virtual void free ( void* obj )=0;
    virtual void output ( SrOutput& UNUSED(o), const void* UNUSED(obj) ) { }
    virtual void input ( SrInput& UNUSED(i), void* UNUSED(obj) ) { }
    virtual int compare ( const void* UNUSED(obj1), const void* UNUSED(obj2) ) { return 0; }
 };

/*! Example of an implementation of a class to be automatically managed
   with SrClassManager<MyData> :
class MyData
 { public :
    MyData ();
    MyData ( const MyData& d );
   ~MyData ();
    friend SrOutput& operator<< ( SrOutput& out, const MyData& d );
    friend SrInput& operator>> ( SrInput& inp, MyData& d );
    friend int sr_compare ( const MyData* d1, const MyData* d2 );
 };*/
 
template <class X>
class SrClassManager : public SrClassManagerBase
 { protected :
    virtual ~SrClassManager<X> () {}

   public :
    virtual void* alloc () { return (void*) new X; }

    virtual void* alloc ( const void* obj ) { return (void*) new X(*((X*)obj)); }

    virtual void free ( void* obj ) { delete (X*) obj; }

    virtual void output ( SrOutput& o, const void* obj ) { o<<*((const X*)obj); }

    virtual void input ( SrInput& i, void* obj ) { i>>*((X*)obj); }

    virtual int compare ( const void* obj1, const void* obj2 )
     { return sr_compare ( (const X*)obj1, (const X*)obj2 ); }
 };

//============================== end of file ===============================

# endif  // SR_CLASS_MANAGER_H
