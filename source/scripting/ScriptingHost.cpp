#include "precompiled.h"

#include "ScriptingHost.h"
#include "ScriptGlue.h"
#include "CConsole.h"
#include <sstream>
#include <fstream>
#include <iostream>

#ifdef _WIN32
# include "float.h" // <- MT: Just for _finite(), converting certain strings was causing wierd bugs.
# define finite _finite
#else
# define finite __finite // PT: Need to use _finite in MSVC, __finite in gcc
#endif

#ifdef NDEBUG
# pragma comment (lib, "js32.lib")
#else
# pragma comment (lib, "js32d.lib")
#endif

extern CConsole* g_Console;

namespace
{
	const int RUNTIME_MEMORY_ALLOWANCE = 16 * 1024 * 1024;
	const int STACK_CHUNK_SIZE = 16 * 1024;

	JSClass GlobalClass = 
	{
		"global", 0,
		JS_PropertyStub, JS_PropertyStub,
		JS_PropertyStub, JS_PropertyStub,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub
	};
}

ScriptingHost::ScriptingHost() : m_RunTime(NULL), m_Context(NULL), m_GlobalObject(NULL)
{
    m_RunTime = JS_NewRuntime(RUNTIME_MEMORY_ALLOWANCE);

    if (m_RunTime == NULL)
	{
		throw (std::string("ScriptingHost: Failed to create JavaScript runtime"));
	}

    m_Context = JS_NewContext(m_RunTime, STACK_CHUNK_SIZE);

    if (m_Context == NULL)
	{
		throw (std::string("ScriptingHost: Failed to create JavaScript context"));
	}

	JS_SetErrorReporter(m_Context, ScriptingHost::ErrorReporter);

	m_GlobalObject = JS_NewObject(m_Context, &GlobalClass, NULL, NULL);

	if (m_GlobalObject == NULL)
	{
		throw (std::string("ScriptingHost: Failed to create global object"));
	}

	if (JS_InitStandardClasses(m_Context, m_GlobalObject) == JSVAL_FALSE)
	{
		throw (std::string("ScriptingHost: Failed to init standard classes"));
	}

	if (JS_DefineFunctions(m_Context, m_GlobalObject, ScriptFunctionTable) == JS_FALSE)
	{
		throw (std::string("ScriptingHost: Failed to setup native functions"));
	}

	std::cout << "Scripting environment initialized" << std::endl;
}

ScriptingHost::~ScriptingHost()
{
	if (m_Context != NULL)
	{
		JS_DestroyContext(m_Context);
		m_Context = NULL;
	}

	if (m_RunTime != NULL)
	{
		JS_DestroyRuntime(m_RunTime);
		m_RunTime = NULL;
	}
}

JSContext* ScriptingHost::getContext()
{
	return( m_Context );
}

void ScriptingHost::LoadScriptFromDisk(const std::string & fileName)
{
	std::string script;
	std::string line;

	std::ifstream scriptFile(fileName.c_str());

	if (scriptFile.is_open() == false)
	{
		throw (std::string("Could not open file: ") + fileName);
	}

	while (scriptFile.eof() == false)
	{
		std::getline(scriptFile, line);
		script += line;
		script += '\n';
	}

	jsval rval; 
	JSBool ok = JS_EvaluateScript(m_Context, m_GlobalObject, script.c_str(), (unsigned int)script.length(), fileName.c_str(), 0, &rval); 

    if (ok == JS_FALSE)
	{
		throw (std::string("Error loading script from disk: ") + fileName);
	}
}

jsval ScriptingHost::CallFunction(const std::string & functionName, jsval * params, int numParams)
{
	jsval result;

	JSBool ok = JS_CallFunctionName(m_Context, m_GlobalObject, functionName.c_str(), numParams, params, &result);

	if (ok == JS_FALSE)
	{
		throw (std::string("Failure whilst calling script funtion: ") + functionName);
	}

	return result;
}

jsval ScriptingHost::ExecuteScript(const std::string & script)
{
	jsval rval; 

	JSBool ok = JS_EvaluateScript(m_Context, m_GlobalObject, script.c_str(), (int)script.length(), "Console", 0, &rval); 

	if (!ok) return JSVAL_NULL;

	return rval;
}

void ScriptingHost::RegisterFunction(const std::string & functionName, JSNative function, int numArgs)
{
	JSFunction * func = JS_DefineFunction(m_Context, m_GlobalObject, functionName.c_str(), function, numArgs, 0);

	if (func == NULL)
	{
		throw (std::string("Could not register function ") + functionName);
	}
}

void ScriptingHost::DefineConstant(const std::string & name, int value)
{
	// First remove this constant if it already exists
	JS_DeleteProperty(m_Context, m_GlobalObject, name.c_str());

	JSBool ok = JS_DefineProperty(	m_Context, m_GlobalObject, name.c_str(), INT_TO_JSVAL(value), 
									NULL, NULL, JSPROP_READONLY);

	if (ok == JS_FALSE)
	{
		throw (std::string("Could not create constant"));
	}
}

void ScriptingHost::DefineConstant(const std::string & name, double value)
{
	// First remove this constant if it already exists
	JS_DeleteProperty(m_Context, m_GlobalObject, name.c_str());

	struct JSConstDoubleSpec spec[2];

	spec[0].name = name.c_str();
	spec[0].dval = value;
	spec[0].flags = JSPROP_READONLY;

	spec[1].name = 0;
	spec[1].dval = 0.0;
	spec[1].flags = 0;

	JSBool ok = JS_DefineConstDoubles(m_Context, m_GlobalObject, spec);

	if (ok == JS_FALSE)
	{
		throw (std::string("Could not create constant"));
	}
}

void ScriptingHost::DefineCustomObjectType(JSClass *clasp, JSNative constructor, uintN minArgs, JSPropertySpec *ps, JSFunctionSpec *fs, JSPropertySpec *static_ps, JSFunctionSpec *static_fs)
{
	std::string typeName = clasp->name;

	if (m_CustomObjectTypes.find(typeName) != m_CustomObjectTypes.end())
	{
		// This type already exists
		throw std::string("Type already exists");
	}

	JSObject * obj = JS_InitClass(	m_Context, m_GlobalObject, 0, 
									clasp,
									constructor, minArgs,				// Constructor, min args
									ps, fs,								// Properties, methods
									static_ps, static_fs);				// Constructor properties, methods

	if (obj != NULL)
	{
		CustomType type;
		
		type.m_Object = obj;
		type.m_Class = clasp;

		m_CustomObjectTypes[typeName] = type;
	}
	else
	{
		throw std::string("Type creation failed");
	}
}

JSObject * ScriptingHost::CreateCustomObject(const std::string & typeName)
{
	std::map < std::string, CustomType > ::iterator it = m_CustomObjectTypes.find(typeName);

	if (it == m_CustomObjectTypes.end())
	{
		throw std::string("Tried to create a type that doesn't exist");
	}

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

void ScriptingHost::SetGlobal(const std::string &globalName, jsval value)
{
	JS_SetProperty(m_Context, m_GlobalObject, globalName.c_str(), &value);
}

jsval ScriptingHost::GetGlobal(const std::string &globalName)
{
	jsval vp;
	JS_GetProperty(m_Context, m_GlobalObject, globalName.c_str(), &vp);
	return vp;
}

int ScriptingHost::ValueToInt(const jsval value)
{
	int32 i = 0;

	JSBool ok = JS_ValueToInt32(m_Context, value, &i);

	if (ok == JS_FALSE)
	{
		throw (std::string("Convert to int failed"));
	}

	return i;
}

bool ScriptingHost::ValueToBool(const jsval value)
{
	JSBool b;

	JSBool ok = JS_ValueToBoolean(m_Context, value, &b);

	if (ok == JS_FALSE)
	{
		throw (std::string("Convert to bool failed"));
	}

	return b == JS_TRUE;
}

std::string ScriptingHost::ValueToString(const jsval value)
{
	JSString * string = JS_ValueToString(m_Context, value);

	return std::string(JS_GetStringBytes(string));
}

double ScriptingHost::ValueToDouble(const jsval value)
{
	jsdouble d;

	JSBool ok = JS_ValueToNumber(m_Context, value, &d);

	if (ok == JS_FALSE || !finite( d ) )
	{
		throw (std::string("Convert to double failed"));
	}

	return d;
}

void ScriptingHost::ErrorReporter(JSContext * context, const char * message, JSErrorReport * report)
{
	debug_out("%s(%d) : %s\n", report->filename, report->lineno, message);

	if (g_Console)
	{
		g_Console->InsertMessage( L"%hs (%d)", report->filename, report->lineno );
		if (message)
		{
			g_Console->InsertMessage( L"%hs", message );
		}
		else
			g_Console->InsertMessage( L"No error message available" );
	}

	if (report->filename != NULL)
	{
		std::cout << report->filename << " (" << report->lineno << ") ";
	}

	if (message != NULL)
	{
		std::cout << message << std::endl;
	}
	else
	{
		std::cout << "No error message available" << std::endl;
	}
}

void ScriptingHost::Tick(float timeElapsed)
{
	if (timeElapsed > 0.2f) timeElapsed = 0.2f;

	for (int i = 0; i < (int)m_DelayedScripts.size(); )
	{
		m_DelayedScripts[i].m_SecondsToExecution -= timeElapsed;

		if (m_DelayedScripts[i].m_SecondsToExecution <= 0.0f)
		{
			this->ExecuteScript(m_DelayedScripts[i].m_FunctionName);
			m_DelayedScripts.erase(m_DelayedScripts.begin() + i);
		}
		else
		{
			i++;
		}
	}
}

void ScriptingHost::AddDelayedScript(const std::string & functionName, float delaySeconds)
{
	m_DelayedScripts.push_back(DelayedScriptExecutor(functionName, delaySeconds));
}

void ScriptingHost::_CollectGarbage()
{
	JS_GC(m_Context);
}
