#ifndef DECLSPEC_H_INCLUDED
#define DECLSPEC_H_INCLUDED

#if defined(_WIN32) && !defined(STATICLIB)
	#ifdef MINIUPNP_EXPORTS
		#define LIBSPEC __declspec(dllexport)
	#else
		#define LIBSPEC __declspec(dllimport)
	#endif
#else
	#if defined(__GNUC__) && __GNUC__ >= 4
		#define LIBSPEC __attribute__ ((visibility ("default")))
	#else
		#define LIBSPEC
	#endif
#endif

#endif

