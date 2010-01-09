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

#include "ScriptVal.h"

#include "js/jsapi.h"

struct Unrooter
{
	Unrooter(JSContext* cx) : cx(cx) { }
	void operator()(jsval* p) { JS_RemoveRoot(cx, p); delete p; }
	JSContext* cx;
};

CScriptValRooted::CScriptValRooted(JSContext* cx, jsval val)
{
	jsval* p = new jsval(val);
	JS_AddNamedRoot(cx, p, "CScriptValRooted");
	m_Val = boost::shared_ptr<jsval>(p, Unrooter(cx));
}

CScriptValRooted::CScriptValRooted(JSContext* cx, CScriptVal val)
{
	jsval* p = new jsval(val.get());
	JS_AddNamedRoot(cx, p, "CScriptValRooted");
	m_Val = boost::shared_ptr<jsval>(p, Unrooter(cx));
}

jsval CScriptValRooted::get() const
{
	if (!m_Val)
		return JSVAL_VOID;
	return *m_Val;
}
