/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUStringConversion.h"
#ifndef __APPLE__
#include "FUStringConversion.hpp"
#endif // __APPLE__

//
// FUStringConversion
//

// Convert a UTF-8 string to a fstring
#ifdef UNICODE
	fstring FUStringConversion::ToFString(const char* value)
	{
		FUStringBuilder builder;
		if (value != NULL)
		{
			uint32 length = (uint32) strlen(value);
			builder.reserve(length + 1);
			for (uint32 i = 0; i < length; ++i)
			{
				builder.append((fchar) value[i]);
			}
		}
		return builder.ToString();
	}
#else // UNICODE
	fstring FUStringConversion::ToFString(const char* value)
	{
		return fstring(value);
	}
#endif // UNICODE

// Convert a fstring string to a UTF-8 string
#ifdef UNICODE
	fm::string FUStringConversion::ToString(const fchar* value)
	{
		FUSStringBuilder builder;
		if (value != NULL)
		{
			uint32 length = (uint32) fstrlen(value);
			builder.reserve(length + 1);
			for (uint32 i = 0; i < length; ++i)
			{
				if (value[i] < 0xFF || (value[i] & (~0xFF)) >= 32) builder.append((char)value[i]);
				else builder.append('_'); // some generic enough character
			}
		}
		return builder.ToString();
	}
#else // UNICODE
	fm::string FUStringConversion::ToString(const fchar* value)
	{
		return fm::string(value);
	}
#endif // UNICODE

fm::string FUStringConversion::ToString(const FMMatrix44& m)
{
	FUSStringBuilder builder;
	ToString(builder, m);
	return builder.ToString();
}

fm::string FUStringConversion::ToString(const FUDateTime& dateTime)
{
	char sz[21];
	snprintf(sz, 21, "%04u-%02u-%02uT%02u:%02u:%02uZ", (unsigned int) dateTime.GetYear(), (unsigned int) dateTime.GetMonth(), (unsigned int) dateTime.GetDay(), (unsigned int) dateTime.GetHour(), (unsigned int) dateTime.GetMinutes(), (unsigned) dateTime.GetSeconds());
	sz[20] = 0;
    return fm::string(sz);
}

fstring FUStringConversion::ToFString(const FMMatrix44& m)
{
    FUStringBuilder builder;
	ToString(builder, m);
	return builder.ToString();
}

fstring FUStringConversion::ToFString(const FUDateTime& dateTime)
{
	fchar sz[21];
	fsnprintf(sz, 21, FC("%04u-%02u-%02uT%02u:%02u:%02uZ"), (unsigned int) dateTime.GetYear(), (unsigned int) dateTime.GetMonth(), (unsigned int) dateTime.GetDay(), (unsigned int) dateTime.GetHour(), (unsigned int) dateTime.GetMinutes(), (unsigned int) dateTime.GetSeconds());	
	sz[20] = 0;
	return fstring(sz);
}

#ifdef HAS_VECTORTYPES

// Split a fstring into multiple substrings
void FUStringConversion::ToFStringList(const fstring& value, FStringList& array)
{
	const fchar* s = value.c_str();

	// Skip beginning white spaces
	fchar c;
	while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }

	size_t index = 0;
	while (*s != 0)
	{
		const fchar* word = s;

		// Find next white space
		while ((c = *s) != 0 && c != ' ' && c != '\t' && c != '\r' && c != '\n') { ++s; }

		if (index < array.size()) array[index++].append(word, s - word);
		else { array.push_back(fstring(word, s - word)); ++index; }

		// Skip all white spaces
		while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }
	}
	array.resize(index);
}

void FUStringConversion::ToStringList(const char* s, StringList& array)
{
	// Skip beginning white spaces
	char c;
	while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }

	size_t index = 0;
	while (*s != 0)
	{
		const char* word = s;

		// Find next white space
		while ((c = *s) != 0 && c != ' ' && c != '\t' && c != '\r' && c != '\n') { ++s; }

		if (index < array.size()) array[index++].append(word, s - word);
		else { array.push_back(fm::string(word, s - word)); ++index; }

		// Skip all white spaces
		while ((c = *s) != 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) { ++s; }
	}
	array.resize(index);
}

#ifdef UNICODE
void FUStringConversion::ToStringList(const fchar* value, StringList& array)
{
	// Performance could be improved...
	ToStringList(ToString(value), array);
}
#endif // UNICODE

#endif // HAS_VECTORTYPES

// Convert a 2D point to a string
fm::string FUStringConversion::ToString(const FMVector2& p)
{
	FUSStringBuilder builder;
	ToString(builder, p);
	return builder.ToString();
}

fstring FUStringConversion::ToFString(const FMVector2& p)
{
	FUStringBuilder builder;
	ToString(builder, p);
	return builder.ToString();
}

// Convert a point to a string
fm::string FUStringConversion::ToString(const FMVector3& p)
{
	FUSStringBuilder builder;
	ToString(builder, p);
	return builder.ToString();
}

// Convert a vector4 to a string
fm::string FUStringConversion::ToString(const FMVector4& p)
{
	FUSStringBuilder builder;
	ToString(builder, p);
	return builder.ToString();
}

fstring FUStringConversion::ToFString(const FMVector4& p)
{
	FUStringBuilder builder;
	ToString(builder, p);
	return builder.ToString();
}


// Convert a point to a fstring
fstring FUStringConversion::ToFString(const FMVector3& p)
{
	FUStringBuilder builder;
	ToString(builder, p);
	return builder.ToString();
}

// Split the target string into its pointer and its qualifier(s)
void FUStringConversion::SplitTarget(const fm::string& target, fm::string& pointer, fm::string& qualifier)
{
	size_t splitIndex = target.find_first_of("([.");
	if (splitIndex != fm::string::npos)
	{
		pointer = target.substr(0, splitIndex);
		qualifier = target.substr(splitIndex);
	}
	else
	{
		pointer = target;
		qualifier.clear();
	}
}

int32 FUStringConversion::ParseQualifier(const char* qualifier)
{
	int32 returnValue = -1;
	const char* c = qualifier;
	while (*c == '(' || *c == '[')
	{
		const char* number = ++c;
		while (*c >= '0' && *c <= '9') ++c;
		if (*c == ')' || *c == ']')
		{
			returnValue = FUStringConversion::ToInt32(number);
//			qualifier.erase(0, c + 1 - qualifier.c_str());
			break;
		}
	}
	return returnValue;
}

#ifndef _MSC_VER
template FMVector2 FUStringConversion::ToVector2<char>(const char**);
template FMVector3 FUStringConversion::ToVector3<char>(const char**);
template FMVector4 FUStringConversion::ToVector4<char>(const char**);

template bool FUStringConversion::ToBoolean<char>(const char*);
template unsigned int FUStringConversion::ToUInt32<char>(const char**);
template unsigned int FUStringConversion::HexToUInt32<char>(const char**, unsigned int);

template void FUStringConversion::ToBooleanList<char>(const char*, fm::vector<bool, true>&);
template void FUStringConversion::ToDateTime<char>(const char*, FUDateTime&);
template void FUStringConversion::ToFloatList<char>(const char*, fm::vector<float, true>&);
template void FUStringConversion::ToInt32List<char>(const char*, fm::vector<int, true>&);
template void FUStringConversion::ToInterleavedFloatList<char>(char const*, fm::pvector<fm::vector<float, true> >&);
template void FUStringConversion::ToInterleavedUInt32List<char>(char const*, fm::pvector<fm::vector<unsigned int, true> >&);
template void FUStringConversion::ToMatrix<char>(const char**, FMMatrix44&);
template void FUStringConversion::ToMatrixList<char>(const char*, fm::vector<FMMatrix44, false>&);
template void FUStringConversion::ToUInt32List<char>(const char*, fm::vector<unsigned int, true>&);
template void FUStringConversion::ToVector3List<char>(const char*, fm::vector<FMVector3, false>&);

template void FUStringConversion::ToString<char>(FUStringBuilderT<char>&, const FMVector2&);
template void FUStringConversion::ToString<char>(FUStringBuilderT<char>&, const FMVector3&);
template void FUStringConversion::ToString<char>(FUStringBuilderT<char>&, const FMVector4&);
template void FUStringConversion::ToString<char>(FUStringBuilderT<char>&, const FMMatrix44&);
template void FUStringConversion::ToString<char>(FUStringBuilderT<char>&, const fm::vector<float, true>&);
template void FUStringConversion::ToString<char>(FUStringBuilderT<char>&, const unsigned int*, size_t);
#endif

// Called by TrickLinker2 in FUStringBuilder.cpp
extern void TrickLinkerFUStringConversion(void)
{
	// Exercise the template functions in order to force the linker to generate and expose the methods.
	FUSStringBuilder sbuilder;
	FUStringBuilder fbuilder;
	const char* c = emptyCharString;
	const fchar* fc = emptyFCharString;
	float f = FUStringConversion::ToFloat(&c);
	f = FUStringConversion::ToFloat(&fc);
	bool b = FUStringConversion::ToBoolean(c);
	b = FUStringConversion::ToBoolean(fc);
	int32 i32 = FUStringConversion::ToInt32(&c);
	i32 = FUStringConversion::ToInt32(&fc);
	uint32 u32 = FUStringConversion::ToUInt32(&c);
	u32 = FUStringConversion::ToUInt32(&fc);
	u32 = FUStringConversion::HexToUInt32(&c);
	u32 = FUStringConversion::HexToUInt32(&fc);
	FMMatrix44 m44;
	FUStringConversion::ToMatrix(&c, m44);
	FUStringConversion::ToMatrix(&fc, m44);
	FUStringConversion::ToString(sbuilder, m44);
	FUStringConversion::ToString(fbuilder, m44);
	FUDateTime dt;
	FUStringConversion::ToDateTime(c, dt);
	FUStringConversion::ToDateTime(fc, dt);
	FMVector2 f2 = FUStringConversion::ToVector2(&c);
	FUStringConversion::ToString(sbuilder, f2);
	FUStringConversion::ToString(fbuilder, f2);
	f2 = FUStringConversion::ToVector2(&fc);
	FMVector3 f3 = FUStringConversion::ToVector3(&c);
	f3 = FUStringConversion::ToVector3(&fc);
	FUStringConversion::ToString(sbuilder, f3);
	FUStringConversion::ToString(fbuilder, f3);
	FMVector4 f4 = FUStringConversion::ToVector4(&c);
	f4 = FUStringConversion::ToVector4(&fc);
	FUStringConversion::ToString(sbuilder, f4);
	FUStringConversion::ToString(fbuilder, f4);

	BooleanList bl;
	FUStringConversion::ToBooleanList(c, bl);
	FUStringConversion::ToBooleanList(fc, bl);
	Int32List il;
	FUStringConversion::ToInt32List(c, il);
	FUStringConversion::ToInt32List(fc, il);
	FUStringConversion::ToString(fbuilder, il);
	FUStringConversion::ToString(sbuilder, il);
	UInt32List ul;
	FUStringConversion::ToUInt32List(c, ul);
	FUStringConversion::ToUInt32List(fc, ul);
	FUStringConversion::ToString(fbuilder, ul);
	FUStringConversion::ToString(sbuilder, ul);
	FloatList fl;
	FUStringConversion::ToFloatList(c, fl);
	FUStringConversion::ToFloatList(fc, fl);
	FUStringConversion::ToString(fbuilder, fl);
	FUStringConversion::ToString(sbuilder, fl);
	FMVector2List f2l;
	FUStringConversion::ToVector2List(c, f2l);
	FUStringConversion::ToVector2List(fc, f2l);
	FMVector3List f3l;
	FUStringConversion::ToVector3List(c, f3l);
	FUStringConversion::ToVector3List(fc, f3l);
	FMVector4List f4l;
	FUStringConversion::ToVector4List(c, f4l);
	FUStringConversion::ToVector4List(fc, f4l);
	FMMatrix44List m44l;
	FUStringConversion::ToMatrixList(c, m44l);
	FUStringConversion::ToMatrixList(fc, m44l);
	fm::pvector<FloatList> pfl;
	FUStringConversion::ToInterleavedFloatList(c, pfl);
	FUStringConversion::ToInterleavedFloatList(fc, pfl);
	fm::pvector<UInt32List> pul;
	FUStringConversion::ToInterleavedUInt32List(c, pul);
	FUStringConversion::ToInterleavedUInt32List(fc, pul);
}
