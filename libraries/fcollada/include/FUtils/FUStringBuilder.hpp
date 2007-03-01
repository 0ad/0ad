/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include <limits>

#ifdef WIN32
#include <float.h>
#endif

#ifdef WIN32
#define ecvt _ecvt
#endif // WIN32

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(ptr) if (ptr != NULL) { delete [] ptr; ptr = NULL; }
#endif

template <class Char, class FloatType>
void FloatToString(FloatType f, Char* sz)
{
	Char* buffer = sz + 1;
	static const int digitCount = 6;
	int decimal, sign;
	char* end = ecvt(f, digitCount, &decimal, &sign);


	if (sign != 0) (*buffer++) = '-';
	int count = digitCount;
	if (decimal > 0)
	{
		for (int i = 0; i < decimal; ++i) (*buffer++) = (*end++);
		if (decimal < digitCount) (*buffer++) = '.';
		count = digitCount - decimal;
	}
	else if (decimal < -digitCount)
	{
		decimal = count = 0;
	}
	else if (decimal < 0 || (decimal == 0 && *end != '0'))
	{
		(*buffer++) = '0'; (*buffer++) = '.';
		for (int i = 0; i < -decimal; ++i) (*buffer++) = '0';
		count = digitCount + decimal;
	}
	for (; count >= 0; --count) if (end[count - 1] != '0') break;
	for (int i = 0; i < count; ++i) (*buffer++) = (*end++);
	if (decimal == 0 && count == 0) (*buffer++) = '0';
	if (buffer[-1] == '.') --buffer;
	(*buffer) = 0;
}

template <class Char>
FUStringBuilderT<Char>::FUStringBuilderT(const String& sz)
{
	this->buffer = NULL;
	this->size = 0;
	this->reserved = 0;
	
	reserve(sz.size() + 32);
	append(sz.c_str());
}

template <class Char>
FUStringBuilderT<Char>::FUStringBuilderT(const Char* sz)
{
	this->buffer = NULL;
	this->size = 0;
	this->reserved = 0;

	size_t len = 0;
	for (const Char* p = sz; *p != 0; ++p) ++len;
	reserve(len + 32);
	append(sz);
}

template <class Char>
FUStringBuilderT<Char>::FUStringBuilderT(Char ch, size_t count)
{
	this->buffer = NULL;
	this->size = 0;
	this->reserved = 0;

	reserve(count + 32);
	for (size_t i = 0; i < count; ++i) buffer[size++] = ch;
}

template <class Char>
FUStringBuilderT<Char>::FUStringBuilderT(size_t reservation)
{
	this->buffer = NULL;
	this->size = 0;
	this->reserved = 0;

	reserve(reservation);
}

template <class Char>
FUStringBuilderT<Char>::FUStringBuilderT()
{
	this->buffer = NULL;
	this->size = 0;
	this->reserved = 0;

#ifndef _DEBUG
	reserve(32);
#endif
}

template <class Char>
FUStringBuilderT<Char>::~FUStringBuilderT()
{
	reserve(0);
}

template <class Char>
void FUStringBuilderT<Char>::enlarge(size_t minimum)
{
	reserve(max(reserved + minimum + 32, 2 * reserved + 32));
}

template <class Char>
void FUStringBuilderT<Char>::clear()
{
	size = 0;
}

template <class Char>
void FUStringBuilderT<Char>::reserve(size_t _length)
{
	FUAssert(size <= reserved, );
	if (_length > reserved)
	{
		Char* b = new Char[_length];
		memcpy(b, buffer, size * sizeof(Char));
		SAFE_DELETE_ARRAY(buffer);
		buffer = b;
		reserved = _length;
	}
	else if (_length == 0)
	{
		SAFE_DELETE_ARRAY(buffer);
		size = reserved = 0;
	}
	else if (_length < reserved)
	{
		size_t realSize = min(size, _length);
		Char* b = new Char[_length];
		memcpy(b, buffer, realSize * sizeof(Char));
		SAFE_DELETE_ARRAY(buffer);
		buffer = b;
		reserved = _length;
		size = realSize;
	}
}

template <class Char>
void FUStringBuilderT<Char>::append(Char c)
{
	if (size + 1 >= reserved) enlarge(2);

	buffer[size++] = c;
}

template <class Char>
void FUStringBuilderT<Char>::append(const String& sz) { append(sz.c_str()); }
template <class Char>
void FUStringBuilderT<Char>::append(const Char* sz)
{
	// This is optimized for SMALL strings.
	for (; *sz != 0; ++sz)
	{
		if (size >= reserved) enlarge(64);
		buffer[size++] = *sz;
	}
}
template <class Char>
void FUStringBuilderT<Char>::append(const FUStringBuilderT& b)
{
	if (size + b.size >= reserved) enlarge(64 + size + b.size - reserved);
	memcpy(buffer + size, b.buffer, b.size * sizeof(Char));
	size += b.size;
}

template <class Char>
void FUStringBuilderT<Char>::append(float f)
{
#ifdef WIN32
	// use <float.h> _isnan method to detect the 1.#IND00 NaN.
	if (f != std::numeric_limits<float>::infinity() && f != -std::numeric_limits<float>::infinity() && f != std::numeric_limits<float>::quiet_NaN() && f != std::numeric_limits<float>::signaling_NaN() && !_isnan((double)f) )
#else
	if (f != std::numeric_limits<float>::infinity() && f != -std::numeric_limits<float>::infinity() && f != std::numeric_limits<float>::quiet_NaN() && f != std::numeric_limits<float>::signaling_NaN() )
#endif
	{
		if (IsEquivalent(f, 0.0f, std::numeric_limits<float>::epsilon())) append('0');
		else
		{
			Char sz[128];
			FloatToString(f, sz);
			append(sz + 1);
		}
	}
	else if (f == std::numeric_limits<float>::infinity())
	{ append('I'); append('N'); append('F'); }
	else if (f == -std::numeric_limits<float>::infinity())
	{ append('-'); append('I'); append('N'); append('F'); }
	else
	{ append('N'); append('a'); append('N'); }
}

template <class Char>
void FUStringBuilderT<Char>::append(double f)
{
#ifdef WIN32
	// use <float.h> _isnan method to detect the .#IND00 NaN.
	if (f != std::numeric_limits<float>::infinity() && f != -std::numeric_limits<float>::infinity() && f != std::numeric_limits<float>::quiet_NaN() && f != std::numeric_limits<float>::signaling_NaN() && !_isnan(f) )
#else
	if (f != std::numeric_limits<float>::infinity() && f != -std::numeric_limits<float>::infinity() && f != std::numeric_limits<float>::quiet_NaN() && f != std::numeric_limits<float>::signaling_NaN() )
#endif
	{
		if (IsEquivalent(f, 0.0, std::numeric_limits<double>::epsilon())) append('0');
		else
		{
			Char sz[128];
			FloatToString(f, sz);
			append(sz + 1);
		}
	}
	else if (f == std::numeric_limits<double>::infinity())
	{ append('I'); append('N'); append('F'); }
	else if (f == -std::numeric_limits<double>::infinity())
	{ append('-'); append('I'); append('N'); append('F'); }
	else
	{ append('N'); append('a'); append('N'); }
}

template <class Char>
void FUStringBuilderT<Char>::appendLine(const Char* sz)
{
	append(sz);
	append('\n');
}

template <class Char>
void FUStringBuilderT<Char>::appendHex(uint8 i)
{
	uint8 top = (i & 0xF0) >> 4;
	uint8 bot = i & 0xF;
	if (top <= 0x9) append((Char) ('0' + top));
	else append((Char) ('A' + (top - 0xA)));
	if (bot <= 0x9) append((Char) ('0' + bot));
	else append((Char) ('A' + (bot - 0xA)));
}

template <class Char>
void FUStringBuilderT<Char>::remove(int32 start)
{
	if ((int32)size > start && start >= 0) size = start;
}

template <class Char>
void FUStringBuilderT<Char>::remove(int32 start, int32 end)
{
	int32 diff = end - start;
	if ((int32)size >= end && start >= 0 && diff > 0)
	{
		const Char* stop = buffer + size - diff;
		for (Char* p = buffer + start; p != stop; ++p)
		{
			*p = *(p + diff);
		}
		size -= diff;
	}
}

template <class Char> 
const Char* FUStringBuilderT<Char>::ToCharPtr() const
{
	FUStringBuilderT<Char>* ncThis = const_cast< FUStringBuilderT<Char>* >(this);
	if (size + 1 > reserved) ncThis->enlarge(1);
	ncThis->buffer[size] = 0;
	return buffer;
}

template <class Char>
int32 FUStringBuilderT<Char>::index(Char c) const
{
	if (buffer != NULL && size > 0)
	{
		const Char* end = buffer + size + 1;
		for (const Char* p = buffer; p != end; ++p)
		{
			if (*p == c) return (int32)(p - buffer);
		}
	}
	return -1;
}

template <class Char>
int32 FUStringBuilderT<Char>::rindex(Char c) const
{
	if (buffer != NULL && size > 0)
	{
		for (const Char* p = buffer + size - 1; p != buffer; --p)
		{
			if (*p == c) return (int32)(p - buffer);
		}
	}
	return -1;
}
