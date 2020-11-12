/* Copyright (C) 2020 Wildfire Games.
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

#include "lib/byte_order.h"
#include "lib/allocators/arena.h"

#include <map>

/**
 * Wrapper for redirecting ostream writes to CBinarySerializer's impl
 */
template<typename T>
class CSerializerStreamBuf : public std::streambuf
{
	NONCOPYABLE(CSerializerStreamBuf);
	T& m_SerializerImpl;
	char m_Buffer[2048];
public:
	CSerializerStreamBuf(T& impl) :
		m_SerializerImpl(impl)
	{
		setp(m_Buffer, m_Buffer + ARRAY_SIZE(m_Buffer) - 1);
	}

protected:
	// Override overflow and sync, because older versions of libc++ streams
	// write strings as individual characters, then xsputn is never called
	int overflow(int ch)
	{
		if (ch == traits_type::eof())
			return traits_type::not_eof(ch);

		ENSURE(pptr() <= epptr());
		*pptr() = ch;
		pbump(1);
		sync();
		return ch;
	}

	int sync()
	{
		std::ptrdiff_t n = pptr() - pbase();
		if (n != 0)
		{
			pbump(-n);
			m_SerializerImpl.Put("stream", reinterpret_cast<const u8*> (pbase()), n);
		}
		return 0;
	}

	std::streamsize xsputn(const char* s, std::streamsize n)
	{
		m_SerializerImpl.Put("stream", reinterpret_cast<const u8*> (s), n);
		return n;
	}
};

/**
 * PutScriptVal implementation details.
 * (Split out from the main class because it's too big to be inlined.)
 */
class CBinarySerializerScriptImpl
{
public:
	CBinarySerializerScriptImpl(const ScriptInterface& scriptInterface, ISerializer& serializer);

	void ScriptString(const char* name, JS::HandleString string);
	void HandleScriptVal(JS::HandleValue val);
private:
	const ScriptInterface& m_ScriptInterface;
	ISerializer& m_Serializer;

	JS::PersistentRootedSymbol m_ScriptBackrefSymbol;
	i32 m_ScriptBackrefsNext;
	i32 GetScriptBackrefTag(JS::HandleObject obj);
};

/**
 * Serialize to a binary stream. T must just implement the Put() method.
 * (We use this templated approach to allow compiler inlining.)
 */
template <typename T>
class CBinarySerializer : public ISerializer
{
	NONCOPYABLE(CBinarySerializer);
public:
	CBinarySerializer(const ScriptInterface& scriptInterface) :
		m_ScriptImpl(new CBinarySerializerScriptImpl(scriptInterface, *this)),
		m_RawStreamBuf(m_Impl),
		m_RawStream(&m_RawStreamBuf)
	{
	}

	template <typename A>
	CBinarySerializer(const ScriptInterface& scriptInterface, A& a) :
		m_ScriptImpl(new CBinarySerializerScriptImpl(scriptInterface, *this)),
		m_Impl(a),
		m_RawStreamBuf(m_Impl),
		m_RawStream(&m_RawStreamBuf)
	{
	}

protected:
	/*
	The Put* implementations here are designed for subclasses
	that want an efficient, portable, deserializable representation.
	(Subclasses with different requirements should override these methods.)

	Numbers are converted to little-endian byte strings, for portability
	and efficiency.

	Data is not aligned, for storage efficiency.
	*/

	virtual void PutNumber(const char* name, uint8_t value)
	{
		m_Impl.Put(name, (const u8*)&value, sizeof(uint8_t));
	}

	virtual void PutNumber(const char* name, int8_t value)
	{
		m_Impl.Put(name, (const u8*)&value, sizeof(int8_t));
	}

	virtual void PutNumber(const char* name, uint16_t value)
	{
		uint16_t v = to_le16(value);
		m_Impl.Put(name, (const u8*)&v, sizeof(uint16_t));
	}

	virtual void PutNumber(const char* name, int16_t value)
	{
		int16_t v = (i16)to_le16((u16)value);
		m_Impl.Put(name, (const u8*)&v, sizeof(int16_t));
	}

	virtual void PutNumber(const char* name, uint32_t value)
	{
		uint32_t v = to_le32(value);
		m_Impl.Put(name, (const u8*)&v, sizeof(uint32_t));
	}

	virtual void PutNumber(const char* name, int32_t value)
	{
		int32_t v = (i32)to_le32((u32)value);
		m_Impl.Put(name, (const u8*)&v, sizeof(int32_t));
	}

	virtual void PutNumber(const char* name, float value)
	{
		m_Impl.Put(name, (const u8*)&value, sizeof(float));
	}

	virtual void PutNumber(const char* name, double value)
	{
		m_Impl.Put(name, (const u8*)&value, sizeof(double));
	}

	virtual void PutNumber(const char* name, fixed value)
	{
		int32_t v = (i32)to_le32((u32)value.GetInternalValue());
		m_Impl.Put(name, (const u8*)&v, sizeof(int32_t));
	}

	virtual void PutBool(const char* name, bool value)
	{
		NumberU8(name, value ? 1 : 0, 0, 1);
	}

	virtual void PutString(const char* name, const std::string& value)
	{
		// TODO: maybe should intern strings, particularly to save space with script property names
		PutNumber("string length", (uint32_t)value.length());
		m_Impl.Put(name, (u8*)value.data(), value.length());
	}

	virtual void PutScriptVal(const char* UNUSED(name), JS::MutableHandleValue value)
	{
		m_ScriptImpl->HandleScriptVal(value);
	}

	virtual void PutRaw(const char* name, const u8* data, size_t len)
	{
		m_Impl.Put(name, data, len);
	}

	virtual std::ostream& GetStream()
	{
		return m_RawStream;
	}

protected:
	T m_Impl;

private:
	std::unique_ptr<CBinarySerializerScriptImpl> m_ScriptImpl;

	CSerializerStreamBuf<T> m_RawStreamBuf;
	std::ostream m_RawStream;
};

#endif // INCLUDED_BINARYSERIALIZER
