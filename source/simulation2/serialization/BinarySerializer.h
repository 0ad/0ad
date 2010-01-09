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

#ifndef INCLUDED_BINARYSERIALIZER
#define INCLUDED_BINARYSERIALIZER

#include "ISerializer.h"

#include <map>

/**
 * Serialize to a binary stream. Subclasses should just need to implement
 * the Put() method.
 */
class CBinarySerializer : public ISerializer
{
	NONCOPYABLE(CBinarySerializer);
public:
	CBinarySerializer(ScriptInterface& scriptInterface);
	virtual ~CBinarySerializer();

protected:
	virtual void PutNumber(const char* name, uint8_t value);
	virtual void PutNumber(const char* name, int32_t value);
	virtual void PutNumber(const char* name, uint32_t value);
	virtual void PutNumber(const char* name, float value);
	virtual void PutNumber(const char* name, double value);
	virtual void PutNumber(const char* name, CFixed_23_8 value);
	virtual void PutBool(const char* name, bool value);
	virtual void PutString(const char* name, const std::string& value);
	virtual void PutScriptVal(const char* name, jsval value);

private:
	// PutScriptVal implementation details:
	void ScriptString(const char* name, JSString* string);
	void HandleScriptVal(jsval val);
	u32 GetScriptBackrefTag(JSObject* obj);
	void FreeScriptBackrefs();
	std::map<JSObject*, u32> m_ScriptBackrefs;
	ScriptInterface& m_ScriptInterface;
};

#endif // INCLUDED_BINARYSERIALIZER
