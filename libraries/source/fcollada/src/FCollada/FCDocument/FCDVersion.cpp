/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDVersion.h"

//
// FCDVersion
//
FCDVersion::FCDVersion()
:	major(0), minor(0), revision(0)
{
}

FCDVersion::FCDVersion(const fm::string& v)
{
	ParseVersionNumbers(v);
}

FCDVersion::FCDVersion(uint32 _major, uint32 _minor, uint32 _revision)
:	major(_major), minor(_minor), revision(_revision)
{
}

void FCDVersion::ParseVersionNumbers(const fm::string& _v)
{
	const char* v = _v.c_str();
	major = FUStringConversion::ToUInt32(v);
	while (*v != 0 && *v != '.') { ++v; } if (*v != 0) ++v; // skip the '.'
	minor = FUStringConversion::ToUInt32(v);
	while (*v != 0 && *v != '.') { ++v; } if (*v != 0) ++v; // skip the '.'
	revision = FUStringConversion::ToUInt32(v);
}

bool IsEquivalent(const FCDVersion& a, const FCDVersion& b)
{
	return a.major == b.major && a.minor == b.minor && a.revision == b.revision;
}

bool FCDVersion::operator< (const FCDVersion& b) const
{
	if (major < b.major) return true;
	if (major > b.major) return false;
	if (minor < b.minor) return true;
	if (minor > b.minor) return false;
	return revision < b.revision;
}

bool FCDVersion::operator<= (const FCDVersion& b) const
{
	if (major < b.major) return true;
	if (major > b.major) return false;
	if (minor < b.minor) return true;
	if (minor > b.minor) return false;
	return revision <= b.revision;
}
