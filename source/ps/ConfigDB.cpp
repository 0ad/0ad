#include "precompiled.h"

#include "Prometheus.h"
#include "Parser.h"
#include "ConfigDB.h"
#include "CLogger.h"
#include "res/vfs.h"
#include "res/file.h"
#include "res/mem.h"
#include "scripting/ScriptingHost.h"
#include "types.h"

using namespace std;

typedef map <CStr, CConfigValueSet> TConfigMap;
TConfigMap CConfigDB::m_Map[CFG_LAST];
CStr CConfigDB::m_ConfigFile[CFG_LAST];
bool CConfigDB::m_UseVFS[CFG_LAST];

namespace ConfigNamespace_JS
{
	JSBool GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		EConfigNamespace cfgNs=(EConfigNamespace)(intptr_t)JS_GetPrivate(cx, obj);
		if (cfgNs < 0 || cfgNs >= CFG_LAST)
			return JS_FALSE;

		CStr propName = g_ScriptingHost.ValueToString(id);
		CConfigValue *val=g_ConfigDB.GetValue(cfgNs, propName);
		if (val)
		{
			JSString *js_str=JS_NewStringCopyN(cx, val->m_String.c_str(), val->m_String.size());
			*vp = STRING_TO_JSVAL(js_str);
		}
		return JS_TRUE;
	}

	JSBool SetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		EConfigNamespace cfgNs=(EConfigNamespace)(intptr_t)JS_GetPrivate(cx, obj);
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

		JSObject *newObj=JS_NewObject(cx, &Class, NULL, obj);
		*rval=OBJECT_TO_JSVAL(newObj);
		return JS_TRUE;
	}

	void SetNamespace(JSContext *cx, JSObject *obj, EConfigNamespace cfgNs)
	{
		JS_SetPrivate(cx, obj, (void *)cfgNs);
	}

	JSBool WriteFile( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
	{
		EConfigNamespace cfgNs=(EConfigNamespace)(intptr_t)JS_GetPrivate(cx, obj);
		if (cfgNs < 0 || cfgNs >= CFG_LAST)
			return JS_FALSE;
		
		if (argc != 2)
			return JS_FALSE;

		JSBool useVFS;
		char *path;
		if (JS_ConvertArguments(cx, 2, argv, "bs", &useVFS, &path))
		{
			JSBool res=g_ConfigDB.WriteFile(cfgNs, useVFS?true:false, path);
			*rval = BOOLEAN_TO_JSVAL(res);
			return JS_TRUE;
		}
		else
			return JS_FALSE;
	}

	JSBool Reload( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
	{
		if (argc != 0)
			return JS_FALSE;

		EConfigNamespace cfgNs=(EConfigNamespace)(intptr_t)JS_GetPrivate(cx, obj);
		if (cfgNs < 0 || cfgNs >= CFG_LAST)
			return JS_FALSE;

		JSBool ret=g_ConfigDB.Reload(cfgNs);
		*rval = BOOLEAN_TO_JSVAL(ret);
		return JS_TRUE;
	}

	JSBool SetFile( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
	{
		if (argc != 0)
			return JS_FALSE;

		EConfigNamespace cfgNs=(EConfigNamespace)(intptr_t)JS_GetPrivate(cx, obj);
		if (cfgNs < 0 || cfgNs >= CFG_LAST)
			return JS_FALSE;

		JSBool useVFS;
		char *path;
		if (JS_ConvertArguments(cx, 2, argv, "bs", &useVFS, &path))
		{
			g_ConfigDB.SetConfigFile(cfgNs, useVFS?true:false, path);
			return JS_TRUE;
		}
		else
			return JS_FALSE;
	}

	JSFunctionSpec Funcs[] = {
		{ "WriteFile", WriteFile, 2, 0, 0},
		{ "Reload", Reload, 0, 0, 0},
		{ "SetFile", SetFile, 1, 0, 0},
		{0}
	};
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

		JSObject *newObj=JS_NewObject(cx, &Class, NULL, obj);
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
	g_ScriptingHost.DefineCustomObjectType(&ConfigNamespace_JS::Class, ConfigNamespace_JS::Construct, 0, NULL, ConfigNamespace_JS::Funcs, NULL, NULL);
	JSObject *js_ConfigDB=g_ScriptingHost.CreateCustomObject("ConfigDB");
	g_ScriptingHost.SetGlobal("g_ConfigDB", OBJECT_TO_JSVAL(js_ConfigDB));
}

CConfigValue *CConfigDB::GetValue(EConfigNamespace ns, CStr name)
{
	/*
	assert(ns < CFG_LAST && ns >= 0);
	
	TConfigMap::iterator it=m_Map[ns].find(name);
	if (it == m_Map[ns].end())
		return NULL;
	else
		return &(it->second[0] );
	*/
	CConfigValueSet* values = GetValues( ns, name );
	if( !values ) return( NULL );
	return &( (*values)[0] );
}

CConfigValueSet *CConfigDB::GetValues(EConfigNamespace ns, CStr name )
{
	assert(ns < CFG_LAST && ns >= 0);
	
	TConfigMap::iterator it=m_Map[ns].find(name);
	if (it == m_Map[ns].end())
		return NULL;
	else
		return &(it->second );
}

CConfigValue *CConfigDB::CreateValue(EConfigNamespace ns, CStr name)
{
	assert(ns < CFG_LAST && ns >= 0);
	
	CConfigValue *ret=GetValue(ns, name);
	if (ret) return ret;
	
	TConfigMap::iterator it=m_Map[ns].insert(m_Map[ns].begin(), make_pair(name, CConfigValueSet( 1 )));
	return &(it->second[0]);
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
	parser.InputTaskType("Assignment", "_$ident_=<_[-$arg(_minus)]_$value_,>_[-$arg(_minus)]_$value[[;]$rest]");
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
			size_t argCount = parserLine.GetArgCount();

			newMap[name].clear();

			for( size_t t = 0; t < argCount; t++ )
			{
				if( !parserLine.GetArgString( (int)t + 1, value ) )
					continue;
				CConfigValue argument;
				argument.m_String = value;
				newMap[name].push_back( argument );
				LOG(NORMAL, "Loaded config string \"%s\" = \"%s\"", name.c_str(), value.c_str());
			}
		}
	}
	while (next < filebuf+buflen);
	
	m_Map[ns].swap(newMap);
	
	// Close the correct file handle
	if (m_UseVFS[ns])
	{
		mem_free_h(fh);
	}
	else
	{
		file_unmap(&f);
		file_close(&f);
	}

	return true;
}

bool CConfigDB::WriteFile(EConfigNamespace ns, bool useVFS, CStr path)
{
	assert(ns >= 0 && ns < CFG_LAST);

	char realpath[VFS_MAX_PATH];
	char nativepath[VFS_MAX_PATH];
	const char *filepath=path.c_str();
	int err;
	FILE *fp;

	if (useVFS)
	{
		if ((err=vfs_realpath(filepath, realpath))!=0)
		{
			LOG(ERROR, "CConfigDB::WriteFile(): vfs_realpath for VFS path \"%s\" failed (error: %d)", filepath, err);
			return false;
		}
		filepath=realpath;
	}
	file_make_native_path(filepath, nativepath);
	if ((fp = fopen(nativepath, "w")) == NULL)
	{
		LOG(ERROR, "CConfigDB::WriteFile(): fopen for path \"%s\" failed (error: %d)", filepath, errno);
		return false;
	}
	
	uint offset=0;

	TConfigMap &map=m_Map[ns];
	TConfigMap::const_iterator it=map.begin();
	while (it != map.end())
	{
		fprintf(fp, "%s = \"%s\"\n", it->first.c_str(), it->second[0].m_String.c_str());
		++it;
	}

	fclose(fp);
	return true;
}
