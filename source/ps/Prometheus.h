/*
Prometheus.h
by Raj Sharma
rsharma@uiuc.edu

Standard declarations which are included in all projects.
*/

#ifndef PROMETHEUS_H
#define PROMETHEUS_H


#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "CLogger.h"

// Globals
extern int g_xres, g_yres;
extern int g_bpp;
extern int g_freq;


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