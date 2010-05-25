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

#ifndef INCLUDED_STDDESERIALIZER
#define INCLUDED_STDDESERIALIZER

#include "IDeserializer.h"

#include "ps/utf16string.h"

#include <map>

class CStdDeserializer : public IDeserializer
{
	NONCOPYABLE(CStdDeserializer);
public:
	CStdDeserializer(ScriptInterface& scriptInterface, std::istream& stream);
	virtual ~CStdDeserializer();

	virtual void ScriptVal(jsval& out);
	virtual void ScriptVal(CScriptVal& out);
	virtual void ScriptVal(CScriptValRooted& out);
	virtual void ScriptObjectAppend(jsval& obj);
	virtual void ScriptString(JSString*& out);

protected:
	virtual void Get(u8* data, size_t len);

private:
	jsval ReadScriptVal(JSObject* appendParent);
	void ReadStringUTF16(utf16string& str);

	virtual void AddScriptBackref(JSObject* obj);
	virtual JSObject* GetScriptBackref(u32 tag);
	void FreeScriptBackrefs();
	std::map<u32, JSObject*> m_ScriptBackrefs; // vector would be nice but maintaining JS roots would be harder
	ScriptInterface& m_ScriptInterface;

	std::istream& m_Stream;
};

#endif // INCLUDED_STDDESERIALIZER
