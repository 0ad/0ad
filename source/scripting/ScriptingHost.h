
#ifndef _SCRIPTINGHOST_H_
#define _SCRIPTINGHOST_H_

#ifdef _WIN32
#define XP_WIN
#endif

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

	void SetObjectProperty(JSObject * object, const std::string & propertyName, jsval value);
	jsval GetObjectProperty( JSObject* object, const std::string& propertyName );

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
