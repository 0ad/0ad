#include "precompiled.h"

#include "Prometheus.h"
#include "Parser.h"
#include "ConfigDB.h"
#include "CLogger.h"
#include "res/vfs.h"
#include "res/file.h"

using namespace std;

typedef map <CStr, CConfigValue> TConfigMap;
TConfigMap CConfigDB::m_Map[CFG_LAST];
CStr CConfigDB::m_ConfigFile[CFG_LAST];
bool CConfigDB::m_UseVFS[CFG_LAST];

CConfigValue *CConfigDB::GetValue(EConfigNamespace ns, CStr name)
{
	assert(ns < CFG_LAST && ns >= 0);
	
	TConfigMap::iterator it=m_Map[ns].find(name);
	if (it == m_Map[ns].end())
		return NULL;
	else
		return &(it->second);
}

CConfigValue *CConfigDB::CreateValue(EConfigNamespace ns, CStr name)
{
	assert(ns < CFG_LAST && ns >= 0);
	
	CConfigValue *ret=GetValue(ns, name);
	if (ret) return ret;
	
	TConfigMap::iterator it=m_Map[ns].insert(m_Map[ns].begin(), make_pair(name, CConfigValue()));
	return &(it->second);
}

void CConfigDB::SetConfigFile(EConfigNamespace ns, bool useVFS, CStr path)
{
	assert(ns < CFG_LAST && ns >= 0);
	
	m_ConfigFile[ns]=path;
	m_UseVFS[ns]=useVFS;
}

bool CConfigDB::Reload(EConfigNamespace ns)
{
	// Set up CParser
	CParser parser;
	CParserLine parserLine;
	parser.InputTaskType("Assignment", "_$ident_=_[-$arg(_minus)]_$value_[;$rest]");
	parser.InputTaskType("Comment", "_;$rest");

	void *buffer;
	uint buflen;
	File f;
	Handle fh;
	if (m_UseVFS[ns])
	{
		// Open file with VFS
		fh=vfs_load(m_ConfigFile[ns], buffer, buflen);
		if (fh <= 0)
		{
			LOG(ERROR, "vfs_load for \"%s\" failed: return was %lld", m_ConfigFile[ns].c_str(), fh);
			return false;
		}
	}
	else
	{
		if (file_open(m_ConfigFile[ns].c_str(), 0, &f)!=0)
		{
			LOG(ERROR, "file_open for \"%s\" failed", m_ConfigFile[ns].c_str());
			return false;
		}
		if (file_map(&f, buffer, buflen) != 0)
		{
			LOG(ERROR, "file_map for \"%s\" failed", m_ConfigFile[ns].c_str());
			return false;
		}
	}
	
	TConfigMap newMap;
	
	char *filebuf=(char *)buffer;
	
	// Read file line by line
	char *next=filebuf-1;
	do
	{
		char *pos=next+1;
		next=strchr(pos, '\n');
		if (!next) next=filebuf+buflen;

		char *lend=next;
		if (*(lend-1) == '\r') lend--;

		// Send line to parser
		parserLine.ParseString(parser, std::string(pos, lend));
		// Get name and value from parser
		string name;
		string value;
		
		if (parserLine.GetArgCount()>=2 &&
			parserLine.GetArgString(0, name) &&
			parserLine.GetArgString(1, value))
		{
			// Add name and value to the map
			newMap[name].m_String=value;
			LOG(NORMAL, "Loaded config string \"%s\" = \"%s\"", name.c_str(), value.c_str());
		}
	}
	while (next < filebuf+buflen);
	
	m_Map[ns].swap(newMap);
	
	// Close the correct file handle
	if (m_UseVFS[ns])
	{
		vfs_close(fh);
	}
	else
	{
		file_unmap(&f);
		file_close(&f);
	}

	return true;
}

void CConfigDB::WriteFile(EConfigNamespace ns, bool useVFS, CStr path)
{
	// TODO Implement this function
}
