/* Copyright (C) 2013 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_ERRORS
#define INCLUDED_ERRORS

/*

The overly-complex error system works as follows:

A source file (typically a .h) can declare errors as follows:

	ERROR_GROUP(ModuleName);
	ERROR_TYPE(ModuleName, FrobnificationFailed);
	ERROR_SUBGROUP(ModuleName, ComponentName);
	ERROR_TYPE(ModuleName_ComponentName, FileNotFound);

etc, to build up a hierarchy of error types.

Then you have to run the /build/errorlist/errorlist.pl script, to regenerate
the Errors.cpp file.

Then you can use the declared errors as an error code:

	PSRETURN foo() { return PSRETURN_ModuleName_FrobnificationFailed; }

	if (ret != PSRETURN_OK)
		... // something failed

	if (ret)
		... // something failed

	if (ret == PSRETURN_ModuleName_FrobnificationFailed)
		... // particular error

	if (ERROR_IS(ret, PSRETURN_ModuleName))
		... // matches any type PSRETURN_ModuleName_* (and PSRETURN_ModuleName_*_* etc)

And you can use it as an exception:

	void foo() { throw PSERROR_ModuleName_FrobnificationFailed(); }

	void bar() { throw PSERROR_ModuleName_FrobnificationFailed("More informative message"); }

	try {
		foo();
	} catch (PSERROR_ModuleName_FrobnificationFailed& e) {
		// catches that particular error type
	} catch (PSERROR_ModuleName& e) {
		// catches anything in the hierarchy
	} catch (PSERROR& e) {
		std::cout << e.what();
	}

plus a few extra things for converting between error codes and exceptions.

*/

#include <exception>

typedef u32 PSRETURN;

class PSERROR : public std::exception
{
public:
	PSERROR(const char* msg);
	virtual const char* what() const throw ();
	virtual PSRETURN getCode() const = 0; // for functions that catch exceptions then return error codes
private:
	const char* m_msg;
};

#define ERROR_GROUP(a) class PSERROR_##a : public PSERROR { protected: PSERROR_##a(const char* msg); }; \
						extern const PSRETURN MASK__PSRETURN_##a; \
						extern const PSRETURN CODE__PSRETURN_##a

#define ERROR_SUBGROUP(a,b) class PSERROR_##a##_##b : public PSERROR_##a { protected: PSERROR_##a##_##b(const char* msg); }; \
						extern const PSRETURN MASK__PSRETURN_##a##_##b; \
						extern const PSRETURN CODE__PSRETURN_##a##_##b


#define ERROR_TYPE(a,b) class PSERROR_##a##_##b : public PSERROR_##a { public: PSERROR_##a##_##b(); PSERROR_##a##_##b(const char* msg); PSRETURN getCode() const; }; \
						extern const PSRETURN MASK__PSRETURN_##a##_##b; \
						extern const PSRETURN CODE__PSRETURN_##a##_##b; \
						extern const PSRETURN PSRETURN_##a##_##b

#define ERROR_IS(a, b) ( ((a) & MASK__PSRETURN_##b) == CODE__PSRETURN_##b )

const PSRETURN PSRETURN_OK = 0;
const PSRETURN MASK__PSRETURN_OK = 0xFFFFFFFF;
const PSRETURN CODE__PSRETURN_OK = 0;

const char* GetErrorString(PSRETURN code);
void ThrowError(PSRETURN code);

#endif
