
#ifndef _SCRIPTINGHOST_H_
#define _SCRIPTINGHOST_H_

#include "Errors.h"

ERROR_GROUP(Scripting);
ERROR_TYPE(Scripting, RuntimeCreationFailed);
ERROR_TYPE(Scripting, ContextCreationFailed);
ERROR_TYPE(Scripting, GlobalObjectCreationFailed);
ERROR_TYPE(Scripting, StandardClassSetupFailed);
ERROR_TYPE(Scripting, NativeFunctionSetupFailed);

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

#include <jsapi.h>

// Make JS debugging a little easier by automatically naming GC roots
#ifndef NDEBUG
// Don't simply #define NAME_ALL_GC_ROOTS, because jsapi.h is horridly broken
# define JS_AddRoot(cx, rp) JS_AddNamedRoot((cx), (rp), __FILE__ )
#endif

#include <string>
#include <vector>
#include <map>

#include "Singleton.h"
#include "CStr.h"

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

	void _CollectGarbage();

public:

	ScriptingHost();
	~ScriptingHost();
	
	// Helpers:

	JSContext* getContext();
	inline JSContext *GetContext()
	{	return getContext(); }

	void LoadScriptFromDisk(const std::string & fileName);

	jsval CallFunction(const std::string & functionName, jsval * params, int numParams);

	jsval ScriptingHost::ExecuteScript(const CStrW& script, const CStrW& calledFrom = CStrW( L"Console" ), JSObject* contextObject = NULL );

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

	jsval UCStringToValue(const utf16string &str);

	static void ErrorReporter(JSContext * context, const char * message, JSErrorReport * report);
};

#define g_ScriptingHost ScriptingHost::GetSingleton()

#endif
