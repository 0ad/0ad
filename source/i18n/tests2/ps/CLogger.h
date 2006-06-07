enum {
	ERROR,
	WARNING,
};

#include <stdarg.h>

static void LOG(int level, const char* cat, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	printf("\n");
}