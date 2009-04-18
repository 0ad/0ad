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

