/**
 * =========================================================================
 * File        : secure_crt.cpp
 * Project     : 0 A.D.
 * Description : partial implementation of VC8's secure CRT functions
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_SECURE_CRT
#define INCLUDED_SECURE_CRT
#if !HAVE_SECURE_CRT

extern int sprintf_s(char* buf, size_t max_chars, const char* fmt, ...);

typedef int errno_t;
extern errno_t fopen_s(FILE** pfile, const char* filename, const char* mode);

#define fscanf_s fscanf

#endif	// #if !HAVE_SECURE_CRT
#endif	// #ifndef INCLUDED_SECURE_CRT
