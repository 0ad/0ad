/* Copyright (C) 2019 Wildfire Games.
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

/**
 * A class using the RAII (Resource Acquisition Is Initialization) idiom to manage initialization
 * and shutdown of the SpiderMonkey script engine. It also keeps a count of active script runtimes
 * in order to validate the following constraints:
 *  1. JS_Init must be called before any ScriptRuntimes are initialized
 *  2. JS_Shutdown must be called after all ScriptRuntimes have been destroyed
 */

class ScriptEngine : public Singleton<ScriptEngine>
{
public:
	ScriptEngine()
	{
		ENSURE(m_Runtimes.empty() && "JS_Init must be called before any runtimes are created!");
		JS_Init();
	}

	~ScriptEngine()
	{
		ENSURE(m_Runtimes.empty() && "All runtimes must be destroyed before calling JS_ShutDown!");
		JS_ShutDown();
	}

	void RegisterRuntime(const JSRuntime* rt) { m_Runtimes.push_back(rt); }
	void UnRegisterRuntime(const JSRuntime* rt) { m_Runtimes.remove(rt); }

private:
	std::list<const JSRuntime*> m_Runtimes;
};

#endif // INCLUDED_SCRIPTENGINE
