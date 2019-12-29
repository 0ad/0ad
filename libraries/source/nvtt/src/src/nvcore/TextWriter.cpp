// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "TextWriter.h"

using namespace nv;


/// Constructor
TextWriter::TextWriter(Stream * s) : 
    s(s), 
    str(1024)
{
    nvCheck(s != NULL);
    nvCheck(s->isSaving());
}

void TextWriter::writeString(const char * str)
{
    nvDebugCheck(s != NULL);
    s->serialize(const_cast<char *>(str), strLen(str));
}

void TextWriter::writeString(const char * str, uint len)
{
    nvDebugCheck(s != NULL);
    s->serialize(const_cast<char *>(str), len);
}

void TextWriter::format(const char * format, ...)
{
    va_list arg;
    va_start(arg,format);
    str.formatList(format, arg);
    writeString(str.str(), str.length());
    va_end(arg);
}

void TextWriter::formatList(const char * format, va_list arg)
{
    va_list tmp;
    va_copy(tmp, arg);
    str.formatList(format, arg);
    writeString(str.str(), str.length());
    va_end(tmp);
}
