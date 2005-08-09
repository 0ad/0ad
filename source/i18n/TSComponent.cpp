#include "precompiled.h"

#include "TSComponent.h"
#include "CLocale.h"

using namespace I18n;

// Vaguely useful utility function for deleting stuff
template<typename T> void delete_fn(T* v) { delete v; }

const StrImW TSComponentString::ToString(CLocale* UNUSED(locale), std::vector<BufferVariable*>& UNUSED(vars)) const
{
	return String;
}

/**************/

const StrImW TSComponentVariable::ToString(CLocale* locale, std::vector<BufferVariable*>& vars) const
{
	// This will never be out-of-bounds -- the number
	// of parameters has been checked earlier
	return vars[ID]->ToString(locale);
}

/**************/

const StrImW TSComponentFunction::ToString(CLocale* locale, std::vector<BufferVariable*>& vars) const
{
	return locale->CallFunction(Name.c_str(), vars, Params);
}

void TSComponentFunction::AddParam(ScriptValue* p)
{
	Params.push_back(p);
}

TSComponentFunction::~TSComponentFunction()
{
	for_each(Params.begin(), Params.end(), delete_fn<ScriptValue>);
}

