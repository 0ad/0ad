#include "precompiled.h"

#include "Prometheus.h"
#include "Parser.h"
#include "ConfigDB.h"
#include "CLogger.h"
#include "res/vfs.h"
#include "res/file.h"
#include "scripting/ScriptingHost.h"
#include "types.h"

using namespace std;

typedef map <CStr, CConfigValue> TConfigMap;
TConfigMap CConfigDB::m_Map[CFG_LAST];
CStr CConfigDB::m_ConfigFile[CFG_LAST];
bool CConfigDB::m_UseVFS[CFG_LAST];

namespace ConfigNamespace_JS
{
	JSBool GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		EConfigNamespace cfgNs=(EConfigNamespace)(int)JS_GetPrivate(cx, obj);
		if (cfgNs < 0 || cfgNs >= CFG_LAST)
			return JS_FALSE;

		CStr propName = g_ScriptingHost.ValueToString(id);
		CConfigValue *val=g_ConfigDB.GetValue(cfgNs, propName);
		if (val)
		{
			JSString *js_str=JS_NewStringCopyN(cx, val->m_String.c_str(), val->m_String.size());
			*vp = STRING_TO_JSVAL(js_str);
		}
		else
			*vp = JSVAL_NULL;
		return JS_TRUE;
	}

	JSBool SetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		EConfigNamespace cfgNs=(EConfigNamespace)(int)JS_GetPrivate(cx, obj);
		if (cfgNs < 0 || cfgNs >= CFG_LAST)
			return JS_FALSE;

		CStr propName = g_ScriptingHost.ValueToString(id);
		CConfigValue *val=g_ConfigDB.CreateValue(cfgNs, propName);
		char *str;
		if (JS_ConvertArguments(cx, 1, vp, "s", &str))
		{
			val->m_String=str;
			return JS_TRUE;
		}
		else
			return JS_FALSE;
	}

	JSClass Class = {
		"ConfigNamespace", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub,
		GetProperty, SetProperty,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub
	};

	JSBool Construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
	{
		if (argc != 0)
			return JS_FALSE;

		JSObject *newObj=JS_NewObject(cx, &Class, NULL, NULL);
		*rval=OBJECT_TO_JSVAL(newObj);
		return JS_TRUE;
	}

	void SetNamespace(JSContext *cx, JSObject *obj, EConfigNamespace cfgNs)
	{
		JS_SetPrivate(cx, obj, (void *)cfgNs);
	}
};

namespace ConfigDB_JS
{
	JSClass Class = {
		"ConfigDB", 0,
		JS_PropertyStub, JS_PropertyStub,
		JS_PropertyStub, JS_PropertyStub,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub
	};

	JSPropertySpec Props[] = {
		{0}
	};

	JSFunctionSpec Funcs[] = {
		{0}
	};

	JSBool Construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
	{
		if (argc != 0)
			return JS_FALSE;

		JSObject *newObj=JS_NewObject(cx, &Class, NULL, NULL);
		*rval=OBJECT_TO_JSVAL(newObj);

		int flags=JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT;
#define cfg_ns(_propname, _enum) STMT (\
	JSObject *nsobj=g_ScriptingHost.CreateCustomObject("ConfigNamespace"); \
	assert(nsobj); \
	ConfigNamespace_JS::SetNamespace(cx, nsobj, _enum); \
	assert(JS_DefineProperty(cx, newObj, _propname, OBJECT_TO_JSVAL(nsobj), NULL, NULL, flags)); )

		cfg_ns("system", CFG_SYSTEM);
		cfg_ns("user", CFG_USER);
		cfg_ns("mod", CFG_MOD);

#undef cfg_ns

		return JS_TRUE;
	}

};

CConfigDB::CConfigDB()
{
	g_ScriptingHost.DefineCustomObjectType(&ConfigDB_JS::Class, ConfigDB_JS::Construct, 0, ConfigDB_JS::Props, ConfigDB_JS::Funcs, NULL, NULL);
	g_ScriptingHost.DefineCustomObjectType(&ConfigNamespace_JS::Class, ConfigNamespace_JS::Construct, 0, NULL, NULL, NULL, NULL);
	JSObject *js_ConfigDB=g_ScriptingHost.CreateCustomObject("ConfigDB");
	g_ScriptingHost.SetGlobal("g_ConfigDB", OBJECT_TO_JSVAL(js_ConfigDB));
}

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
	parser.InputTaskType("Assignment", "_$ident_=_[-$arg(_minus)]_$value_[[;]$rest]");
	parser.InputTaskType("CommentOrBlank", "_[;[$rest]]");

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
		if (file_open(m_ConfigFile[ns], 0, &f)!=0)
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
		bool parseOk=parserLine.ParseString(parser, std::string(pos, lend));
		// Get name and value from parser
		string name;
		string value;
		
		if (parseOk &&
			parserLine.GetArgCount()>=2 &&
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
