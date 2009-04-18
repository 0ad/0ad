/* Copyright (C) 2009 Wildfire Games.
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

#include <exception>

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
const PSRETURN MASK__PSRETURN_OK = 0xFFFFFFFF;
const PSRETURN CODE__PSRETURN_OK = 0;

const char* GetErrorString(PSRETURN code);
const char* GetErrorString(const PSERROR& err);
void ThrowError(PSRETURN code);

#endif
