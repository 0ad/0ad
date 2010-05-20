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

#ifndef INCLUDED_SCRIPTTYPES
#define INCLUDED_SCRIPTTYPES

#ifdef _WIN32
# define XP_WIN
#else
# define XP_UNIX
#endif
// (we don't support XP_OS2 or XP_BEOS)

#include "js/jspubtd.h"
#include "js/jsversion.h"

#if JS_VERSION != 185
#error Your compiler is trying to use an incorrect version of the SpiderMonkey library.
#error The only version that works is the one in the libraries/spidermonkey-tip/ directory,
#error and it will not work with a typical system-installed version.
#error Make sure you have got all the right files and include paths.
#endif

class ScriptInterface;
class CScriptVal;
class CScriptValRooted;

#endif // INCLUDED_SCRIPTTYPES
