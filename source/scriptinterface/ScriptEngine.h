/* Copyright (C) 2020 Wildfire Games.
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

#ifndef INCLUDED_SCRIPTENGINE
#define INCLUDED_SCRIPTENGINE

#include "ScriptTypes.h"
#include "ps/Singleton.h"

#include "js/Initialization.h"

/**
 * A class using the RAII (Resource Acquisition Is Initialization) idiom to manage initialization
 * and shutdown of the SpiderMonkey script engine. It also keeps a count of active script contexts
 * in order to validate the following constraints:
 *  1. JS_Init must be called before any ScriptContexts are initialized
 *  2. JS_Shutdown must be called after all ScriptContexts have been destroyed
 */

class ScriptEngine : public Singleton<ScriptEngine>
{
public:
	ScriptEngine()
	{
		ENSURE(m_Contexts.empty() && "JS_Init must be called before any contexts are created!");
		JS_Init();
	}

	~ScriptEngine()
	{
		ENSURE(m_Contexts.empty() && "All contexts must be destroyed before calling JS_ShutDown!");
		JS_ShutDown();
	}

	void RegisterContext(const JSContext* cx) { m_Contexts.push_back(cx); }
	void UnRegisterContext(const JSContext* cx) { m_Contexts.remove(cx); }

private:
	std::list<const JSContext*> m_Contexts;
};

#endif // INCLUDED_SCRIPTENGINE
