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

#ifndef INCLUDED_SCRIPTVAL
#define INCLUDED_SCRIPTVAL

#include "ScriptTypes.h"
#include <boost/shared_ptr.hpp>

/**
 * A trivial wrapper around a jsval. Used to avoid template overload ambiguities
 * with jsval (which is just an integer), for any code that uses
 * ScriptInterface::ToJSVal or ScriptInterface::FromJSVal
 */
class CScriptVal
{
public:
	CScriptVal() : m_Val(0) { }
	CScriptVal(jsval val) : m_Val(val) { }

	jsval get() const { return m_Val; }

private:
	jsval m_Val;
};

class CScriptValRooted
{
public:
	CScriptValRooted() { }
	CScriptValRooted(JSContext* cx, jsval val);
	CScriptValRooted(JSContext* cx, CScriptVal val);

	jsval get() const;

	bool undefined() const;

	bool uninitialised() const;

private:
	boost::shared_ptr<jsval> m_Val;
};

#endif // INCLUDED_SCRIPTVAL
