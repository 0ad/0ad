/*

Stores sections of a translated string: constant strings, simple variables,
and JS function calls, allowing conversion to strings.

*/

#ifndef INCLUDED_I18N_TSCOMPONENT
#define INCLUDED_I18N_TSCOMPONENT

#include "StrImmutable.h"
#include "BufferVariable.h"
#include "ScriptInterface.h"

#include <algorithm>
#include <vector>

namespace I18n
{
	class CLocale;

	class TSComponent
	{
	public:
		virtual const StrImW ToString(CLocale* locale, std::vector<BufferVariable*>& vars) const = 0;
		virtual ~TSComponent() {}
	};

	
	class TSComponentString : public TSComponent, noncopyable
	{
	public:
		TSComponentString(const wchar_t* s) : String(s) {}

		const StrImW ToString(CLocale* locale, std::vector<BufferVariable*>& vars) const;

	private:
		const StrImW String;
	};


	class TSComponentVariable : public TSComponent, noncopyable
	{
	public:
		TSComponentVariable(unsigned char id) : ID(id) {}

		const StrImW ToString(CLocale* locale, std::vector<BufferVariable*>& vars) const;

	private:
		unsigned char ID;
	};


	class TSComponentFunction : public TSComponent, noncopyable
	{
	public:
		TSComponentFunction(const char* name) : Name(name) {}
		~TSComponentFunction();

		const StrImW ToString(CLocale* locale, std::vector<BufferVariable*>& vars) const;

		// AddParam is called when loading the function, building up the
		// internal list of parameters (strings / ints / variables)
		void AddParam(ScriptValue* p);

	private:
		const std::string Name;
		std::vector<ScriptValue*> Params;
	};


}

#endif // INCLUDED_I18N_TSCOMPONENT
