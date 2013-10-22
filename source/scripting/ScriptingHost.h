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


#ifndef INCLUDED_SCRIPTINGHOST
#define INCLUDED_SCRIPTINGHOST

#include "ps/Errors.h"

ERROR_GROUP(Scripting);
ERROR_TYPE(Scripting, SetupFailed);

ERROR_SUBGROUP(Scripting, LoadFile);
ERROR_TYPE(Scripting_LoadFile, OpenFailed);
ERROR_TYPE(Scripting_LoadFile, EvalErrors);

ERROR_TYPE(Scripting, ConversionFailed);
ERROR_TYPE(Scripting, CallFunctionFailed);
ERROR_TYPE(Scripting, RegisterFunctionFailed);
ERROR_TYPE(Scripting, DefineConstantFailed);
ERROR_TYPE(Scripting, CreateObjectFailed);
ERROR_TYPE(Scripting, TypeDoesNotExist);

ERROR_SUBGROUP(Scripting, DefineType);
ERROR_TYPE(Scripting_DefineType, AlreadyExists);
ERROR_TYPE(Scripting_DefineType, CreationFailed);

#include "scripting/SpiderMonkey.h"
#include "lib/file/vfs/vfs_path.h"

#include <string>
#include <vector>
#include <map>

#include "ps/Singleton.h"
#include "ps/CStr.h"

class ScriptInterface;

class IPropertyOwner
{
};

class ScriptingHost : public Singleton < ScriptingHost >
{
private:
	class CustomType
	{
	public:
		JSObject *	m_Object;
		JSClass *	m_Class;
	};

	JSContext *		m_Context;
	JSObject *		m_GlobalObject;

	std::map < std::string, CustomType >		m_CustomObjectTypes;

	// The long-term plan is to migrate from ScriptingHost to the newer shinier ScriptInterface.
	// For now, just have a ScriptInterface that hooks onto the ScriptingHost's context so they
	// can both be used.
	ScriptInterface* m_ScriptInterface;
public:

	ScriptingHost();
	~ScriptingHost();

	ScriptInterface& GetScriptInterface();

	static void FinalShutdown();
	
	// Helpers:

	// TODO: Remove one of these
	inline JSContext *getContext() { return m_Context; }
	inline JSContext *GetContext() { return m_Context; }

	inline JSObject* GetGlobalObject() { return m_GlobalObject; }

	void RunMemScript(const char* script, size_t size, const char* filename = 0, int line = 0, JSObject* globalObject = 0);
	void RunScript(const VfsPath& filename, JSObject* globalObject = 0);


	jsval ExecuteScript(const CStrW& script, const CStrW& calledFrom = L"Console", JSObject* contextObject = NULL );

	void DefineCustomObjectType(JSClass *clasp, JSNative constructor, uintN nargs, JSPropertySpec *ps, JSFunctionSpec *fs, JSPropertySpec *static_ps, JSFunctionSpec *static_fs);

	JSObject * CreateCustomObject(const std::string & typeName);

	void  SetObjectProperty(JSObject * object, const std::string & propertyName, jsval value);
	jsval GetObjectProperty(JSObject * object, const std::string & propertyName);

	void   SetObjectProperty_Double(JSObject* object, const char* propertyName, double value);
	double GetObjectProperty_Double(JSObject* object, const char* propertyName);

	void SetGlobal(const std::string& globalName, jsval value);

	CStrW ValueToUCString(const jsval value);
};

#define g_ScriptingHost ScriptingHost::GetSingleton()

#endif
