/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_DEBUGSERIALIZER
#define INCLUDED_DEBUGSERIALIZER

#include "ISerializer.h"

/**
 * Serialize to a human-readable YAML-like format.
 */
class CDebugSerializer : public ISerializer
{
	NONCOPYABLE(CDebugSerializer);
public:
	/**
	 * @param scriptInterface Script interface corresponding to any jsvals passed to ScriptVal()
	 * @param stream Stream to receive UTF-8 encoded output
	 * @param includeDebugInfo If true then additional non-deterministic data will be included in the output
	 */
	CDebugSerializer(ScriptInterface& scriptInterface, std::ostream& stream, bool includeDebugInfo = true);

	void Comment(const std::string& comment);
	void TextLine(const std::string& text);
	void Indent(int spaces);
	void Dedent(int spaces);

	virtual bool IsDebug() const;
	virtual std::ostream& GetStream();

protected:
	virtual void PutNumber(const char* name, uint8_t value);
	virtual void PutNumber(const char* name, int8_t value);
	virtual void PutNumber(const char* name, uint16_t value);
	virtual void PutNumber(const char* name, int16_t value);
	virtual void PutNumber(const char* name, uint32_t value);
	virtual void PutNumber(const char* name, int32_t value);
	virtual void PutNumber(const char* name, float value);
	virtual void PutNumber(const char* name, double value);
	virtual void PutNumber(const char* name, fixed value);
	virtual void PutBool(const char* name, bool value);
	virtual void PutString(const char* name, const std::string& value);
	virtual void PutScriptVal(const char* name, JS::MutableHandleValue value);
	virtual void PutRaw(const char* name, const u8* data, size_t len);

private:
	ScriptInterface& m_ScriptInterface;
	std::ostream& m_Stream;
	bool m_IsDebug;
	int m_Indent;
};

#endif // INCLUDED_DEBUGSERIALIZER
