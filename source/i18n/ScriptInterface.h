/*

ScriptObject provides an interface to the JavaScript engine.

ScriptValues are used to generate jsvals from 'constants' (strings, ints)
and 'variables' (dependent on the type passed with "translate(...) <<").

*/

#ifndef I18N_SCRIPTINTERFACE_H
#define I18N_SCRIPTINTERFACE_H

#include "Common.h"
#include "BufferVariable.h"

#include <vector>

ERROR_SUBGROUP(I18n, Script);
ERROR_TYPE(I18n_Script, SetupFailed);

typedef _W64 long jsval;
typedef unsigned short jschar;
struct JSContext;
struct JSObject;

namespace I18n
{

	class ScriptValue;
	class CLocale;

	class ScriptObject
	{
	public:
		ScriptObject(CLocale* locale, JSContext* cx);
		~ScriptObject();

		bool ExecuteCode(const jschar* data, size_t len, const char* filename);

		const StrImW CallFunction(const char* name, const std::vector<BufferVariable*>& vars, const std::vector<ScriptValue*>& params);

		JSContext* Context;
		JSObject* Object;
	};

	// Virtual base class for script values
	class ScriptValue
	{
	public:
		virtual jsval GetJsval(const std::vector<BufferVariable*>& vars) = 0;
		virtual ~ScriptValue() {};

	protected:
		JSContext* Context;
	};

	// Particular types of script value:

	class ScriptValueString : public ScriptValue
	{
	public:
		ScriptValueString(ScriptObject& script, const wchar_t* s);
		~ScriptValueString();
		jsval GetJsval(const std::vector<BufferVariable*>& vars);

	private:
		jsval Value;
	};

	class ScriptValueInteger : public ScriptValue
	{
	public:
		ScriptValueInteger(ScriptObject& script, const int v);
		~ScriptValueInteger();
		jsval GetJsval(const std::vector<BufferVariable*>& vars);

	private:
		jsval Value;
	};

	class ScriptValueVariable : public ScriptValue
	{
	public:
		ScriptValueVariable(ScriptObject& script, const unsigned char id);
		~ScriptValueVariable();
		jsval GetJsval(const std::vector<BufferVariable*>& vars);

	private:
    	unsigned char ID;
		void* GCVal; // something that's being garbage-collected
	};
}

#endif // I18N_SCRIPTINTERFACE_H
