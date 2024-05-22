/* Copyright (C) 2024 Wildfire Games.
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

#include "graphics/MapGenerator.h"
#include "ps/Filesystem.h"
#include "simulation2/system/ComponentTest.h"

#include <atomic>

class TestMapGenerator : public CxxTest::TestSuite
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

	void test_mapgen_scripts()
	{
		if (!VfsDirectoryExists(L"maps/random/tests/"))
		{
			debug_printf("Skipping map generator tests (can't find binaries/data/mods/public/maps/random/tests/)\n");
			return;
		}

		VfsPaths paths;
		TS_ASSERT_OK(vfs::GetPathnames(g_VFS, L"maps/random/tests/", L"test_*.js", paths));

		for (const VfsPath& path : paths)
		{
			TestLogger logger;
			ScriptInterface scriptInterface("Engine", "MapGenerator", g_ScriptContext);
			ScriptTestSetup(scriptInterface);

			std::atomic<int> progress{1};

			const Script::StructuredClone result{RunMapGenerationScript(progress, scriptInterface,
				path, "{\"Seed\": 0}", JSPROP_ENUMERATE | JSPROP_PERMANENT)};

			if (path == "maps/random/tests/test_Generator.js" ||
				path == "maps/random/tests/test_RecoverableError.js")
			{
				TS_ASSERT_EQUALS(progress.load(), 50);
				TS_ASSERT_DIFFERS(result, nullptr);
			}
			else
			{
				// The test scripts don't call `ExportMap` so `RunMapGenerationScript` allways
				// returns `nullptr`.
				TS_ASSERT_EQUALS(result, nullptr);
				// Because the test scripts don't call `ExportMap`, `GenerateMap` is searched, which
				// doesn't exist.
				TS_ASSERT_STR_CONTAINS(logger.GetOutput(),
					"Failed to call the generator `GenerateMap`.");
			}
		}
	}
};
