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

	virtual void ScriptVal(const char* name, JS::MutableHandleValue out);
	virtual void ScriptObjectAppend(const char* name, JS::HandleValue objVal);
	virtual void ScriptString(const char* name, JSString*& out);

	virtual std::istream& GetStream();
	virtual void RequireBytesInStream(size_t numBytes);

	virtual void SetSerializablePrototypes(std::map<std::wstring, JSObject*>& prototypes);
	
protected:
	virtual void Get(const char* name, u8* data, size_t len);

private:
	jsval ReadScriptVal(const char* name, JS::HandleObject appendParent);
	void ReadStringUTF16(const char* name, utf16string& str);

	virtual void AddScriptBackref(JSObject* obj);
	virtual JSObject* GetScriptBackref(u32 tag);
	virtual u32 ReserveScriptBackref();
	virtual void SetReservedScriptBackref(u32 tag, JSObject* obj);
	void FreeScriptBackrefs();
	std::map<u32, JSObject*> m_ScriptBackrefs; // vector would be nice but maintaining JS roots would be harder
	ScriptInterface& m_ScriptInterface;

	std::istream& m_Stream;

	std::map<std::wstring, JSObject*> m_SerializablePrototypes;

	bool IsSerializablePrototype(const std::wstring& name);
	JSObject* GetSerializablePrototype(const std::wstring& name);
};

#endif // INCLUDED_STDDESERIALIZER
