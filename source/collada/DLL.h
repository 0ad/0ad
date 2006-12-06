#ifndef COLLADA_DLL_H__
#define COLLADA_DLL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
# ifdef COLLADA_DLL
#  define EXPORT extern __declspec(dllexport)
# else
#  define EXPORT extern __declspec(dllimport)
# endif
#else
# define EXPORT extern
#endif

#define LOG_INFO 0
#define LOG_WARNING 1
#define LOG_ERROR 2

typedef void (*LogFn) (int severity, const char* text);
typedef void (*OutputFn) (const char* data, unsigned int length);

EXPORT void set_logger(LogFn logger);
EXPORT int convert_dae_to_pmd(const char* dae, OutputFn pmd_writer);

#ifdef __cplusplus
};
#endif

#endif /* COLLADA_DLL_H__ */
