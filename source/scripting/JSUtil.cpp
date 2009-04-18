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

#include "precompiled.h"

#include "SpiderMonkey.h"

jsval jsu_report_param_error(JSContext* cx, jsval* rval)
{
	JS_ReportError(cx, "Invalid parameter(s) or count");

	if(rval)
		*rval = JSVAL_NULL;

	// yes, we had an error, but returning JS_FALSE would cause SpiderMonkey
	// to abort. that would be hard to debug.
	return JS_TRUE;
}
