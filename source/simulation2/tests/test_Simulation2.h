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

#include "lib/self_test.h"

#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpTest.h"

#include "graphics/Terrain.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"

class TestSimulation2 : public CxxTest::TestSuite
{
	void copyFile(const VfsPath& src, const VfsPath& dst)
	{
		shared_ptr<u8> data;
		size_t size = 0;
		TS_ASSERT_OK(g_VFS->LoadFile(src, data, size));
		TS_ASSERT_OK(g_VFS->CreateFile(dst, data, size));
	}

	CTerrain m_Terrain;

public:
	void setUp()
	{
		g_VFS = CreateVfs(20 * MiB);
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir()/L"mods/_test.sim"));
		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();
		g_VFS.reset();
	}

	void test_AllocateNewEntity()
	{
		CSimulation2 sim(NULL, &m_Terrain);
		sim.ResetState(true);
		TS_ASSERT_EQUALS(sim.AllocateNewEntity(), (u32)2);
		TS_ASSERT_EQUALS(sim.AllocateNewEntity(), (u32)3);
		TS_ASSERT_EQUALS(sim.AllocateNewEntity(), (u32)4);
	}

	void test_AddEntity()
	{
		CSimulation2 sim(NULL, &m_Terrain);
		TS_ASSERT(sim.LoadScripts(L"simulation/components/addentity/"));

		sim.ResetState(true);

		entity_id_t ent1 = sim.AddEntity(L"test1");
		TS_ASSERT_EQUALS(ent1, (u32)2);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (sim.QueryInterface(ent1, IID_Test1))->GetX(), 999);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (sim.QueryInterface(ent1, IID_Test2))->GetX(), 12345);

		entity_id_t ent2 = sim.AddEntity(L"test1-inherit");
		TS_ASSERT_EQUALS(ent2, (u32)3);

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (sim.QueryInterface(ent2, IID_Test1))->GetX(), 1234);
		TS_ASSERT_EQUALS(static_cast<ICmpTest2*> (sim.QueryInterface(ent2, IID_Test2))->GetX(), 12345);
	}

	void test_hotload_scripts()
	{
		CSimulation2 sim(NULL, &m_Terrain);

		TS_ASSERT_OK(CreateDirectories(DataDir()/L"mods/_test.sim/simulation/components/hotload/", 0700));

		copyFile(L"simulation/components/test-hotload1.js", L"simulation/components/hotload/hotload.js");
		TS_ASSERT_OK(g_VFS->Invalidate(L"simulation/components/hotload/hotload.js"));
		TS_ASSERT(sim.LoadScripts(L"simulation/components/hotload/"));

		sim.ResetState(true);

		entity_id_t ent = sim.AddEntity(L"hotload");

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (sim.QueryInterface(ent, IID_Test1))->GetX(), 100);

		copyFile(L"simulation/components/test-hotload2.js", L"simulation/components/hotload/hotload.js");
		TS_ASSERT_OK(g_VFS->Invalidate(L"simulation/components/hotload/hotload.js"));

		TS_ASSERT_OK(sim.ReloadChangedFile(L"art/irrelevant.xml"));
		TS_ASSERT_OK(sim.ReloadChangedFile(L"simulation/components/irrelevant.js"));
		TS_ASSERT_OK(sim.ReloadChangedFile(L"simulation/components/hotload/nonexistent.js"));

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (sim.QueryInterface(ent, IID_Test1))->GetX(), 100);

		TS_ASSERT_OK(sim.ReloadChangedFile(L"simulation/components/hotload/hotload.js"));

		TS_ASSERT_EQUALS(static_cast<ICmpTest1*> (sim.QueryInterface(ent, IID_Test1))->GetX(), 1000);

		TS_ASSERT_OK(DeleteDirectory(DataDir()/L"mods/_test.sim/simulation/components/hotload/"));
		TS_ASSERT_OK(g_VFS->Invalidate(L"simulation/components/hotload/hotload.js"));

		TS_ASSERT_OK(sim.ReloadChangedFile(L"simulation/components/hotload/hotload.js"));
	}
};
