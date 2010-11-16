/* Copyright (C) 2010 Wildfire Games.
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

#include "Pyrogenesis.h"
#include "Parser.h"
#include "ConfigDB.h"
#include "CLogger.h"
#include "Filesystem.h"
#include "scripting/ScriptingHost.h"

#include "scriptinterface/ScriptInterface.h"

#include <boost/algorithm/string.hpp>

typedef std::map <CStr, CConfigValueSet> TConfigMap;
TConfigMap CConfigDB::m_Map[CFG_LAST];
CStrW CConfigDB::m_ConfigFile[CFG_LAST];
bool CConfigDB::m_UseVFS[CFG_LAST];

#define GET_NS_PRIVATE(cx, obj) (EConfigNamespace)((intptr_t)JS_GetPrivate(cx, obj) >> 1)

namespace ConfigNamespace_JS
{
	JSBool GetProperty(JSContext* cx, JSObject* obj, jsid id, jsval* vp)
	{
		EConfigNamespace cfgNs = GET_NS_PRIVATE(cx, obj);
		if (cfgNs < 0 || cfgNs >= CFG_LAST)
			return JS_FALSE;

		jsval idval;
		if (!JS_IdToValue(cx, id, &idval))
			return JS_FALSE;

		std::string propName;
		if (!ScriptInterface::FromJSVal(cx, idval, propName))
			return JS_FALSE;

		CConfigValue *val = g_ConfigDB.GetValue(cfgNs, propName);
		if (val)
		{
			JSString *js_str = JS_NewStringCopyN(cx, val->m_String.c_str(), val->m_String.size());
			*vp = STRING_TO_JSVAL(js_str);
		}
		return JS_TRUE;
	}

	JSBool SetProperty(JSContext* cx, JSObject* obj, jsid id, jsval* vp)
	{
		EConfigNamespace cfgNs = GET_NS_PRIVATE(cx, obj);
		if (cfgNs < 0 || cfgNs >= CFG_LAST)
			return JS_FALSE;

		jsval idval;
		if (!JS_IdToValue(cx, id, &idval))
			return JS_FALSE;

		std::string propName;
		if (!ScriptInterface::FromJSVal(cx, idval, propName))
			return JS_FALSE;

		CConfigValue *val = g_ConfigDB.CreateValue(cfgNs, propName);

		if (!ScriptInterface::FromJSVal(cx, *vp, val->m_String))
			return JS_FALSE;

		return JS_TRUE;
	}

	JSClass Class = {
		"ConfigNamespace", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub,
		GetProperty, SetProperty,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub
	};

	JSBool Construct(JSContext* cx, uintN argc, jsval* vp)
	{
		UNUSED2(argc);

		JSObject *newObj = JS_NewObject(cx, &Class, NULL, NULL);
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(newObj));
		return JS_TRUE;
	}

	void SetNamespace(JSContext *cx, JSObject *obj, EConfigNamespace cfgNs)
	{
		JS_SetPrivate(cx, obj, (void *)((uintptr_t)cfgNs << 1)); // JS requires bottom bit = 0
	}

	JSBool WriteFile(JSContext* cx, uintN argc, jsval* vp)
	{
		EConfigNamespace cfgNs = GET_NS_PRIVATE(cx, JS_THIS_OBJECT(cx, vp));
		if (cfgNs < 0 || cfgNs >= CFG_LAST)
			return JS_FALSE;
		
		if (argc != 2)
			return JS_FALSE;

		bool useVFS;
		if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[0], useVFS))
			return JS_FALSE;

		std::wstring path;
		if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[1], path))
			return JS_FALSE;

		bool res = g_ConfigDB.WriteFile(cfgNs, useVFS, path);

		JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(res));
		return JS_TRUE;
	}

	JSBool Reload(JSContext* cx, uintN argc, jsval* vp)
	{
		if (argc != 0)
			return JS_FALSE;

		EConfigNamespace cfgNs = GET_NS_PRIVATE(cx, JS_THIS_OBJECT(cx, vp));
		if (cfgNs < 0 || cfgNs >= CFG_LAST)
			return JS_FALSE;

		JSBool ret = g_ConfigDB.Reload(cfgNs);
		JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(ret));
		return JS_TRUE;
	}

	JSBool SetFile(JSContext* cx, uintN argc, jsval* vp)
	{
		EConfigNamespace cfgNs = GET_NS_PRIVATE(cx, JS_THIS_OBJECT(cx, vp));
		if (cfgNs < 0 || cfgNs >= CFG_LAST)
			return JS_FALSE;

		if (argc != 2)
			return JS_FALSE;

		bool useVFS;
		if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[0], useVFS))
			return JS_FALSE;

		std::wstring path;
		if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[1], path))
			return JS_FALSE;

		g_ConfigDB.SetConfigFile(cfgNs, useVFS, path);

		JS_SET_RVAL(cx, vp, JSVAL_VOID);
		return JS_TRUE;
	}

	JSFunctionSpec Funcs[] = {
		{ "writeFile", WriteFile, 2, 0 },
		{ "reload", Reload, 0, 0 },
		{ "setFile", SetFile, 2, 0 },
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

	JSBool Construct(JSContext* cx, uintN argc, jsval* vp)
	{
		UNUSED2(argc);

		JSObject *newObj = JS_NewObject(cx, &Class, NULL, NULL);
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(newObj));

		int flags=JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT;
#define cfg_ns(_propname, _enum) STMT (\
	JSObject *nsobj=g_ScriptingHost.CreateCustomObject("ConfigNamespace"); \
	debug_assert(nsobj); \
	ConfigNamespace_JS::SetNamespace(cx, nsobj, _enum); \
	debug_assert(JS_DefineProperty(cx, newObj, _propname, OBJECT_TO_JSVAL(nsobj), NULL, NULL, flags)); )

		cfg_ns("default", CFG_DEFAULT);
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
	JSObject *js_ConfigDB = g_ScriptingHost.CreateCustomObject("ConfigDB");
	g_ScriptingHost.SetGlobal("g_ConfigDB", OBJECT_TO_JSVAL(js_ConfigDB));
}

CConfigValue *CConfigDB::GetValue(EConfigNamespace ns, const CStr& name)
{
	CConfigValueSet* values = GetValues(ns, name);
	if (!values)
		return (NULL);
	return &((*values)[0]);
}

CConfigValueSet *CConfigDB::GetValues(EConfigNamespace ns, const CStr& name)
{
	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return NULL;
	}

	TConfigMap::iterator it = m_Map[CFG_COMMAND].find(name);
	if (it != m_Map[CFG_COMMAND].end())
		return &(it->second);

	for (int search_ns = ns; search_ns >= 0; search_ns--)
	{
		TConfigMap::iterator it = m_Map[search_ns].find(name);
		if (it != m_Map[search_ns].end())
			return &(it->second);
	}

	return NULL;
}	

std::vector<std::pair<CStr, CConfigValueSet> > CConfigDB::GetValuesWithPrefix(EConfigNamespace ns, const CStr& prefix)
{
	std::vector<std::pair<CStr, CConfigValueSet> > ret;

	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return ret;
	}

	for (TConfigMap::iterator it = m_Map[CFG_COMMAND].begin(); it != m_Map[CFG_COMMAND].end(); ++it)
	{
		if (boost::algorithm::starts_with(it->first, prefix))
			ret.push_back(std::make_pair(it->first, it->second));
	}

	for (int search_ns = ns; search_ns >= 0; search_ns--)
	{
		for (TConfigMap::iterator it = m_Map[search_ns].begin(); it != m_Map[search_ns].end(); ++it)
		{
			if (boost::algorithm::starts_with(it->first, prefix))
				ret.push_back(std::make_pair(it->first, it->second));
		}
	}

	return ret;
}

CConfigValue *CConfigDB::CreateValue(EConfigNamespace ns, const CStr& name)
{
	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return NULL;
	}
	
	CConfigValue *ret=GetValue(ns, name);
	if (ret) return ret;
	
	TConfigMap::iterator it=m_Map[ns].insert(m_Map[ns].begin(), make_pair(name, CConfigValueSet( 1 )));
	return &(it->second[0]);
}

void CConfigDB::SetConfigFile(EConfigNamespace ns, bool useVFS, const CStrW& path)
{
	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return;
	}
	
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

	// Open file with VFS
	shared_ptr<u8> buffer; size_t buflen;
	{
		// Handle missing files quietly
		if (g_VFS->GetFileInfo(m_ConfigFile[ns], NULL) < 0)
		{
			LOGMESSAGE(L"Cannot find config file \"%ls\" - ignoring", m_ConfigFile[ns].c_str());
			return false;
		}
		else
		{
			LOGMESSAGE(L"Loading config file \"%ls\"", m_ConfigFile[ns].c_str());
			LibError ret = g_VFS->LoadFile(m_ConfigFile[ns], buffer, buflen);
			if (ret != INFO::OK)
			{
				LOGERROR(L"CConfigDB::Reload(): vfs_load for \"%ls\" failed: return was %ld", m_ConfigFile[ns].c_str(), ret);
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
		if (*(lend-1) == '\r') lend--;

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
				LOGMESSAGE(L"Loaded config string \"%hs\" = \"%hs\"", name.c_str(), value.c_str());
			}
		}
	}
	while (next < filebufend);
	
	m_Map[ns].swap(newMap);

	return true;
}

bool CConfigDB::WriteFile(EConfigNamespace ns, bool useVFS, const CStrW& path)
{
	debug_assert(useVFS);

	if (ns < 0 || ns >= CFG_LAST)
	{
		debug_warn(L"CConfigDB: Invalid ns value");
		return false;
	}

	shared_ptr<u8> buf = io_Allocate(1*MiB);
	char* pos = (char*)buf.get();
	TConfigMap &map=m_Map[ns];
	for(TConfigMap::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		pos += sprintf(pos, "%s = \"%s\"\n", it->first.c_str(), it->second[0].m_String.c_str());
	}
	const size_t len = pos - (char*)buf.get();

	LibError ret = g_VFS->CreateFile(path, buf, len);
	if(ret < 0)
	{
		LOGERROR(L"CConfigDB::WriteFile(): CreateFile \"%ls\" failed (error: %d)", path.c_str(), (int)ret);
		return false;
	}

	return true;
}
