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

// included from SpiderMonkey.h

extern JSBool jsu_report_param_error(JSContext* cx, jsval* vp);


// consistent argc checking for normal function wrappers: reports an
// error via JS and returns if number of parameters is incorrect.
// .. require exact number (most common case)
#define JSU_REQUIRE_PARAMS(exact_number)\
	if(argc != exact_number)\
		return jsu_report_param_error(cx, vp);
// .. require 0 params (avoids L4 warning "unused argv param")
#define JSU_REQUIRE_NO_PARAMS()\
	if(argc != 0)\
		return jsu_report_param_error(cx, vp);
// .. require a certain range (e.g. due to optional params)
#define JSU_REQUIRE_PARAM_RANGE(min_number, max_number)\
	if(!(min_number <= argc && argc <= max_number))\
		return jsu_report_param_error(cx, vp);
// .. require at most a certain count
#define JSU_REQUIRE_MAX_PARAMS(max_number)\
	if(argc > max_number)\
		return jsu_report_param_error(cx, vp);
// .. require at least a certain count (rarely needed)
#define JSU_REQUIRE_MIN_PARAMS(min_number)\
	if(argc < min_number)\
		return jsu_report_param_error(cx, vp);

// same as JSU_REQUIRE_PARAMS, but used from C++ functions that are
// a bit further removed from SpiderMonkey, i.e. return a
// C++ bool indicating success, and not taking an rval param.
#define JSU_REQUIRE_PARAMS_CPP(exact_number)\
	if(argc != exact_number)\
	{\
		jsu_report_param_error(cx, 0);\
		return false;\
	}


#define JSU_ASSERT(expr, msg)\
STMT(\
	if(!(expr))\
	{\
		JS_ReportError(cx, msg);\
		return JS_FALSE;\
	}\
)
