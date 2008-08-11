#include "precompiled.h"

#include "SynchedJSObject.h"
#include "ps/Parser.h"
#include "ScriptCustomTypes.h"

template <>
CStrW ToNetString(const size_t &val)
{
	return CStrW(val);
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
	return val ? CStrW("true") : CStrW("false");
}

template <>
void SetFromNetString(bool &val, const CStrW& string)
{
	val = (string == CStrW("true"));
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
	
	return CStrW(buf);
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

