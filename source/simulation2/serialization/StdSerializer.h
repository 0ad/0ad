/* Copyright (C) 2015 Wildfire Games.
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

#ifndef INCLUDED_STDSERIALIZER
#define INCLUDED_STDSERIALIZER

#include "BinarySerializer.h"

#include <cstring>

#define DEBUG_SERIALIZER_ANNOTATE 0 // annotate the stream to help debugging if you're reading the output in a hex editor

class CStdSerializerImpl
{
	NONCOPYABLE(CStdSerializerImpl);
public:
	CStdSerializerImpl(std::ostream& stream);

	~CStdSerializerImpl()
	{
		m_Stream.flush();
	}

	void Put(const char* name, const u8* data, size_t len)
	{
#if DEBUG_SERIALIZER_ANNOTATE
		m_Stream.put('<');
		m_Stream.write(name, strlen(name));
		m_Stream.put('>');
#else
		UNUSED2(name);
#endif
		m_Stream.write((const char*)data, (std::streamsize)len);
	}

	std::ostream& GetStream()
	{
		return m_Stream;
	}

private:
	std::ostream& m_Stream;
};

class CStdSerializer : public CBinarySerializer<CStdSerializerImpl>
{
public:
	CStdSerializer(ScriptInterface& scriptInterface, std::ostream& stream);

	virtual std::ostream& GetStream();
};

#endif // INCLUDED_STDSERIALIZER
