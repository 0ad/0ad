#include "precompiled.h"

#include "ScriptInterface.h"
#include "CLocale.h"
#include "StringConvert.h"

#include "jsapi.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "i18n"

using namespace I18n;

namespace LookedupWord {
	
	JSBool GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
	{
		CLocale::LookupType* lookedup = (CLocale::LookupType*)JS_GetPrivate(cx, obj);
		assert(lookedup);

		JSObject* parent = JS_GetParent(cx, obj);
		assert(parent);
		CLocale* locale = (CLocale*)JS_GetPrivate(cx, parent);
		assert(locale);

		JSString* prop = JS_ValueToString(cx, id);
		if (!prop)
		{
			JS_ReportError(cx, "lookup() property failed to convert to string");
			return JS_FALSE;
		}

		jschar* prop_chars = JS_GetStringChars(prop);
		if (!prop_chars)
		{
			JS_ReportError(cx, "lookup() property failed to convert string to chars");
			return JS_FALSE;
		}

		Str prop_str;
		StringConvert::jschars_to_wstring(prop_chars, JS_GetStringLength(prop), prop_str);

		Str result;
		if (! locale->LookupProperty(lookedup, prop_str, result))
			result = L"(unrecognised property)";

		JSString* result_str = StringConvert::wstring_to_jsstring(cx, result);
		if (!result_str)
		{
			JS_ReportError(cx, "lookup() property failed to create string");
			return JS_FALSE;
		}

		*vp = STRING_TO_JSVAL(result_str);
		
		return JS_TRUE;
	}

	void Finalize(JSContext *cx, JSObject *obj)
	{
		// Free the LookupType that was allocated when building this object

		CLocale::LookupType* lookedup = (CLocale::LookupType*)JS_GetPrivate(cx, obj);
		assert(lookedup);
		delete lookedup;
	}

	static JSClass JSI_class = {
		"LookedupWord", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub,
		GetProperty, JS_PropertyStub,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, Finalize,
		NULL, NULL, NULL, NULL
	};
}


JSBool JSFunc_LookupWord(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if (argc != 2)
	{
		JS_ReportError(cx, "Incorrect number of parameters to lookup(dictionary, word) function");
		return JS_FALSE;
	}

	// Get the strings
	JSString* dictname = JS_ValueToString(cx, argv[0]);
	JSString* word = JS_ValueToString(cx, argv[1]);
	if (!dictname || !word)
	{
		JS_ReportError(cx, "lookup() failed to convert parameters to strings");
		return JS_FALSE;
	}
	// and the characters from the strings
	jschar* dictname_chars = JS_GetStringChars(dictname);
	jschar* word_chars = JS_GetStringChars(word);
	if (!dictname_chars || !word_chars)
	{
		JS_ReportError(cx, "lookup() failed to get parameter string data");
		return JS_FALSE;
	}
	// and convert those characters into to wstrings
	Str dictname_str, word_str;
	StringConvert::jschars_to_wstring(dictname_chars, JS_GetStringLength(dictname), dictname_str);
	StringConvert::jschars_to_wstring(word_chars, JS_GetStringLength(word), word_str);

	// Extract the CLocale* from the 'global' object
	CLocale* locale = (CLocale*)JS_GetPrivate(cx, obj);

	const CLocale::LookupType* lookedup = locale->LookupWord(dictname_str, word_str);
	if (! lookedup)
	{
		// Couldn't find the word in the table
		*rval = JSVAL_NULL;
		return JS_TRUE;
	}

	// Create an object to be returned, containing enough data to access properties of the found word
	JSObject* wordobj = JS_NewObject(cx, &LookedupWord::JSI_class, NULL, obj);
	if (!wordobj)
	{
		JS_ReportError(cx, "lookup() failed to create object");
		return JS_FALSE;
	}
	// Associate the looked-up word data with the JS object
	JS_SetPrivate(cx, wordobj, (void*)lookedup);

	*rval = OBJECT_TO_JSVAL(wordobj);
	return JS_TRUE;
}

static JSFunctionSpec JSFunc_list[] = {
	{"lookup", JSFunc_LookupWord, 2, 0, 0},
	{0,0,0,0,0},
};


static JSClass JSI_class_scriptglobal = {
	"", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, NULL
};


ScriptObject::ScriptObject(CLocale* locale, JSContext* cx)
	: Context(cx)
{
	// Don't do much if there's currently no scripting support
	if (cx == NULL)
		return;

	// This should, in theory, never fail

	Object = JS_NewObject(Context, &JSI_class_scriptglobal, NULL, NULL);
	if (! Object)
	{
		debug_warn("Object creation failed");
		throw PSERROR_I18n_Script_SetupFailed();
	}
	JS_AddRoot(Context, &Object);

	// Register the 'global' functions
	if (! JS_DefineFunctions(Context, Object, JSFunc_list))
	{
		debug_warn("JS_DefineFunctions failed");
		throw PSERROR_I18n_Script_SetupFailed();
	}

	// Store the CLocale* ins in the script-object's private area
	JS_SetPrivate(Context, Object, locale);
}

ScriptObject::~ScriptObject()
{
	if (Context)
		JS_RemoveRoot(Context, &Object);
}

bool ScriptObject::ExecuteCode(const jschar* data, size_t len, const char* filename)
{
	jsval ret;
	return (JS_EvaluateUCScript(Context, Object, data, (int)len-1, filename, 1, &ret) ? true : false);
	//               use len-1 to stop JS crashing. *shrug* ^^^^^
}


const StrImW ScriptObject::CallFunction(const char* name, const std::vector<BufferVariable*>& vars, const std::vector<ScriptValue*>& params)
{
	int argc = (int)params.size();

	// Construct argv, converting each parameter into a jsval
	jsval* argv = new jsval[argc];
	for (int i = 0; i < argc; ++i)
		argv[i] = params[i]->GetJsval(vars);

	jsval rval;
	bool called = JS_CallFunctionName(Context, Object, name, argc, argv, &rval) ? true : false;

	delete[] argv;

	if (! called)
	{
		LOG(ERROR, LOG_CATEGORY, "I18n: Error executing JS function '%s'", name);
		return L"(JS error)";
	}

	// Convert rval to a string and return

	JSString* str = JS_ValueToString(Context, rval);
	if (! str)
	{
		debug_warn("Conversion to string failed");
		return L"(JS error)";
	}
	jschar* chars = JS_GetStringChars(str);
	if (! chars)
	{
		debug_warn("GetStringChars failed");
		return L"(JS error)";
	}
	return StrImW(chars, JS_GetStringLength(str));
}


ScriptValueString::ScriptValueString(ScriptObject& script, const wchar_t* s)
{
	Context = script.Context;

	JSString* str = StringConvert::wchars_to_jsstring(Context, s);
	if (!str)
	{
		debug_warn("Error creating JS string");
		Value = JSVAL_NULL;
	}
	else
	{
		JS_AddRoot(Context, str);
		Value = STRING_TO_JSVAL(str);
	}
}

jsval ScriptValueString::GetJsval(const std::vector<BufferVariable*>& vars)
{
	return Value;
}


ScriptValueString::~ScriptValueString()
{
	if (! JSVAL_IS_NULL(Value))
	{
		JSString* str = JSVAL_TO_STRING(Value);
		JS_RemoveRoot(Context, str);
	}
}

/************/

ScriptValueInteger::ScriptValueInteger(ScriptObject& script, const int v)
{
	Context = script.Context;
	Value = INT_TO_JSVAL(v);
}

jsval ScriptValueInteger::GetJsval(const std::vector<BufferVariable*>& vars)
{
	return Value;
}

ScriptValueInteger::~ScriptValueInteger()
{
}

/************/


ScriptValueVariable::ScriptValueVariable(ScriptObject& script, const unsigned char id)
{
	Context = script.Context;
	ID = id;
	GCVal = NULL;
}

jsval ScriptValueVariable::GetJsval(const std::vector<BufferVariable*>& vars)
{
	// Clean up from earlier invocations
	if (GCVal)
	{
		JS_RemoveRoot(Context, GCVal);
		GCVal = NULL;
	}

	switch (vars[ID]->Type)
	{
	case vartype_int:
		{
			int val = ((BufferVariable_int*)vars[ID])->value;
			return INT_TO_JSVAL(val);
		}
	case vartype_double:
		{
			jsdouble* val = JS_NewDouble(Context, ((BufferVariable_double*)vars[ID])->value);
			if (!val)
			{
				debug_warn("Error creating JS double");
				return JSVAL_NULL;
			}
			GCVal = (void*)val;
			JS_AddRoot(Context, val);
			return DOUBLE_TO_JSVAL(val);
		}
	case vartype_string:
		{
			JSString* val = StringConvert::wchars_to_jsstring(Context, ((BufferVariable_string*)vars[ID])->value.str());
			if (!val)
			{
				debug_warn("Error creating JS string");
				return JSVAL_NULL;
			}
			GCVal = (void*)val;
			JS_AddRoot(Context, val);
			return STRING_TO_JSVAL(val);
		}
	case vartype_rawstring:
		{
			JSString* val = StringConvert::wchars_to_jsstring(Context, ((BufferVariable_rawstring*)vars[ID])->value.str());
			if (!val)
			{
				debug_warn("Error creating JS string");
				return JSVAL_NULL;
			}
			GCVal = (void*)val;
			JS_AddRoot(Context, val);
			return STRING_TO_JSVAL(val);
		}
	default:
		debug_warn("Invalid type");
		return JSVAL_NULL;
	}
}

ScriptValueVariable::~ScriptValueVariable()
{
	if (GCVal)
		JS_RemoveRoot(Context, GCVal);
}
