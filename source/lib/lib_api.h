#include "lib/sysdep/compiler.h"

// note: EXTERN_C cannot be used because shared_ptr is often returned
// by value, which requires C++ linkage.

#ifdef LIB_DLL
# ifdef LIB_BUILD
#  define LIB_API __declspec(dllexport)
# else
#  define LIB_API __declspec(dllimport)
# endif
#else
# define LIB_API
#endif

#if defined(LIB_DLL) && !defined(LIB_BUILD)
# if MSC_VERSION
#  ifdef NDEBUG
#   pragma comment(lib, "lib.lib")
#  else
#   pragma comment(lib, "lib_d.lib")
#  endif
# endif
#endif
