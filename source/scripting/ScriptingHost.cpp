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

#include <sstream>

#include "ScriptingHost.h"
#include "ScriptGlue.h"
#include "lib/utf8.h"
#include "ps/Profile.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "scriptinterface/ScriptInterface.h"

#define LOG_CATEGORY L"scriptinghost"

ScriptingHost::ScriptingHost()
{
	m_ScriptInterface = new ScriptInterface("Engine", "GUI");

    m_Context = m_ScriptInterface->GetContext();

	m_GlobalObject = JS_GetGlobalObject(m_Context);

	if (!JS_DefineFunctions(m_Context, m_GlobalObject, ScriptFunctionTable))
		throw PSERROR_Scripting_SetupFailed();

	if (!JS_DefineProperties(m_Context, m_GlobalObject, ScriptGlobalTable))
		throw PSERROR_Scripting_SetupFailed();
}

ScriptingHost::~ScriptingHost()
{
	delete m_ScriptInterface;
}

ScriptInterface& ScriptingHost::GetScriptInterface()
{
	return *m_ScriptInterface;
}

void ScriptingHost::FinalShutdown()
{
	// This should only be called once per process, just to clean up before
	// we report memory leaks. (Otherwise, if it's called while there are
	// other contexts active in other threads, things will break.)
	JS_ShutDown();
}

// filename, line and globalObject default to 0 (in which case we execute
// the whole script / use our m_GlobalObject)
void ScriptingHost::RunMemScript(const char* script, size_t size, const char* filename, int line, JSObject* globalObject)
{
	if(!filename)
		filename = "unspecified file";
	if(!globalObject)
		globalObject = m_GlobalObject;

	// Maybe TODO: support Unicode input formats?

	jsval rval;
	JSBool ok = JS_EvaluateScript(m_Context, globalObject, script, (uintN)size, filename, line, &rval); 

	if (ok == JS_FALSE)
		throw PSERROR_Scripting_LoadFile_EvalErrors();
}

// globalObject defaults to 0 (in which case we use our m_GlobalObject).
void ScriptingHost::RunScript(const VfsPath& pathname, JSObject* globalObject)
{
	if(!globalObject)
		globalObject = m_GlobalObject;

	shared_ptr<u8> buf; size_t size;
	if(g_VFS->LoadFile(pathname, buf, size) != INFO::OK)	// ERRTODO: translate/pass it on
		throw PSERROR_Scripting_LoadFile_OpenFailed();

	std::wstring scriptw = wstring_from_utf8(std::string(buf.get(), buf.get() + size));
	utf16string script(scriptw.begin(), scriptw.end());

	jsval rval;
	JSBool ok = JS_EvaluateUCScript(m_Context, globalObject, script.c_str(), (uintN)script.size(), CStr(pathname.string()).c_str(), 1, &rval);

	if (ok == JS_FALSE)
		throw PSERROR_Scripting_LoadFile_EvalErrors();
}

jsval ScriptingHost::CallFunction(const std::string & functionName, jsval * params, int numParams)
{
	jsval result;

	JSBool ok = JS_CallFunctionName(m_Context, m_GlobalObject, functionName.c_str(), numParams, params, &result);

	if (ok == JS_FALSE)
		throw PSERROR_Scripting_CallFunctionFailed();

	return result;
}

jsval ScriptingHost::ExecuteScript(const CStrW& script, const CStrW& calledFrom, JSObject* contextObject )
{
	jsval rval; 

	JSBool ok = JS_EvaluateUCScript(m_Context, contextObject ? contextObject : m_GlobalObject,
		script.utf16().c_str(), (int)script.length(), CStr(calledFrom), 1, &rval); 

	if (!ok) return JSVAL_NULL;

	return rval;
}

void ScriptingHost::DefineConstant(const std::string & name, int value)
{
	// First remove this constant if it already exists
	JS_DeleteProperty(m_Context, m_GlobalObject, name.c_str());

	JSBool ok = JS_DefineProperty(	m_Context, m_GlobalObject, name.c_str(), INT_TO_JSVAL(value), 
									NULL, NULL, JSPROP_READONLY);

	if (ok == JS_FALSE)
		throw PSERROR_Scripting_DefineConstantFailed();
}

void ScriptingHost::DefineCustomObjectType(JSClass *clasp, JSNative constructor, uintN minArgs, JSPropertySpec *ps, JSFunctionSpec *fs, JSPropertySpec *static_ps, JSFunctionSpec *static_fs)
{
	std::string typeName = clasp->name;

	if (m_CustomObjectTypes.find(typeName) != m_CustomObjectTypes.end())
	{
		// This type already exists
		throw PSERROR_Scripting_DefineType_AlreadyExists();
	}

	JSObject * obj = JS_InitClass(	m_Context, m_GlobalObject, 0, 
									clasp,
									constructor, minArgs,				// Constructor, min args
									ps, fs,								// Properties, methods
									static_ps, static_fs);				// Constructor properties, methods

	if (obj == NULL)
		throw PSERROR_Scripting_DefineType_CreationFailed();

	CustomType type;
	
	type.m_Object = obj;
	type.m_Class = clasp;

	m_CustomObjectTypes[typeName] = type;
}

JSObject * ScriptingHost::CreateCustomObject(const std::string & typeName)
{
	std::map < std::string, CustomType > ::iterator it = m_CustomObjectTypes.find(typeName);

	if (it == m_CustomObjectTypes.end())
		throw PSERROR_Scripting_TypeDoesNotExist();

	return JS_ConstructObject(m_Context, (*it).second.m_Class, (*it).second.m_Object, NULL);

}



void ScriptingHost::SetObjectProperty(JSObject * object, const std::string & propertyName, jsval value)
{
	JS_SetProperty(m_Context, object, propertyName.c_str(), &value);
}

jsval ScriptingHost::GetObjectProperty( JSObject* object, const std::string& propertyName )
{
	jsval vp;
	JS_GetProperty( m_Context, object, propertyName.c_str(), &vp );
	return( vp );
}



void ScriptingHost::SetObjectProperty_Double(JSObject* object, const char* propertyName, double value)
{
	jsdouble* d = JS_NewDouble(m_Context, value);
	if (! d)
		throw PSERROR_Scripting_ConversionFailed();

	jsval v = DOUBLE_TO_JSVAL(d);

	if (! JS_SetProperty(m_Context, object, propertyName, &v))
		throw PSERROR_Scripting_ConversionFailed();
}

double ScriptingHost::GetObjectProperty_Double(JSObject* object, const char* propertyName)
{
	jsval v;
	double d;

	if (! JS_GetProperty(m_Context, object, propertyName, &v))
		throw PSERROR_Scripting_ConversionFailed();
	if (! JS_ValueToNumber(m_Context, v, &d))
		throw PSERROR_Scripting_ConversionFailed();
	return d;
}



void ScriptingHost::SetGlobal(const std::string &globalName, jsval value)
{
	JS_SetProperty(m_Context, m_GlobalObject, globalName.c_str(), &value);
}



//----------------------------------------------------------------------------
// conversions
//----------------------------------------------------------------------------

std::string ScriptingHost::ValueToString(const jsval value)
{
	JSString* string = JS_ValueToString(m_Context, value);
	if (string == NULL)
		throw PSERROR_Scripting_ConversionFailed();

	return std::string(JS_GetStringBytes(string), JS_GetStringLength(string));
}

CStrW ScriptingHost::ValueToUCString( const jsval value )
{
	JSString* string = JS_ValueToString(m_Context, value);
	if (string == NULL)
		throw PSERROR_Scripting_ConversionFailed();

	jschar *strptr=JS_GetStringChars(string);
	size_t length=JS_GetStringLength(string);
	return std::wstring(strptr, strptr+length);
}

jsval ScriptingHost::UCStringToValue( const CStrW& str )
{
	return STRING_TO_JSVAL(JS_NewUCStringCopyZ(m_Context, str.utf16().c_str()));
}
