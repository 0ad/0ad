/* Copyright (C) 2010 Wildfire Games.
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

#include "simulation2/system/ComponentTest.h"

#include "ps/Filesystem.h"

class TestComponentScripts : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		g_VFS = CreateVfs(20 * MiB);
		g_VFS->Mount(L"", DataDir()/"mods/public", VFS_MOUNT_MUST_EXIST); // ignore directory-not-found errors
		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();
		g_VFS.reset();
	}

	static void load_script(ScriptInterface& scriptInterface, const VfsPath& pathname)
	{
		CVFSFile file;
		TS_ASSERT_EQUALS(file.Load(g_VFS, pathname), PSRETURN_OK);
		CStr content = file.GetAsString();
		std::wstring wcontent(content.begin(), content.end());
		TSM_ASSERT(L"Running script "+pathname.string(), scriptInterface.LoadScript(pathname, wcontent));
	}

	static void Script_LoadComponentScript(void* cbdata, VfsPath pathname)
	{
		CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);
		TS_ASSERT(componentManager->LoadScript(VfsPath(L"simulation/components") / pathname));
	}

	static void Script_LoadHelperScript(void* cbdata, VfsPath pathname)
	{
		CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);
		TS_ASSERT(componentManager->LoadScript(VfsPath(L"simulation/helpers") / pathname));
	}

	void test_scripts()
	{
		if (!VfsFileExists(L"simulation/components/tests/setup.js"))
		{
			debug_printf(L"WARNING: Skipping component scripts tests (can't find binaries/data/mods/public/simulation/components/tests/setup.js)\n");
			return;
		}

		VfsPaths paths;
		TS_ASSERT_OK(fs_util::GetPathnames(g_VFS, L"simulation/components/tests/", L"test_*.js", paths));
		for (size_t i = 0; i < paths.size(); ++i)
		{
			CSimContext context;
			CComponentManager componentManager(context, true);

			ScriptTestSetup(componentManager.GetScriptInterface());

			componentManager.GetScriptInterface().RegisterFunction<void, VfsPath, Script_LoadComponentScript> ("LoadComponentScript");
			componentManager.GetScriptInterface().RegisterFunction<void, VfsPath, Script_LoadHelperScript> ("LoadHelperScript");

			componentManager.LoadComponentTypes();

			load_script(componentManager.GetScriptInterface(), L"simulation/components/tests/setup.js");
			load_script(componentManager.GetScriptInterface(), paths[i]);
		}
	}
};
