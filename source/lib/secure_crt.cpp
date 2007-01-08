#include "precompiled.h"

#include <stdio.h>
#include <errno.h>

#include "secure_crt.h"

#if !HAVE_SECURE_CRT

int sprintf_s(char* buf, size_t max_chars, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(buf, max_chars, fmt, args);
	va_end(args);
	return len;
}


errno_t fopen_s(FILE** pfile, const char* filename, const char* mode)
{
	*pfile = NULL;
	FILE* file = fopen(filename, mode);
	if(!file)
		return ENOENT;
	*pfile = file;
	return 0;
}

#endif // #if !HAVE_SECURE_CRT
