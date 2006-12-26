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
typedef void (*OutputFn) (void* cb_data, const char* data, unsigned int length);

#define COLLADA_CONVERTER_VERSION 1

EXPORT void set_logger(LogFn logger);
EXPORT int convert_dae_to_pmd(const char* dae, OutputFn pmd_writer, void* cb_data);
EXPORT int convert_dae_to_psa(const char* dae, OutputFn psa_writer, void* cb_data);

#ifdef __cplusplus
};
#endif

#endif /* COLLADA_DLL_H__ */
