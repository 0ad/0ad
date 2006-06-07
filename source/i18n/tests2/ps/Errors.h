#ifndef _ERRORS_H_
#define _ERRORS_H_

#include <exception>

#include "lib/types.h"

typedef u32 PSRETURN;

class PSERROR : public std::exception
{
public:
	virtual const char* what() const throw ();
	virtual PSRETURN getCode() const = 0; // for functions that catch exceptions then return error codes
};

#define ERROR_GROUP(a) class PSERROR_##a : public PSERROR {}; \
						extern const PSRETURN MASK__PSRETURN_##a; \
						extern const PSRETURN CODE__PSRETURN_##a

#define ERROR_SUBGROUP(a,b) class PSERROR_##a##_##b : public PSERROR_##a {}; \
						extern const PSRETURN MASK__PSRETURN_##a##_##b; \
						extern const PSRETURN CODE__PSRETURN_##a##_##b


#define ERROR_TYPE(a,b) class PSERROR_##a##_##b : public PSERROR_##a { public: PSRETURN getCode() const; }; \
						extern const PSRETURN MASK__PSRETURN_##a##_##b; \
						extern const PSRETURN CODE__PSRETURN_##a##_##b; \
						extern const PSRETURN PSRETURN_##a##_##b

#define ERROR_IS(a, b) ( ((a) & MASK__PSRETURN_##b) == CODE__PSRETURN_##b )

const PSRETURN PSRETURN_OK = 0;
const PSRETURN MASK__PSRETURN_OK = 0xffffffff;
const PSRETURN CODE__PSRETURN_OK = 0;

const char* GetErrorString(PSRETURN code);
const char* GetErrorString(const PSERROR& err);
void ThrowError(PSRETURN code);

#endif
