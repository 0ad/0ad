#include "precompiled.h"

#include "SynchedJSObject.h"
#include "Parser.h"
#include "ScriptCustomTypes.h"

template <>
CStrW ToNetString(const uint &val)
{
	return CStrW(val);
}

template <>
void SetFromNetString(uint &val, CStrW string)
{
	val=string.ToUInt();
}

template <>
CStrW ToNetString(const CStrW &data)
{	return data; }

template <> void SetFromNetString(CStrW &data, CStrW string)
{	data=string; }

template <>
CStrW ToNetString(const SColour &data)
{
	wchar_t buf[256];
	swprintf(buf, 256, L"%f %f %f %f", data.r, data.g, data.b, data.a);
	buf[255]=0;
	
	return CStrW(buf);
}

template <>
void SetFromNetString(SColour &data, CStrW wstring)
{
	CParser &parser(CParserCache::Get("$value_$value_$value_$value"));
	CParserLine line;
	
	line.ParseString(parser, CStr(wstring));
	
	float values[4];
	if (line.GetArgCount() != 4) return;
	for (uint i=0; i<4; ++i)
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

ISynchedJSProperty *CSynchedJSObjectBase::GetSynchedProperty(CStrW name)
{
	SynchedPropertyIterator prop=m_SynchedProperties.find(name);
	if (prop != m_SynchedProperties.end())
		return prop->second;
	else
		return NULL;
}

