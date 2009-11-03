/* Copyright (C) 2009 Wildfire Games.
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

#include "SynchedJSObject.h"
#include "ps/Parser.h"
#include "ScriptCustomTypes.h"

template <>
CStrW ToNetString(const size_t &val)
{
	return CStrW((unsigned long)val);
}

template <>
void SetFromNetString(size_t &val, const CStrW& string)
{
	val=string.ToUInt();
}

template <>
CStrW ToNetString(const int &val)
{
	return CStrW(val);
}

template <>
void SetFromNetString(int &val, const CStrW& string)
{
	val=string.ToInt();
}

template <>
CStrW ToNetString(const bool &val)
{
	return val ? L"true" : L"false";
}

template <>
void SetFromNetString(bool &val, const CStrW& string)
{
	val = (string == L"true");
}

template <>
CStrW ToNetString(const CStrW& data)
{	
	return data; 
}

template <> void SetFromNetString(CStrW& data, const CStrW& string)
{	
	data=string; 
}

template <>
CStrW ToNetString(const SColour &data)
{
	wchar_t buf[256];
	swprintf(buf, 256, L"%f %f %f %f", data.r, data.g, data.b, data.a);
	buf[255]=0;
	
	return buf;
}

template <>
void SetFromNetString(SColour &data, const CStrW& wstring)
{
	CParser &parser(CParserCache::Get("$value_$value_$value_$value"));
	CParserLine line;
	
	line.ParseString(parser, CStr(wstring));
	
	float values[4];
	if (line.GetArgCount() != 4) return;
	for (size_t i=0; i<4; ++i)
	{
		if (!line.GetArgFloat(i, values[i]))
		{
			return;
		}
	}

	data.r = values[0];
	data.g = values[1];
	data.b = values[2];
	data.a = values[3];
}

void CSynchedJSObjectBase::IterateSynchedProperties(IterateCB *cb, void *userdata)
{
	SynchedPropertyIterator it=m_SynchedProperties.begin();
	while (it != m_SynchedProperties.end())
	{
		cb(it->first, it->second, userdata);
		++it;
	}
}

ISynchedJSProperty *CSynchedJSObjectBase::GetSynchedProperty(const CStrW& name)
{
	SynchedPropertyIterator prop=m_SynchedProperties.find(name);
	if (prop != m_SynchedProperties.end())
		return prop->second;
	else
		return NULL;
}

