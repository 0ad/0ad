
#ifndef _SCRIPTINGHOST_H_
#define _SCRIPTINGHOST_H_

#ifdef OS_WIN
# define XP_WIN
#endif

#ifdef OS_UNIX
# ifndef XP_UNIX
#  define XP_UNIX
# endif
#endif

ERROR_GROUP(PSERROR, Scripting);
ERROR_TYPE(PSERROR_Scripting, RuntimeCreationFailed);
ERROR_TYPE(PSERROR_Scripting, ContextCreationFailed);
ERROR_TYPE(PSERROR_Scripting, GlobalObjectCreationFailed);
ERROR_TYPE(PSERROR_Scripting, StandardClassSetupFailed);
ERROR_TYPE(PSERROR_Scripting, NativeFunctionSetupFailed);

ERROR_GROUP(PSERROR_Scripting, LoadFile);
ERROR_TYPE(PSERROR_Scripting_LoadFile, OpenFailed);
ERROR_TYPE(PSERROR_Scripting_LoadFile, EvalErrors);

ERROR_TYPE(PSERROR_Scripting, ConversionFailed);
ERROR_TYPE(PSERROR_Scripting, CallFunctionFailed);
ERROR_TYPE(PSERROR_Scripting, RegisterFunctionFailed);
ERROR_TYPE(PSERROR_Scripting, DefineConstantFailed);
ERROR_TYPE(PSERROR_Scripting, CreateObjectFailed);
ERROR_TYPE(PSERROR_Scripting, TypeDoesNotExist);

ERROR_GROUP(PSERROR_Scripting, DefineType);
ERROR_TYPE(PSERROR_Scripting_DefineType, AlreadyExists);
ERROR_TYPE(PSERROR_Scripting_DefineType, CreationFailed);

#include <jsapi.h>

#include <string>
#include <vector>
#include <map>

#include "Singleton.h"

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

	std::vector < DelayedScriptExecutor >		m_DelayedScripts;
	std::map < std::string, CustomType >		m_CustomObjectTypes;

	void _CollectGarbage();

public:

	ScriptingHost();
	~ScriptingHost();
	
	JSContext* getContext();

	void LoadScriptFromDisk(const std::string & fileName);

	jsval CallFunction(const std::string & functionName, jsval * params, int numParams);

	jsval ExecuteScript(const std::string & script);

	void RegisterFunction(const std::string & functionName, JSNative function, int numArgs);

	void DefineConstant(const std::string & name, int value);
	void DefineConstant(const std::string & name, double value);

	void DefineCustomObjectType(JSClass *clasp, JSNative constructor, uintN nargs, JSPropertySpec *ps, JSFunctionSpec *fs, JSPropertySpec *static_ps, JSFunctionSpec *static_fs);

	JSObject * CreateCustomObject(const std::string & typeName);

	void  SetObjectProperty(JSObject * object, const std::string & propertyName, jsval value);
	jsval GetObjectProperty(JSObject * object, const std::string & propertyName);

	void SetGlobal(const std::string& globalName, jsval value);
	jsval GetGlobal(const std::string& globalName);

	int ValueToInt(const jsval value);
	bool ValueToBool(const jsval value);
	std::string ValueToString(const jsval value);
    double ValueToDouble(const jsval value);

	void Tick(float timeElapsed);

	void AddDelayedScript(const std::string & functionName, float delaySeconds);

	static void ErrorReporter(JSContext * context, const char * message, JSErrorReport * report);
};

#define g_ScriptingHost ScriptingHost::GetSingleton()

#endif
