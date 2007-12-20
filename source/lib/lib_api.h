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
