/* Copyright (C) 2016 Wildfire Games.
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
	virtual void ScriptString(const char* name, JS::MutableHandleString out);

	virtual std::istream& GetStream();
	virtual void RequireBytesInStream(size_t numBytes);

	virtual void SetSerializablePrototypes(std::map<std::wstring, JS::Heap<JSObject*> >& prototypes);

	static void Trace(JSTracer *trc, void *data);

	void TraceMember(JSTracer *trc);

protected:
	virtual void Get(const char* name, u8* data, size_t len);

private:
	jsval ReadScriptVal(const char* name, JS::HandleObject appendParent);
	void ReadStringLatin1(const char* name, std::vector<JS::Latin1Char>& str);
	void ReadStringUTF16(const char* name, utf16string& str);

	virtual void AddScriptBackref(JS::HandleObject obj);
	virtual void GetScriptBackref(u32 tag, JS::MutableHandleObject ret);
	std::vector<JS::Heap<JSObject*> > m_ScriptBackrefs;
	JS::PersistentRooted<JSObject*> m_dummyObject;

	ScriptInterface& m_ScriptInterface;

	std::istream& m_Stream;

	std::map<std::wstring, JS::Heap<JSObject*> > m_SerializablePrototypes;

	bool IsSerializablePrototype(const std::wstring& name);
	void GetSerializablePrototype(const std::wstring& name, JS::MutableHandleObject ret);
};

#endif // INCLUDED_STDDESERIALIZER
