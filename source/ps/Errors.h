#ifndef _ERRORS_H_
#define _ERRORS_H_

#include <stdlib.h> // for wchar_t

typedef unsigned long PSRETURN;

class PSERROR
{
public:
	int magic;	// = 0x45725221, so the exception handler can recognise
				// that it's a PSERROR and not some other random object.
	PSRETURN code; // unique (but arbitrary) code, for translation tables etc
};

#define ERROR_GROUP(a) class PSERROR_##a : public PSERROR {}; \
						extern const PSRETURN MASK__PSRETURN_##a; \
						extern const PSRETURN CODE__PSRETURN_##a

#define ERROR_SUBGROUP(a,b) class PSERROR_##a##_##b : public PSERROR_##a {}; \
						extern const PSRETURN MASK__PSRETURN_##a##_##b; \
						extern const PSRETURN CODE__PSRETURN_##a##_##b


#define ERROR_TYPE(a,b) class PSERROR_##a##_##b : public PSERROR_##a { public: PSERROR_##a##_##b(); }; \
						extern const PSRETURN MASK__PSRETURN_##a##_##b; \
						extern const PSRETURN CODE__PSRETURN_##a##_##b; \
						extern const PSRETURN PSRETURN_##a##_##b

#define ERROR_IS(a, b) ( ((a) & MASK__PSRETURN_##b) == CODE__PSRETURN_##b )

const PSRETURN PSRETURN_OK = 0;
const PSRETURN MASK__PSRETURN_OK = 0xffffffff;
const PSRETURN CODE__PSRETURN_OK = 0;

const wchar_t* GetErrorString(PSRETURN code);

#endif
