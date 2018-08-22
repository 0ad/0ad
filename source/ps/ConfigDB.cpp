/* Copyright (C) 2018 Wildfire Games.
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

#include "ConfigDB.h"

#include <boost/algorithm/string.hpp>

#include "lib/allocators/shared_ptr.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Filesystem.h"
#include "ps/ThreadUtil.h"

#include <unordered_set>

typedef std::map<CStr, CConfigValueSet> TConfigMap;
TConfigMap CConfigDB::m_Map[CFG_LAST];
VfsPath CConfigDB::m_ConfigFile[CFG_LAST];
bool CConfigDB::m_HasChanges[CFG_LAST];

static pthread_mutex_t cfgdb_mutex = PTHREAD_MUTEX_INITIALIZER;

// These entries will not be printed to logfiles, so that logfiles can be shared without leaking personal or sensitive data
static const std::unordered_set<std::string> g_UnloggedEntries = {
	"lobby.password",
	"lobby.buddies",
	"userreport.id" // authentication token for GDPR personal data requests
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

#define CHECK_NS(rval)\
	do {\
		if (ns < 0 || ns >= CFG_LAST)\
		{\
			debug_warn(L"CConfigDB: Invalid ns value");\
			return rval;\
		}\
	} while (false)

namespace {
template<typename T> void Get(const CStr& value, T& ret)
{
	std::stringstream ss(value);
	ss >> ret;
}
template<> void Get<>(const CStr& value, bool& ret)
{
	ret = value == "true";
}
template<> void Get<>(const CStr& value, std::string& ret)
{
	ret = value;
}
std::string EscapeString(const CStr& str)
{
	std::string ret;
	for (size_t i = 0; i < str.length(); ++i)
	{
		if (str[i] == '\\')
			ret += "\\\\";
		else if (str[i] == '"')
			ret += "\\\"";
		else
			ret += str[i];
	}
	return ret;
}
} // namespace

#define GETVAL(type)\
	void CConfigDB::GetValue(EConfigNamespace ns, const CStr& name, type& value)\
	{\
		CHECK_NS(;);\
		CScopeLock s(&cfgdb_mutex);\
		TConfigMap::iterator it = m_Map[CFG_COMMAND].find(name);\
		if (it != m_Map[CFG_COMMAND].end())\
		{\
			Get(it->second[0], value);\
			return;\
		}\
		for (int search_ns = ns; search_ns >= 0; --search_ns)\
		{\
			it = m_Map[search_ns].find(name);\
			if (it != m_Map[search_ns].end())\
			{\
				Get(it->second[0], value);\
				return;\
			}\
		}\
	}
GETVAL(bool)
GETVAL(int)
GETVAL(u32)
GETVAL(float)
GETVAL(double)
GETVAL(std::string)
#undef GETVAL

bool CConfigDB::HasChanges(EConfigNamespace ns) const
{
	CHECK_NS(false);

	CScopeLock s(&cfgdb_mutex);
	return m_HasChanges[ns];
}

void CConfigDB::SetChanges(EConfigNamespace ns, bool value)
{
	CHECK_NS(;);

	CScopeLock s(&cfgdb_mutex);
	m_HasChanges[ns] = value;
}

void CConfigDB::GetValues(EConfigNamespace ns, const CStr& name, CConfigValueSet& values) const
{
	CHECK_NS(;);

	CScopeLock s(&cfgdb_mutex);
	TConfigMap::iterator it = m_Map[CFG_COMMAND].find(name);
	if (it != m_Map[CFG_COMMAND].end())
	{
		values = it->second;
		return;
	}

	for (int search_ns = ns; search_ns >= 0; --search_ns)
	{
		it = m_Map[search_ns].find(name);
		if (it != m_Map[search_ns].end())
		{
			values = it->second;
			return;
		}
	}
}

EConfigNamespace CConfigDB::GetValueNamespace(EConfigNamespace ns, const CStr& name) const
{
	CHECK_NS(CFG_LAST);

	CScopeLock s(&cfgdb_mutex);
	TConfigMap::iterator it = m_Map[CFG_COMMAND].find(name);
	if (it != m_Map[CFG_COMMAND].end())
		return CFG_COMMAND;

	for (int search_ns = ns; search_ns >= 0; --search_ns)
	{
		it = m_Map[search_ns].find(name);
		if (it != m_Map[search_ns].end())
			return (EConfigNamespace)search_ns;
	}

	return CFG_LAST;
}

std::map<CStr, CConfigValueSet> CConfigDB::GetValuesWithPrefix(EConfigNamespace ns, const CStr& prefix) const
{
	CScopeLock s(&cfgdb_mutex);
	std::map<CStr, CConfigValueSet> ret;

	CHECK_NS(ret);

	// Loop upwards so that values in later namespaces can override
	// values in earlier namespaces
	for (int search_ns = 0; search_ns <= ns; ++search_ns)
		for (const std::pair<CStr, CConfigValueSet>& p : m_Map[search_ns])
			if (boost::algorithm::starts_with(p.first, prefix))
				ret[p.first] = p.second;

	for (const std::pair<CStr, CConfigValueSet>& p : m_Map[CFG_COMMAND])
		if (boost::algorithm::starts_with(p.first, prefix))
			ret[p.first] = p.second;

	return ret;
}

void CConfigDB::SetValueString(EConfigNamespace ns, const CStr& name, const CStr& value)
{
	CHECK_NS(;);

	CScopeLock s(&cfgdb_mutex);
	TConfigMap::iterator it = m_Map[ns].find(name);
	if (it == m_Map[ns].end())
		it = m_Map[ns].insert(m_Map[ns].begin(), make_pair(name, CConfigValueSet(1)));

	it->second[0] = value;
}

void CConfigDB::SetValueBool(EConfigNamespace ns, const CStr& name, const bool value)
{
	CStr valueString = value ? "true" : "false";
	SetValueString(ns, name, valueString);
}

void CConfigDB::RemoveValue(EConfigNamespace ns, const CStr& name)
{
	CHECK_NS(;);

	CScopeLock s(&cfgdb_mutex);
	TConfigMap::iterator it = m_Map[ns].find(name);
	if (it == m_Map[ns].end())
		return;
	m_Map[ns].erase(it);
}

void CConfigDB::SetConfigFile(EConfigNamespace ns, const VfsPath& path)
{
	CHECK_NS(;);

	CScopeLock s(&cfgdb_mutex);
	m_ConfigFile[ns] = path;
}

bool CConfigDB::Reload(EConfigNamespace ns)
{
	CHECK_NS(false);

	CScopeLock s(&cfgdb_mutex);

	shared_ptr<u8> buffer;
	size_t buflen;
	{
		// Handle missing files quietly
		if (g_VFS->GetFileInfo(m_ConfigFile[ns], NULL) < 0)
		{
			LOGMESSAGE("Cannot find config file \"%s\" - ignoring", m_ConfigFile[ns].string8());
			return false;
		}

		LOGMESSAGE("Loading config file \"%s\"", m_ConfigFile[ns].string8());
		Status ret = g_VFS->LoadFile(m_ConfigFile[ns], buffer, buflen);
		if (ret != INFO::OK)
		{
			LOGERROR("CConfigDB::Reload(): vfs_load for \"%s\" failed: return was %lld", m_ConfigFile[ns].string8(), (long long)ret);
			return false;
		}
	}

	TConfigMap newMap;
	char *filebuf = (char*)buffer.get();
	char *filebufend = filebuf+buflen;

	bool quoted = false;
	CStr header;
	CStr name;
	CStr value;
	int line = 1;
	std::vector<CStr> values;
	for (char* pos = filebuf; pos < filebufend; ++pos)
	{
		switch (*pos)
		{
		case '\n':
		case ';':
			break; // We finished parsing this line

		case ' ':
		case '\r':
		case '\t':
			continue; // ignore

		case '[':
			header.clear();
			for (++pos; pos < filebufend && *pos != '\n' && *pos != ']'; ++pos)
				header.push_back(*pos);

			if (pos == filebufend || *pos == '\n')
			{
				LOGERROR("Config header with missing close tag encountered on line %d in '%s'", line, m_ConfigFile[ns].string8());
				header.clear();
				++line;
				continue;
			}

			LOGMESSAGE("Found config header '%s'", header.c_str());
			header.push_back('.');
			while (++pos < filebufend && *pos != '\n' && *pos != ';')
				if (*pos != ' ' && *pos != '\r')
				{
					LOGERROR("Config settings on the same line as a header on line %d in '%s'", line, m_ConfigFile[ns].string8());
					break;
				}
			while (pos < filebufend && *pos != '\n')
				++pos;
			++line;
			continue;

		case '=':
			// Parse parameters (comma separated, possibly quoted)
			for (++pos; pos < filebufend && *pos != '\n' && *pos != ';'; ++pos)
			{
				switch (*pos)
				{
				case '"':
					quoted = true;
					// parse until not quoted anymore
					for (++pos; pos < filebufend && *pos != '\n' && *pos != '"'; ++pos)
					{
						if (*pos == '\\' && ++pos == filebufend)
						{
							LOGERROR("Escape character at end of input (line %d in '%s')", line, m_ConfigFile[ns].string8());
							break;
						}

						value.push_back(*pos);
					}
					if (pos < filebufend && *pos == '"')
						quoted = false;
					else
						--pos; // We should terminate the outer loop too
					break;

				case ' ':
				case '\r':
				case '\t':
					break; // ignore

				case ',':
					if (!value.empty())
						values.push_back(value);
					value.clear();
					break;

				default:
					value.push_back(*pos);
					break;
				}
			}
			if (quoted) // We ignore the invalid parameter
				LOGERROR("Unmatched quote while parsing config file '%s' on line %d", m_ConfigFile[ns].string8(), line);
			else if (!value.empty())
				values.push_back(value);
			value.clear();
			quoted = false;
			break; // We are either at the end of the line, or we still have a comment to parse

		default:
			name.push_back(*pos);
			continue;
		}

		// Consume the rest of the line
		while (pos < filebufend && *pos != '\n')
			++pos;
		// Store the setting
		if (!name.empty() && !values.empty())
		{
			CStr key(header + name);
			newMap[key] = values;
			if (g_UnloggedEntries.find(key) != g_UnloggedEntries.end())
				LOGMESSAGE("Loaded config string \"%s\"", key);
			else
			{
				std::string vals;
				for (size_t i = 0; i < newMap[key].size() - 1; ++i)
					vals += "\"" + EscapeString(newMap[key][i]) + "\", ";
				vals += "\"" + EscapeString(newMap[key][values.size()-1]) + "\"";
				LOGMESSAGE("Loaded config string \"%s\" = %s", key, vals);
			}
		}
		else if (!name.empty())
			LOGERROR("Encountered config setting '%s' without value while parsing '%s' on line %d", name, m_ConfigFile[ns].string8(), line);

		name.clear();
		values.clear();
		++line;
	}

	if (!name.empty())
		LOGERROR("Config file does not have a new line after the last config setting '%s'", name);

	m_Map[ns].swap(newMap);

	return true;
}

bool CConfigDB::WriteFile(EConfigNamespace ns) const
{
	CHECK_NS(false);

	CScopeLock s(&cfgdb_mutex);
	return WriteFile(ns, m_ConfigFile[ns]);
}

bool CConfigDB::WriteFile(EConfigNamespace ns, const VfsPath& path) const
{
	CHECK_NS(false);

	CScopeLock s(&cfgdb_mutex);
	shared_ptr<u8> buf;
	AllocateAligned(buf, 1*MiB, maxSectorSize);
	char* pos = (char*)buf.get();
	for (const std::pair<CStr, CConfigValueSet>& p : m_Map[ns])
	{
		size_t i;
		pos += sprintf(pos, "%s = ", p.first.c_str());
		for (i = 0; i < p.second.size() - 1; ++i)
			pos += sprintf(pos, "\"%s\", ", EscapeString(p.second[i]).c_str());
		pos += sprintf(pos, "\"%s\"\n", EscapeString(p.second[i]).c_str());
	}
	const size_t len = pos - (char*)buf.get();

	Status ret = g_VFS->CreateFile(path, buf, len);
	if (ret < 0)
	{
		LOGERROR("CConfigDB::WriteFile(): CreateFile \"%s\" failed (error: %d)", path.string8(), (int)ret);
		return false;
	}

	return true;
}

bool CConfigDB::WriteValueToFile(EConfigNamespace ns, const CStr& name, const CStr& value)
{
	CHECK_NS(false);

	CScopeLock s(&cfgdb_mutex);
	return WriteValueToFile(ns, name, value, m_ConfigFile[ns]);
}

bool CConfigDB::WriteValueToFile(EConfigNamespace ns, const CStr& name, const CStr& value, const VfsPath& path)
{
	CHECK_NS(false);

	CScopeLock s(&cfgdb_mutex);

	TConfigMap newMap;
	m_Map[ns].swap(newMap);
	Reload(ns);

	SetValueString(ns, name, value);
	bool ret = WriteFile(ns, path);
	m_Map[ns].swap(newMap);
	return ret;
}

#undef CHECK_NS
