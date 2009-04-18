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

#include "precompiled.h"

#include "ScriptInterface.h"
#include "CLocale.h"
#include "ps/StringConvert.h"
#include "scripting/SpiderMonkey.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "i18n"

using namespace I18n;

// LookedupWord JS class:
namespace JSI_LookedupWord {

	static JSBool GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
	{
		CLocale::LookupType* lookedup = (CLocale::LookupType*)JS_GetPrivate(cx, obj);
		debug_assert(lookedup);

		JSObject* parent = JS_GetParent(cx, obj);
		debug_assert(parent);
		CLocale* locale = (CLocale*)JS_GetPrivate(cx, parent);
		debug_assert(locale);

		JSString* prop = JS_ValueToString(cx, id);
		JSU_ASSERT(prop, "lookup() property failed to convert to string");

		jschar* prop_chars = JS_GetStringChars(prop);

		Str prop_str;
		StringConvert::jschars_to_wstring(prop_chars, JS_GetStringLength(prop), prop_str);

		Str result;
		if (! locale->LookupProperty(lookedup, prop_str, result))
			result = L"(unrecognised property)";

		JSString* result_str = StringConvert::wstring_to_jsstring(cx, result);
		JSU_ASSERT(result_str, "lookup() property failed to create string");

		*vp = STRING_TO_JSVAL(result_str);
		
		return JS_TRUE;
	}

	static void Finalize(JSContext *cx, JSObject *obj)
	{
		// Free the LookupType that was allocated when building this object

		CLocale::LookupType* lookedup = (CLocale::LookupType*)JS_GetPrivate(cx, obj);
		debug_assert(lookedup);
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


// 'i18n' JS class:
namespace JSI_i18n {

	#define TYPE(x)	\
	static JSBool Create_##x(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval)	\
	{																								\
		/* Set *rval = { type => "Name", value => argv[0] } */			\
																		\
		JSObject* object = JS_NewObject(cx, NULL, NULL, obj);			\
		JSU_ASSERT(object, "Failed to create i18n value object");		\
																		\
		/* TODO: More error checking */									\
		JSU_REQUIRE_PARAMS(1);                                          \
		jsval type = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, #x));		\
		jsval value = STRING_TO_JSVAL(JS_ValueToString(cx, argv[0]));	\
		JS_SetProperty(cx, object, "type", &type);						\
		JS_SetProperty(cx, object, "value", &value);					\
																		\
		*rval = OBJECT_TO_JSVAL(object);								\
																		\
		return JS_TRUE;													\
	}
	TYPE(Name)
	TYPE(Raw)
	TYPE(Noun)
	#undef TYPE

	static JSClass JSI_class = {
		"JSI_i18n", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub,
		JS_PropertyStub, JS_PropertyStub,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub,
		NULL, NULL, NULL, NULL
	};

	#define TYPE(x) {#x, Create_##x, 1, 0, 0}
	static JSFunctionSpec JSI_funcs[] = {
		TYPE(Name),
		TYPE(Raw),
		TYPE(Noun),
		{0,0,0,0,0},
	};
	#undef TYPE
}


static JSBool JSFunc_LookupWord(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSU_REQUIRE_PARAMS(2);

	// Get the strings
	JSString* dictname = JS_ValueToString(cx, argv[0]);
	JSString* word = JS_ValueToString(cx, argv[1]);
	JSU_ASSERT(dictname && word, "lookup() failed to convert parameters to strings");

	// and the characters from the strings
	jschar* dictname_chars = JS_GetStringChars(dictname);
	jschar* word_chars = JS_GetStringChars(word);
	// (can't fail)

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
	JSObject* wordobj = JS_NewObject(cx, &JSI_LookedupWord::JSI_class, NULL, obj);
	JSU_ASSERT(wordobj, "lookup() failed to create object");

	// Associate the looked-up word data with the JS object
	JS_SetPrivate(cx, wordobj, (void*)lookedup);

	*rval = OBJECT_TO_JSVAL(wordobj);
	return JS_TRUE;
}

static JSBool JSFunc_Translate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSU_ASSERT(argc >= 1, "Too few parameters to translate() function");

	JSString* phrase_str = JS_ValueToString(cx, argv[0]);
	JSU_ASSERT(phrase_str, "translate() failed to convert first parameter to string");

	CStrW phrase (JS_GetStringChars(phrase_str));

	jsval locale_objval;
	JSU_ASSERT(JS_GetProperty(cx, obj, "i18n", &locale_objval), "translate() failed to find i18n object in current scope");
	CLocale* locale = (CLocale*)JS_GetPrivate(cx, JSVAL_TO_OBJECT(locale_objval));

	StringBuffer sb = locale->Translate(phrase.c_str());
	for (uintN i=1; i<argc; ++i)
	{
		if (JSVAL_IS_INT(argv[i]))
		{
			sb << (int) JSVAL_TO_INT(argv[i]);
			continue;
		}

		if (JSVAL_IS_DOUBLE(argv[i]))
		{
			sb << *JSVAL_TO_DOUBLE(argv[i]);
			continue;
		}

		if (JSVAL_IS_OBJECT(argv[i]))
		{
			// Check for 'type' and 'value' properties
			jsval type, value;
			if (JS_GetProperty(cx, JSVAL_TO_OBJECT(argv[i]), "type", &type)
			 && type != JSVAL_VOID
			 && JS_GetProperty(cx, JSVAL_TO_OBJECT(argv[i]), "value", &value)
			 && value != JSVAL_VOID)
			{
				// TODO: More error handling
				std::string typestr = JS_GetStringBytes(JS_ValueToString(cx, type));

				std::wstring val;
				if (typestr == "Name")
				{
					JSString* s = JS_ValueToString(cx, value);
					sb << I18n::Name(JS_GetStringChars(s));
					continue;
				}
				else if (typestr == "Raw")
				{
					JSString* s = JS_ValueToString(cx, value);
					sb << I18n::Raw(JS_GetStringChars(s));
					continue;
				}
				else if (typestr == "Noun")
				{
					JSString* s = JS_ValueToString(cx, value);
					sb << I18n::Noun(JS_GetStringChars(s));
					continue;
				}
			}
		}

		JS_ReportError(cx, "Invalid parameter passed to translate() (must be a number or a i18n.something() object)");
		return JS_FALSE;
	}

	JSString* result_str = StringConvert::wstring_to_jsstring(cx, sb);
	*rval = STRING_TO_JSVAL(result_str);

	return JS_TRUE;
}


// Visible to all JS code:
static JSFunctionSpec JSI_i18nInterfaceFunctions[] = {
	{"translate", JSFunc_Translate, 0, 0, 0},
	{0,0,0,0,0},
};

// Visible to functions called by the i18n system:
static JSFunctionSpec JSI_i18nFunctions[] = {
	{"lookup", JSFunc_LookupWord, 2, 0, 0},
	{0,0,0,0,0},
};

// Object under which to run functions called by the i18n system:
static JSClass JSI_i18nFunctionScope = {
	"", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, NULL
};


ScriptObject::ScriptObject(CLocale* locale, JSContext* cx, JSObject* scope)
	: Context(cx)
{
	// Don't do much if there's currently no scripting support
	if (cx == NULL)
		return;

	// This constructor should, in theory, never fail

	// Create the object [/scope] under which the locale-dependent translation
	// functions are executed
	Object = JS_NewObject(Context, &JSI_i18nFunctionScope, NULL, NULL);
	if (! Object)
		throw PSERROR_I18n_Script_SetupFailed();

	JS_AddRoot(Context, &Object);
	// (This will get leaked if the constructor throws, but there will be more
	// important things to worry about than memory leaks in those situations)

	// Register the functions for use by i18n translations
	if (! JS_DefineFunctions(Context, Object, JSI_i18nFunctions))
		throw PSERROR_I18n_Script_SetupFailed();

	// Store the CLocale* in the script-object's private area
	JS_SetPrivate(Context, Object, locale);

	// Interface setup:

	// Register the interface functions
	if (! JS_DefineFunctions(Context, scope, JSI_i18nInterfaceFunctions))
		throw PSERROR_I18n_Script_SetupFailed();

	// Create the 'i18n' interface object
	JSObject* i18nObject = JS_DefineObject(Context, scope, "i18n", &JSI_i18n::JSI_class, NULL, JSPROP_READONLY | JSPROP_PERMANENT);
	if (! i18nObject)
		throw PSERROR_I18n_Script_SetupFailed();

	// Define its functions (i18n.Name() etc)
	if (! JS_DefineFunctions(Context, i18nObject, JSI_i18n::JSI_funcs))
		throw PSERROR_I18n_Script_SetupFailed();

	// Store the CLocale* in the i18n object's private area
	JS_SetPrivate(Context, i18nObject, locale);

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
		LOG(CLogger::Error, LOG_CATEGORY, "I18n: Error executing JS function '%s'", name);
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
		Value = STRING_TO_JSVAL(str);
		JS_AddRoot(Context, &Value);
	}
}

jsval ScriptValueString::GetJsval(const std::vector<BufferVariable*>& UNUSED(vars))
{
	return Value;
}


ScriptValueString::~ScriptValueString()
{
	if (! JSVAL_IS_NULL(Value))
	{
		JS_RemoveRoot(Context, &Value);
	}
}

/************/

ScriptValueInteger::ScriptValueInteger(ScriptObject& script, const int v)
{
	Context = script.Context;
	Value = INT_TO_JSVAL(v);
}

jsval ScriptValueInteger::GetJsval(const std::vector<BufferVariable*>& UNUSED(vars))
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
		JS_RemoveRoot(Context, &GCVal);
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
			JS_AddRoot(Context, &GCVal);
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
			JS_AddRoot(Context, &GCVal);
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
			JS_AddRoot(Context, &GCVal);
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
		JS_RemoveRoot(Context, &GCVal);
}
