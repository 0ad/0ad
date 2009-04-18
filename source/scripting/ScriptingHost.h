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


#ifndef INCLUDED_SCRIPTINGHOST
#define INCLUDED_SCRIPTINGHOST

#include "ps/Errors.h"

ERROR_GROUP(Scripting);
ERROR_TYPE(Scripting, SetupFailed);

ERROR_SUBGROUP(Scripting, LoadFile);
ERROR_TYPE(Scripting_LoadFile, OpenFailed);
ERROR_TYPE(Scripting_LoadFile, EvalErrors);

ERROR_TYPE(Scripting, ConversionFailed);
ERROR_TYPE(Scripting, CallFunctionFailed);
ERROR_TYPE(Scripting, RegisterFunctionFailed);
ERROR_TYPE(Scripting, DefineConstantFailed);
ERROR_TYPE(Scripting, CreateObjectFailed);
ERROR_TYPE(Scripting, TypeDoesNotExist);

ERROR_SUBGROUP(Scripting, DefineType);
ERROR_TYPE(Scripting_DefineType, AlreadyExists);
ERROR_TYPE(Scripting_DefineType, CreationFailed);

#include "scripting/SpiderMonkey.h"

#include <string>
#include <vector>
#include <map>

#include "ps/Singleton.h"
#include "ps/CStr.h"

class IPropertyOwner
{
};

/*
class DelayedScriptExecutor
{
public:
	DelayedScriptExecutor(const std::string & functionName, float delaySeconds) 
		: m_FunctionName(functionName), m_SecondsToExecution(delaySeconds)
	{
	}

	std::string m_FunctionName;
	float		m_SecondsToExecution;
};
*/

class CustomType
{
public:
	JSObject *	m_Object;
	JSClass *	m_Class;
};

class ScriptingHost : public Singleton < ScriptingHost >
{
private:
	JSRuntime *		m_RunTime;
	JSContext *		m_Context;
	JSObject *		m_GlobalObject;

	JSErrorReport	m_ErrorReport;

	std::map < std::string, CustomType >		m_CustomObjectTypes;

#ifndef NDEBUG
	// A hook to capture script calls
	static void* jshook_script( JSContext* cx, JSStackFrame* fp, JSBool before, JSBool* ok, void* closure );
	// A hook to capture function calls
	static void* jshook_function( JSContext* cx, JSStackFrame* fp, JSBool before, JSBool* ok, void* closure );
#endif
public:

	ScriptingHost();
	~ScriptingHost();

	static void FinalShutdown();
	
	// Helpers:

	// TODO: Remove one of these
	inline JSContext *getContext() { return m_Context; }
	inline JSContext *GetContext() { return m_Context; }
	inline JSObject* GetGlobalObject() { return m_GlobalObject; }

	void RunMemScript(const char* script, size_t size, const char* filename = 0, int line = 0, JSObject* globalObject = 0);
	void RunScript(const CStr& filename, JSObject* globalObject = 0);


	jsval CallFunction(const std::string & functionName, jsval * params, int numParams);

	jsval ExecuteScript(const CStrW& script, const CStrW& calledFrom = CStrW( L"Console" ), JSObject* contextObject = NULL );

	void RegisterFunction(const std::string & functionName, JSNative function, int numArgs);

	void DefineConstant(const std::string & name, int value);
	void DefineConstant(const std::string & name, double value);

	void DefineCustomObjectType(JSClass *clasp, JSNative constructor, uintN nargs, JSPropertySpec *ps, JSFunctionSpec *fs, JSPropertySpec *static_ps, JSFunctionSpec *static_fs);

	JSObject * CreateCustomObject(const std::string & typeName);

	void  SetObjectProperty(JSObject * object, const std::string & propertyName, jsval value);
	jsval GetObjectProperty(JSObject * object, const std::string & propertyName);

	void   SetObjectProperty_Double(JSObject* object, const char* propertyName, double value);
	double GetObjectProperty_Double(JSObject* object, const char* propertyName);

	void SetGlobal(const std::string& globalName, jsval value);
	jsval GetGlobal(const std::string& globalName);

	int ValueToInt(const jsval value);
	bool ValueToBool(const jsval value);
	std::string ValueToString(const jsval value);
	CStrW ValueToUCString( const jsval value );
	utf16string ValueToUTF16( const jsval value );
    double ValueToDouble(const jsval value);

	jsval UTF16ToValue(const utf16string &str);
	jsval UCStringToValue(const CStrW& str);

	static void ErrorReporter(JSContext * context, const char * message, JSErrorReport * report);
};

#define g_ScriptingHost ScriptingHost::GetSingleton()

#endif
