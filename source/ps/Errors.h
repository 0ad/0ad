#ifndef _ERRORS_H_
#define _ERRORS_H_

#include <stdlib.h> // for wchar_t

class PSERROR
{
public:
	int magic; // = 0x50534552, so the exception handler can recognise that it's a PSERROR
	int code;
};

#define ERROR_GROUP(a,b) class a##_##b : public a {}
#define ERROR_TYPE(a,b) class a##_##b : public a { public: a##_##b(); }

const wchar_t* GetErrorString(int code);

#endif
