/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "simulation2/system/ComponentTest.h"
#include "simulation2/serialization/StdDeserializer.h"
#include "simulation2/serialization/StdSerializer.h"

#include "ps/Filesystem.h"

#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptContext.h"

class TestComponentScripts : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		g_VFS = CreateVfs();
		g_VFS->Mount(L"", DataDir() / "mods" / "mod" / "", VFS_MOUNT_MUST_EXIST);
		g_VFS->Mount(L"", DataDir() / "mods" / "public" / "", VFS_MOUNT_MUST_EXIST, 1); // ignore directory-not-found errors
		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();
		g_VFS.reset();
	}

	static void load_script(const ScriptInterface& scriptInterface, const VfsPath& pathname)
	{
		CVFSFile file;
		TS_ASSERT_EQUALS(file.Load(g_VFS, pathname), PSRETURN_OK);
		CStr content = file.DecodeUTF8(); // assume it's UTF-8
		TSM_ASSERT(L"Running script "+pathname.string(), scriptInterface.LoadScript(pathname, content));
	}

	static void Script_LoadComponentScript(const ScriptInterface& scriptInterface, const VfsPath& pathname)
	{
		ScriptRequest rq(scriptInterface);
		CComponentManager* componentManager = scriptInterface.ObjectFromCBData<CComponentManager>(rq);
		TS_ASSERT(componentManager->LoadScript(VfsPath(L"simulation/components") / pathname));
	}

	static void Script_LoadHelperScript(const ScriptInterface& scriptInterface, const VfsPath& pathname)
	{
		ScriptRequest rq(scriptInterface);
		CComponentManager* componentManager = scriptInterface.ObjectFromCBData<CComponentManager>(rq);
		TS_ASSERT(componentManager->LoadScript(VfsPath(L"simulation/helpers") / pathname));
	}

	static JS::Value Script_SerializationRoundTrip(const ScriptInterface& scriptInterface, JS::HandleValue value)
	{
		ScriptRequest rq(scriptInterface);

		JS::RootedValue val(rq.cx);
		val = value;
		std::stringstream stream;
		CStdSerializer serializer(scriptInterface, stream);
		serializer.ScriptVal("", &val);
		CStdDeserializer deserializer(scriptInterface, stream);
		deserializer.ScriptVal("", &val);
		return val;
	}

	void test_global_scripts()
	{
		if (!VfsDirectoryExists(L"globalscripts/tests/"))
		{
			debug_printf("Skipping globalscripts tests (can't find binaries/data/mods/public/globalscripts/tests/)\n");
			return;
		}

		VfsPaths paths;
		TS_ASSERT_OK(vfs::GetPathnames(g_VFS, L"globalscripts/tests/", L"test_*.js", paths));
		for (const VfsPath& path : paths)
		{
			CSimContext context;
			CComponentManager componentManager(context, *g_ScriptContext, true);
			ScriptTestSetup(componentManager.GetScriptInterface());

			ScriptRequest rq(componentManager.GetScriptInterface());
			ScriptFunction::Register<Script_SerializationRoundTrip>(rq, "SerializationRoundTrip");

			load_script(componentManager.GetScriptInterface(), path);
		}
	}

	void test_scripts()
	{
		if (!VfsFileExists(L"simulation/components/tests/setup.js"))
		{
			debug_printf("Skipping component scripts tests (can't find binaries/data/mods/public/simulation/components/tests/setup.js)\n");
			return;
		}

		VfsPaths paths;
		TS_ASSERT_OK(vfs::GetPathnames(g_VFS, L"simulation/components/tests/", L"test_*.js", paths));
		TS_ASSERT_OK(vfs::GetPathnames(g_VFS, L"simulation/helpers/tests/", L"test_*.js", paths));
		paths.push_back(VfsPath(L"simulation/components/tests/setup_test.js"));
		for (const VfsPath& path : paths)
		{
			// Clean up previous scripts.
			g_ScriptContext->ShrinkingGC();
			CSimContext context;
			CComponentManager componentManager(context, *g_ScriptContext, true);

			ScriptTestSetup(componentManager.GetScriptInterface());

			ScriptRequest rq(componentManager.GetScriptInterface());
			ScriptFunction::Register<Script_LoadComponentScript>(rq, "LoadComponentScript");
			ScriptFunction::Register<Script_LoadHelperScript>(rq, "LoadHelperScript");
			ScriptFunction::Register<Script_SerializationRoundTrip>(rq, "SerializationRoundTrip");

			componentManager.LoadComponentTypes();

			load_script(componentManager.GetScriptInterface(), L"simulation/components/tests/setup.js");
			load_script(componentManager.GetScriptInterface(), path);
		}
	}
};
