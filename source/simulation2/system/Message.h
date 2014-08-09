/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_MESSAGE
#define INCLUDED_MESSAGE

#include "scriptinterface/ScriptTypes.h"
#include "scriptinterface/ScriptVal.h"

class CMessage
{
	NONCOPYABLE(CMessage);
protected:
	CMessage() { }
public:
	virtual ~CMessage() { }
	virtual int GetType() const = 0;
	virtual const char* GetScriptHandlerName() const = 0;
	virtual const char* GetScriptGlobalHandlerName() const = 0;
	virtual JS::Value ToJSVal(ScriptInterface&) const = 0;
	JS::Value ToJSValCached(ScriptInterface&) const;
private:
	mutable CScriptValRooted m_Cached;
};
// TODO: GetType could be replaced with a plain member variable to avoid some
// virtual calls, if that turns out to be worthwhile

CMessage* CMessageFromJSVal(int mtid, ScriptInterface&, JS::HandleValue);

#endif // INCLUDED_MESSAGE
