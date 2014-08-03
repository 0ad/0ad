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

#include "precompiled.h"

#include "DebugSerializer.h"

#include "scriptinterface/ScriptInterface.h"

#include "lib/secure_crt.h"
#include "lib/utf8.h"
#include "ps/CStr.h"

#include <sstream>
#include <iomanip>

/*
 * The output format here is intended to be compatible with YAML,
 * so it is human readable and usable in diff and can also be parsed with
 * external tools.
 */

// MSVC and GCC give slightly different serializations of floats
// (e.g. "1e+010" vs "1e+10"). To make the debug serialization easily comparable
// across platforms, we want to convert to a canonical form.
// TODO: we just do e+0xx now; ought to handle varying precisions and inf and nan etc too
template<typename T>
std::string canonfloat(T value, int prec)
{
	std::stringstream str;
	str << std::setprecision(prec) << value;
	std::string r = str.str();
	size_t e = r.find('e');
	if (e == r.npos) // no "e"
		return r;
	if (e == r.length() - 5 && r[e + 2] == '0') // e.g. "1e+010"
		r.erase(e + 2, 1);
	return r;
}

CDebugSerializer::CDebugSerializer(ScriptInterface& scriptInterface, std::ostream& stream, bool includeDebugInfo) :
	m_ScriptInterface(scriptInterface), m_Stream(stream), m_IsDebug(includeDebugInfo), m_Indent(0)
{
}

void CDebugSerializer::Indent(int spaces)
{
	m_Indent += spaces;
}

void CDebugSerializer::Dedent(int spaces)
{
	ENSURE(spaces <= m_Indent);
	m_Indent -= spaces;
}

#define INDENT std::string(m_Indent, ' ')

void CDebugSerializer::Comment(const std::string& comment)
{
	m_Stream << INDENT << "# " << comment << "\n";
}

void CDebugSerializer::TextLine(const std::string& text)
{
	m_Stream << INDENT << text << "\n";
}

void CDebugSerializer::PutNumber(const char* name, uint8_t value)
{
	m_Stream << INDENT << name << ": " << (int)value << "\n";
}

void CDebugSerializer::PutNumber(const char* name, int8_t value)
{
	m_Stream << INDENT << name << ": " << (int)value << "\n";
}

void CDebugSerializer::PutNumber(const char* name, uint16_t value)
{
	m_Stream << INDENT << name << ": " << value << "\n";
}

void CDebugSerializer::PutNumber(const char* name, int16_t value)
{
	m_Stream << INDENT << name << ": " << value << "\n";
}

void CDebugSerializer::PutNumber(const char* name, uint32_t value)
{
	m_Stream << INDENT << name << ": " << value << "\n";
}

void CDebugSerializer::PutNumber(const char* name, int32_t value)
{
	m_Stream << INDENT << name << ": " << value << "\n";
}

void CDebugSerializer::PutNumber(const char* name, float value)
{
	m_Stream << INDENT << name << ": " << canonfloat(value, 8) << "\n";
}

void CDebugSerializer::PutNumber(const char* name, double value)
{
	m_Stream << INDENT << name << ": " << canonfloat(value, 17) << "\n";
}

void CDebugSerializer::PutNumber(const char* name, fixed value)
{
	m_Stream << INDENT << name << ": " << value.ToString() << "\n";
}

void CDebugSerializer::PutBool(const char* name, bool value)
{
	m_Stream << INDENT << name << ": " << (value ? "true" : "false") << "\n";
}

void CDebugSerializer::PutString(const char* name, const std::string& value)
{
	std::string escaped;
	escaped.reserve(value.size());
	for (size_t i = 0; i < value.size(); ++i)
		if (value[i] == '"')
			escaped += "\\\"";
		else if (value[i] == '\\')
			escaped += "\\\\";
		else if (value[i] == '\n')
			escaped += "\\n";
		else
			escaped += value[i];

	m_Stream << INDENT << name << ": " << "\"" << escaped << "\"\n";
}

void CDebugSerializer::PutScriptVal(const char* name, JS::MutableHandleValue value)
{
	std::wstring source = m_ScriptInterface.ToString(value, true);

	m_Stream << INDENT << name << ": " << utf8_from_wstring(source) << "\n";
}

void CDebugSerializer::PutRaw(const char* name, const u8* data, size_t len)
{
	m_Stream << INDENT << name << ": (" << len << " bytes)";

	char buf[4];
	for (size_t i = 0; i < len; ++i)
	{
		sprintf_s(buf, ARRAY_SIZE(buf), " %02x", (unsigned int)data[i]);
		m_Stream << buf;
	}

	m_Stream << "\n";
}

bool CDebugSerializer::IsDebug() const
{
	return m_IsDebug;
}

std::ostream& CDebugSerializer::GetStream()
{
	return m_Stream;
}
