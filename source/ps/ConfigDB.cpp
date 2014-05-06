/* Copyright (C) 2013 Wildfire Games.
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

#include <boost/algorithm/string.hpp>

#include "Pyrogenesis.h"
#include "Parser.h"
#include "ConfigDB.h"
#include "CLogger.h"
#include "Filesystem.h"
#include "lib/allocators/shared_ptr.h"

typedef std::map <CStr, CConfigValueSet> TConfigMap;
TConfigMap CConfigDB::m_Map[CFG_LAST];
VfsPath CConfigDB::m_ConfigFile[CFG_LAST];

static pthread_mutex_t cfgdb_mutex = PTHREAD_MUTEX_INITIALIZER;
struct ScopedLock
{
	ScopedLock() { pthread_mutex_lock(&cfgdb_mutex); }
	~ScopedLock() { pthread_mutex_unlock(&cfgdb_mutex); }
};

CConfigDB::CConfigDB()
{
	// Recursive mutex needed for WriteFile
	pthread_mutexattr_t attr;
	int err;
	err = pthread_mutexattr_init(&attr);
	ENSURE(err == 0);
	err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	ENSURE(err == 0);
	err = pthread_mutex_init(&cfgdb_mutex, &attr);
	ENSURE(err == 0);
	err = pthread_mutexattr_destroy(&attr);
	ENSURE(err == 0);
}

#define GETVAL(T, type) \
	void CConfigDB::GetValue##T(EConfigNamespace ns, const CStr& name, type& value) \
	{ \
		if (ns < 0 || ns >= CFG_LAST) \
		{ \
			debug_warn(L"CConfigDB: Invalid ns value"); \
			return; \
		} \
		ScopedLock s; \
		TConfigMap::iterator it = m_Map[CFG_COMMAND].find(name); \
		if (it != m_Map[CFG_COMMAND].end()) \
		{ \
			it->second[0].Get##T(value); \
			return; \
		} \
		\
		for (int search_ns = ns; search_ns >= 0; search_ns--) \
		{ \
			it = m_Map[search_ns].find(name); \
			if (it != m_Map[search_ns].end()) \
			{ \
				it->second[0].Get##T(value); \
				return; \
			} \
		} \
	}

GETVAL(Bool, bool)
GETVAL(Int, int)
GETVAL(Float, float)
GETVAL(Double, double)
GETVAL(String, std::string)

#undef GETVAL

void CConfigDB::GetValues(EConfigNamespace ns, const CStr& name, CConfigValueSet& values)
{
	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return;
	}

	ScopedLock s;
	TConfigMap::iterator it = m_Map[CFG_COMMAND].find(name);
	if (it != m_Map[CFG_COMMAND].end())
	{
		values = it->second;
		return;
	}

	for (int search_ns = ns; search_ns >= 0; search_ns--)
	{
		it = m_Map[search_ns].find(name);
		if (it != m_Map[search_ns].end())
		{
			values = it->second;
			return;
		}
	}
}

EConfigNamespace CConfigDB::GetValueNamespace(EConfigNamespace ns, const CStr& name)
{
	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return CFG_LAST;
	}

	ScopedLock s;
	TConfigMap::iterator it = m_Map[CFG_COMMAND].find(name);
	if (it != m_Map[CFG_COMMAND].end())
		return CFG_COMMAND;

	for (int search_ns = ns; search_ns >= 0; search_ns--)
	{
		it = m_Map[search_ns].find(name);
		if (it != m_Map[search_ns].end())
			return (EConfigNamespace)search_ns;
	}

	return CFG_LAST;
}

std::map<CStr, CConfigValueSet> CConfigDB::GetValuesWithPrefix(EConfigNamespace ns, const CStr& prefix)
{
	ScopedLock s;
	std::map<CStr, CConfigValueSet> ret;

	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return ret;
	}

	// Loop upwards so that values in later namespaces can override
	// values in earlier namespaces
	for (int search_ns = 0; search_ns <= ns; search_ns++)
	{
		for (TConfigMap::iterator it = m_Map[search_ns].begin(); it != m_Map[search_ns].end(); ++it)
		{
			if (boost::algorithm::starts_with(it->first, prefix))
				ret[it->first] = it->second;
		}
	}

	for (TConfigMap::iterator it = m_Map[CFG_COMMAND].begin(); it != m_Map[CFG_COMMAND].end(); ++it)
	{
		if (boost::algorithm::starts_with(it->first, prefix))
			ret[it->first] = it->second;
	}

	return ret;
}

void CConfigDB::SetValueString(EConfigNamespace ns, const CStr& name, const CStr& value)
{
	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return;
	}

	ScopedLock s;
	TConfigMap::iterator it = m_Map[ns].find(name);
	if (it == m_Map[ns].end())
		it = m_Map[ns].insert(m_Map[ns].begin(), make_pair(name, CConfigValueSet(1)));

	it->second[0].m_String = value;
}

void CConfigDB::SetConfigFile(EConfigNamespace ns, const VfsPath& path)
{
	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return;
	}

	ScopedLock s;
	m_ConfigFile[ns]=path;
}

bool CConfigDB::Reload(EConfigNamespace ns)
{
	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return false;
	}

	ScopedLock s;

	// Set up CParser
	CParser parser;
	CParserLine parserLine;
	parser.InputTaskType("Assignment", "_$ident_=<_[-$arg(_minus)]_$value_,>_[-$arg(_minus)]_$value[[;]$rest]");
	parser.InputTaskType("CommentOrBlank", "_[;[$rest]]");

	// Open file with VFS
	shared_ptr<u8> buffer; size_t buflen;
	{
		// Handle missing files quietly
		if (g_VFS->GetFileInfo(m_ConfigFile[ns], NULL) < 0)
		{
			LOGMESSAGE(L"Cannot find config file \"%ls\" - ignoring", m_ConfigFile[ns].string().c_str());
			return false;
		}
		else
		{
			LOGMESSAGE(L"Loading config file \"%ls\"", m_ConfigFile[ns].string().c_str());
			Status ret = g_VFS->LoadFile(m_ConfigFile[ns], buffer, buflen);
			if (ret != INFO::OK)
			{
				LOGERROR(L"CConfigDB::Reload(): vfs_load for \"%ls\" failed: return was %lld", m_ConfigFile[ns].string().c_str(), (long long)ret);
				return false;
			}
		}
	}
	
	TConfigMap newMap;
	
	char *filebuf=(char *)buffer.get();
	char *filebufend=filebuf+buflen;
	
	// Read file line by line
	char *next=filebuf-1;
	do
	{
		char *pos=next+1;
		next=(char *)memchr(pos, '\n', filebufend-pos);
		if (!next) next=filebufend;

		char *lend=next;
		if (lend > filebuf && *(lend-1) == '\r') lend--;

		// Send line to parser
		bool parseOk=parserLine.ParseString(parser, std::string(pos, lend));
		// Get name and value from parser
		std::string name;
		std::string value;
		
		if (parseOk &&
			parserLine.GetArgCount()>=2 &&
			parserLine.GetArgString(0, name) &&
			parserLine.GetArgString(1, value))
		{
			// Add name and value to the map
			size_t argCount = parserLine.GetArgCount();

			newMap[name].clear();

			for( size_t t = 0; t < argCount; t++ )
			{
				if( !parserLine.GetArgString( (int)t + 1, value ) )
					continue;
				CConfigValue argument;
				argument.m_String = value;
				newMap[name].push_back( argument );
				if (name == "lobby.password")
					value = "*******";
				LOGMESSAGE(L"Loaded config string \"%hs\" = \"%hs\"", name.c_str(), value.c_str());
			}
		}
	}
	while (next < filebufend);
	
	m_Map[ns].swap(newMap);

	return true;
}

bool CConfigDB::WriteFile(EConfigNamespace ns)
{
	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return false;
	}

	ScopedLock s;
	return WriteFile(ns, m_ConfigFile[ns]);
}

bool CConfigDB::WriteFile(EConfigNamespace ns, const VfsPath& path)
{
	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return false;
	}

	ScopedLock s;
	shared_ptr<u8> buf;
	AllocateAligned(buf, 1*MiB, maxSectorSize);
	char* pos = (char*)buf.get();
	TConfigMap &map=m_Map[ns];
	for(TConfigMap::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		pos += sprintf(pos, "%s = \"%s\"\n", it->first.c_str(), it->second[0].m_String.c_str());
	}
	const size_t len = pos - (char*)buf.get();

	Status ret = g_VFS->CreateFile(path, buf, len);
	if(ret < 0)
	{
		LOGERROR(L"CConfigDB::WriteFile(): CreateFile \"%ls\" failed (error: %d)", path.string().c_str(), (int)ret);
		return false;
	}

	return true;
}
