
# ifndef SR_MEM_CONTROL_H
# define SR_MEM_CONTROL_H

# include <stdlib.h>

/** \file sr_mem_control.h 
 * Functions and macros to control memory allocation.
 * The idea of usage is similar to sr_trace.h: only inside 
 * files that testing memory allocation is required that the activation
 * of macros should be defined. */

/*! This function saves in an internal buffer the given parameters
    in order to keep track of allocated memory.
    It may be used by replacing new/malloc calls to:
    \code sr_mem_control_alloc("X",__FILE__,__LINE__,sizeof(X),new X) \endcode
    Then sr_memory_report() can be called to see all allocated data that was not
    freed. It is better to use this function with the macros SR_NEW and SR_MALLOC. */
void* sr_mem_control_alloc ( const char* type, char* file, int line, int size, void* addr );

/*! This function erases from the internal buffer the given address
    of memory, that was  previously registered with sr_mem_control_alloc().
    It is to be used replacing delete/free calls in the following way:
    \code sr_mem_control_free(__FILE__,__LINE__,X); delete X; \endcode
    It is better to use this function with the macros SR_DEL, SR_DELA and SR_FREE. */
bool sr_mem_control_free ( char* file, int line, void* addr );

/*! This function performs a call to realloc(addr,size), and returns the new
    pointer. But it also updates the pointers saved in the internal buffer in
    order to keep track of allocated memory.
    It may be used replacing realloc calls in the following way:
    \code pt=sr_mem_control_realloc(__FILE__,__LINE__,newsize,pt) \endcode
    It is better to use this function with the macro SR_REALLOC. */
void* sr_mem_control_realloc ( char* file, int line, int size, void* addr );

/*! This function gives a report about all allocated data that was not freed
    using sr_out. But to make it work, the user needs to change all 
    new/malloc/delete/free calls by calls to the macros SR_NEW, SR_MALLOC,
    SR_REALLOC, SR_DEL, SR_DELA, SR_FREE or functions
    sr_mem_control_alloc/free/realloc. */
void sr_memory_report ();

/*! Returns the total of memory allocated. Only the memory allocated using
    sr_mem_control_alloc/free/realloc, or the related macros are taken into account. */
unsigned sr_memory_allocated ();

class SrOutput;

/*! Changes the output used for the reports. */
void sr_memory_report_output ( SrOutput& o );

# ifdef SR_USE_MEM_CONTROL
#  define SR_DEL(X)       if (sr_mem_control_free(__FILE__,__LINE__,X)) delete X;   
#  define SR_DELA(X)      if (sr_mem_control_free(__FILE__,__LINE__,X)) delete[] X; 
#  define SR_FREE(p)      if (sr_mem_control_free(__FILE__,__LINE__,p)) free(p);    
#  define SR_NEW(X)       ((X*)sr_mem_control_alloc(#X,__FILE__,__LINE__,sizeof(X),new X))
#  define SR_NEW1(X,A)    ((X*)sr_mem_control_alloc(#X,__FILE__,__LINE__,sizeof(X),new X(A)))
#  define SR_NEW2(X,A,B)  ((X*)sr_mem_control_alloc(#X,__FILE__,__LINE__,sizeof(X),new X(A,B)))
#  define SR_NEWA(X,s)    ((X*)sr_mem_control_alloc(#X,__FILE__,__LINE__,sizeof(X),new X[s]))
#  define SR_MALLOC(s)    sr_mem_control_alloc("malloc",__FILE__,__LINE__,s,malloc(s))
#  define SR_REALLOC(p,s) sr_mem_control_realloc(__FILE__,__LINE__,s,p)
# else
#  define SR_DEL(X)       delete X
#  define SR_DELA(X)      delete[] X
#  define SR_FREE(p)      free(p)
#  define SR_NEW(X)       new X
#  define SR_NEW1(X,A)    new X(A)
#  define SR_NEW2(X,A,B)  new X(A,B)
#  define SR_MALLOC(s)    malloc(s)
#  define SR_REALLOC(p,s) realloc(p,s)
# endif

/*! \def SR_DEL(X)
    The user needs to use SR_DEL in place of delete, in order to be able to
    use sr_memory_report() for obtaining a report of memory that was allocated
    but not freed.
    But to activate this define, the user must define SR_USE_MEM_CONTROL before
    including the header sr_mem_control.h. Otherwise this define will just
    directly call delete, with no overhead to the compiled code.
    Use: SR_DEL(X); instead of delete X; */

/*! \def SR_DELA(X)
    The user needs to use SR_DELA in place of delete[], in order to be able to
    use sr_memory_report() for obtaining a report of memory that was allocated
    but not freed.
    But to activate this define, the user must define SR_USE_MEM_CONTROL before
    including the header sr_mem_control.h. Otherwise this define will just
    directly call delete[], with no overhead to the compiled code.
    Use: SR_DELA(X); instead of delete[] X; */

/*! \def SR_FREE(p)
    The user needs to use SR_FREE in place of free(), in order to be able to use
    sr_memory report() for obtaining a report of memory that was allocated but
    not freed.
    But to activate this define, the user must define SR_USE_MEM_CONTROL before
    including the header sr_mem_control.h. Otherwise this define will just directly
    call free(), with no overhead to the compiled code.
    Use: SR_FREE(p); instead of free(p); */

/*! \def SR_NEW(X)
    The user needs to use SR_NEW in place of new, in order to be able to use
    sr_memory report() for obtaining a report of memory that was allocated but
    not freed.
    But to activate this define, the user must define SR_USE_MEM_CONTROL before
    including the header sr_mem_control.h. Otherwise this define will just
    directly call new, with no overhead to the compiled code.
    Use: pt=SR_NEW(X); instead of pt=new X; */

/*! \def SR_NEW1(X,A)
    Equivalent to macro SR_NEW, but allows to set one initialisation parameter for
    the constructor of class X.
    Use: pt=SR_NEW(X,A); instead of pt=new X(A); */

/*! \def SR_NEW2(X,A,B)
    Equivalent to macro SR_NEW, but allows to set two initialisation parameters for
    the constructor of class X.
    Use: pt=SR_NEW(X,A,B); instead of pt=new X(A,B); */

/*! \def SR_MALLOC(s)
    The user needs to use SR_MALLOC in place of malloc(), in order to be able
    to use sr_memory report() for obtaining a report of memory that was allocated
    but not freed.
    But to activate this define, the user must define SR_USE_MEM_CONTROL before
    including the header sr_mem_control.h. Otherwise this define will just directly
    call malloc(), with no overhead to the compiled code.
    Use: pt=SR_MALLOC(s); instead of pt=malloc(s); */

/*! \def SR_REALLOC(p,s)
    The user needs to use SR_REALLOC in place of realloc(), in order to be able to
    use sr_memory report() for obtaining a report of memory that was allocated but
    not freed.
    But to activate this define, the user must define SR_USE_MEM_CONTROL before
    including the header sr_mem_control.h. Otherwise this define will just directly
    call realloc(), with no overhead to the compiled code.
    Use: pt=SR_REALLOC(p,s); instead of pt=realloc(p,s); */

//============================= end of file ==========================

# endif  // SR_MEM_CONTROL_H
