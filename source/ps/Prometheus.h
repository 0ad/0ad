/*
Prometheus.h
by Raj Sharma
rsharma@uiuc.edu

Standard declarations which are included in all projects.
*/

#ifndef PROMETHEUS_H
#define PROMETHEUS_H

#pragma warning (disable : 4786)
#pragma warning (disable : 4291)

#include <stdio.h>
#include <math.h>
#include <assert.h>

// Standard typedefs

typedef int            _int;
typedef unsigned int   _unsigned_int;

typedef long           _long;
typedef unsigned long  _unsigned_long;

typedef bool           _bool;

typedef char           _char;
typedef unsigned char  _unsigned_char;

typedef char           _byte;
typedef unsigned char  _unsigned_byte;

typedef float          _float;
typedef double         _double;

typedef unsigned short _unsigned_short;
typedef unsigned short _ushort;
typedef short _short;
typedef unsigned long _ulong;
typedef unsigned int _uint;

// Error handling

typedef const char * PS_RESULT;

#define DEFINE_ERROR(x, y)  PS_RESULT x=y; 
#define DECLARE_ERROR(x)  extern PS_RESULT x; 

DECLARE_ERROR(PS_OK);
DECLARE_ERROR(PS_FAIL);


/*
inline bool ErrorMechanism_Error();
	inline void ErrorMechanism_ClearError();
	inline bool ErrorMechanism_TestError( PS_RESULT);

	inline void ErrorMechanism_SetError(
		PS_RESULT, 
		std::string, 
		std::string, 
		unsigned int);

	inline CError ErrorMechanism_GetError();
*/


// Setup error interface
#define IsError() GError.ErrorMechanism_Error()
#define ClearError() GError.ErrorMechanism_ClearError()
#define TestError(TError)  GError.ErrorMechanism_TestError(TError)

#define SetError_Short(w,x) GError.ErrorMechanism_SetError(w,x,__FILE__,__LINE__)
#define SetError_Long(w,x,y,z) GError.ErrorMechanism_SetError(w,x,y,z)

#define GetError() GError.ErrorMechanism_GetError()

#endif