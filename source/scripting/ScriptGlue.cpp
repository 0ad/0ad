
#include "ScriptGlue.h"

// Parameters for the table are:

// 1: The name the function will be called as from script
// 2: The number of aguments this function expects
// 3: Depreciated, always zero
// 4: Reserved for future use, always zero

JSFunctionSpec ScriptFunctionTable[] = 
{
	{"WriteLog", WriteLog, 1, 0, 0},

	{0, 0, 0, 0, 0}, 
};

// Allow scripts to output to the global log file
JSBool WriteLog(JSContext * context, JSObject * globalObject, unsigned int argc, jsval * argv, jsval * rval)
{
	if (argc < 1)
		return JS_FALSE;

	for (int i = 0; i < (int)argc; i++)
	{
		if (JSVAL_IS_INT(argv[i]))
		{
			printf("%d", JSVAL_TO_INT(argv[i]));
		}

		if (JSVAL_IS_DOUBLE(argv[i]))
		{
			double d = g_ScriptingHost.ValueToDouble(argv[i]);
			printf("%e", d);
		}

		if (JSVAL_IS_STRING(argv[i]))
		{
			JSString * str = JS_ValueToString(context, argv[i]);
			char * chars = JS_GetStringBytes(str);
			printf(chars);
		}
	}

	printf("\n");

	return JS_TRUE;
}
