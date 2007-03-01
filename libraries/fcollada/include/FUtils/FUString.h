/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FUString.h
	This file includes FUStringBuilder.h and FUStringConversion.h
	and defines important string-related macros and inline functions.
*/

#ifndef _FU_STRING_H_
#define _FU_STRING_H_

/** A dynamically-sized array of Unicode strings. */
typedef fm::vector<fstring> FStringList;

/** A dynamically-sized array of simple strings. */
typedef fm::vector<fm::string> StringList;

/** Returns whether two 8-bit strings are equivalent. This is a case-sensitive comparison.
	@param sz1 The first 8-bit string to compare.
	@param sz2 The second 8-bit string to compare.
	@return Whether the two 8-bit strings are equivalent. */
inline bool IsEquivalent(const char* sz1, const char* sz2) { return strcmp(sz1, sz2) == 0; }
inline bool IsEquivalent(const fm::string& sz1, const char* sz2) { return strcmp(sz1.c_str(), sz2) == 0; } /**< See above. */
inline bool IsEquivalent(const char* sz1, const fm::string& sz2) { return strcmp(sz1, sz2.c_str()) == 0; } /**< See above. */
inline bool IsEquivalent(const fm::string& sz1, const fm::string& sz2) { return strcmp(sz1.c_str(), sz2.c_str()) == 0; } /**< See above. */

/** Returns whether two 8-bit strings are equivalent. This is a case-insensitive comparison.
	@param sz1 The first 8-bit string to compare.
	@param sz2 The second 8-bit string to compare.
	@return Whether the two 8-bit strings are equivalent. */
inline bool IsEquivalentI(const char* sz1, const char* sz2) { return _stricmp(sz1, sz2) == 0; }
inline bool IsEquivalentI(const fm::string& sz1, const char* sz2) { return _stricmp(sz1.c_str(), sz2) == 0; } /**< See above. */
inline bool IsEquivalentI(const char* sz1, const fm::string& sz2) { return _stricmp(sz1, sz2.c_str()) == 0; } /**< See above. */
inline bool IsEquivalentI(const fm::string& sz1, const fm::string& sz2) { return _stricmp(sz1.c_str(), sz2.c_str()) == 0; } /**< See above. */
#ifdef UNICODE
inline bool IsEquivalentI(const fchar* sz1, const fchar* sz2) { return fstricmp(sz1, sz2) == 0; } /**< See above. */
inline bool IsEquivalentI(const fstring& sz1, const fchar* sz2) { return fstricmp(sz1.c_str(), sz2) == 0; } /**< See above. */
inline bool IsEquivalentI(const fchar* sz1, const fstring& sz2) { return fstricmp(sz1, sz2.c_str()) == 0; } /**< See above. */
inline bool IsEquivalentI(const fstring& sz1, const fstring& sz2) { return fstricmp(sz1.c_str(), sz2.c_str()) == 0; } /**< See above. */
#endif // UNICODE

/** Returns whether two 8-bit strings are equivalent. This is a case-sensitive comparison.
	@param sz1 The first 8-bit string to compare.
	@param sz2 The second 8-bit string to compare.
	@return Whether the two 8-bit strings are equivalent. */
inline bool operator==(const fm::string& sz1, const char* sz2) { return strcmp(sz1.c_str(), sz2) == 0; }

/** Appends a signed integer to an 8-bit string. This function is meant
	for debugging purposes. Use the FUStringBuilder class instead.
	@param sz1 The 8-bit string prefix.
	@param i The signed integer to convert and append.
	@return The final 8-bit string. A static variable is returned and
		may be overwritten at a later time. If you are interested
		in this value, copy it to a local string. */
FCOLLADA_EXPORT const fm::string& operator+(const fm::string& sz1, int32 i);

#ifdef UNICODE
/** Returns whether two Unicode strings are equivalent. This is a case-sensitive comparison.
	@param sz1 The first Unicode string to compare.
	@param sz2 The second Unicode string to compare.
	@return Whether the two Unicode strings are equivalent. */
inline bool IsEquivalent(const fchar* sz1, const fchar* sz2) { return fstrcmp(sz1, sz2) == 0; }
inline bool IsEquivalent(const fstring& sz1, const fchar* sz2) { return fstrcmp(sz1.c_str(), sz2) == 0; } /**< See above. */
inline bool IsEquivalent(const fchar* sz1, const fstring& sz2) { return fstrcmp(sz1, sz2.c_str()) == 0; } /**< See above. */
inline bool IsEquivalent(const fstring& sz1, const fstring& sz2) { return fstrcmp(sz1.c_str(), sz2.c_str()) == 0; } /**< See above. */

/** Returns whether two Unicode strings are equivalent. This is a case-sensitive comparison.
	@param sz1 The first Unicode string to compare.
	@param sz2 The second Unicode string to compare.
	@return Whether the two Unicode strings are equivalent. */
inline bool operator==(const fstring& sz1, const fchar* sz2) { return fstrcmp(sz1.c_str(), sz2) == 0; }

/** Appends a signed integer to a Unicode string. This function is meant
	for debugging purposes. Use the FUStringBuilder class instead.
	@param sz1 The Unicode string prefix.
	@param i The signed integer to convert and append.
	@return The final Unicode string. A static variable is returned and
		may be overwritten at a later time. If you are interested
		in this value, copy it to a local string. */
FCOLLADA_EXPORT const fstring& operator+(const fstring& sz1, int32 i);
#endif // UNICODE

// Include the main string modification classes.
#include "FUtils/FUStringBuilder.h"
#include "FUtils/FUStringConversion.h"

/** A Unicode string from a constant 8-bit string. */
#define FS(a) fstring(FC(a))
/** A Unicode string from any convertable value: string, vector-type or simple numeric. */
#define TO_FSTRING(a) FUStringConversion::ToFString(a)
/** An 8-bit string from any convertable value: Unicode string, vector-type or simple numeric. */
#define TO_STRING(a) FUStringConversion::ToString(a)

/** An empty UTF-8 string. This string is returned in many functions when there is an error. */
extern FCOLLADA_EXPORT const fm::string emptyString;
/** An empty Unicode string. This string is returned in many functions when there is an error. */
extern FCOLLADA_EXPORT const fstring emptyFString;

/** Returns whether a string builder and a string are equivalent. This is a case-sensitive comparison.
	@param builder The string builder to compare.
	@param sz The string to compare.
	@return Whether the two strings are equivalent. */
inline bool IsEquivalent(FUSStringBuilder& builder, const char* sz) { return IsEquivalent(builder.ToCharPtr(), sz); }
inline bool IsEquivalent(FUSStringBuilder& builder, const fm::string& sz) { return IsEquivalent(builder.ToCharPtr(), sz.c_str()); } /**< See above. */
#ifdef UNICODE
inline bool IsEquivalent(FUStringBuilder& builder, const fchar* sz) { return IsEquivalent(builder.ToCharPtr(), sz); } /**< See above. */
inline bool IsEquivalent(FUStringBuilder& builder, const fstring& sz) { return IsEquivalent(builder.ToCharPtr(), sz.c_str()); } /**< See above. */
#endif

#endif // _FU_STRING_H_
