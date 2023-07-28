/* Copyright (C) 2020 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_SCRIPTEXCEPTIONS
#define INCLUDED_SCRIPTEXCEPTIONS

struct JSContext;
class JSErrorReport;
class ScriptRequest;

namespace ScriptException
{
/**
 * @return Whether there is a JS exception pending.
 */
bool IsPending(const ScriptRequest& rq);

/**
 * Log and then clear the current pending exception. This function should always be called after calling a
 * JS script (or anything that can throw JS errors, such as structured clones),
 * in case that script doesn't catch an exception thrown during its execution.
 * If no exception is pending, this does nothing.
 * Note that JS code that wants to throw errors should throw new Error(...), otherwise the stack cannot be used.
 * @return Whether there was a pending exception.
 */
bool CatchPending(const ScriptRequest& rq);

/**
 * Raise a JS exception from C++ code.
 * This is only really relevant in JSNative functions that don't use ObjectOpResult,
 * as the latter overwrites the pending exception.
 * Prefer either simply logging an error if you know a stack-trace will be raised elsewhere.
 */
void Raise(const ScriptRequest& rq, const char* format, ...);
}

#endif // INCLUDED_SCRIPTEXCEPTIONS
