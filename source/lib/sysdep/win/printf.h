// Acts like vsnprintf, but handles positional parameters and %lld
int vsnprintf2(char* buffer, size_t count, const char* format, va_list argptr);
