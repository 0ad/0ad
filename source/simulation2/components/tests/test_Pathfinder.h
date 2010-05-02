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

#include "simulation2/components/ICmpPathfinder.h"

#include "graphics/MapReader.h"
#include "graphics/Terrain.h"
#include "graphics/TextureManager.h"
#include "ps/Loader.h"
#include "ps/Profile.h"
#include "ps/ProfileViewer.h"
#include "scripting/ScriptingHost.h"
#include "simulation/EntityTemplateCollection.h"
#include "simulation2/Simulation2.h"

class TestCmpPathfinder : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		g_UseSimulation2 = true;

		CXeromyces::Startup();

		g_VFS = CreateVfs(20 * MiB);
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir()/L"mods/public"));
		TS_ASSERT_OK(g_VFS->Mount(L"cache/", DataDir()/L"cache"));

		// Set up loads of stuff that's needed in order to load a map
		new CTextureManager();
		new CProfileViewer();
		new CProfileManager();
		new ScriptingHost();
		new CEntityTemplateCollection();
		g_EntityTemplateCollection.LoadTemplates();
	}

	void tearDown()
	{
		delete &g_EntityTemplateCollection;
		delete &g_ScriptingHost;
		delete &g_Profiler;
		delete &g_ProfileViewer;
		delete &g_TexMan;

		g_VFS.reset();

		CXeromyces::Terminate();

		g_UseSimulation2 = false;
	}

	// disabled by default; run tests with the "-test TestCmpPathfinder" flag to enable
	void test_performance_DISABLED()
	{
		CTerrain terrain;

		CSimulation2 sim2(NULL, &terrain);
		sim2.LoadDefaultScripts();
		sim2.ResetState();

		CMapReader* mapReader = new CMapReader(); // it'll call "delete this" itself

		LDR_BeginRegistering();
		mapReader->LoadMap(L"maps/scenarios/Latium.pmp", &terrain, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &sim2, NULL);
		LDR_EndRegistering();
		TS_ASSERT_OK(LDR_NonprogressiveLoad());

		sim2.Update(0);

		CmpPtr<ICmpPathfinder> cmp(sim2, SYSTEM_ENTITY);

#if 0
		entity_pos_t x0 = entity_pos_t::FromInt(10);
		entity_pos_t z0 = entity_pos_t::FromInt(495);
		entity_pos_t x1 = entity_pos_t::FromInt(500);
		entity_pos_t z1 = entity_pos_t::FromInt(495);
		ICmpPathfinder::Goal goal = { x1, z1, entity_pos_t::FromInt(0), entity_pos_t::FromInt(0) };

		ICmpPathfinder::Path path;
		cmp->ComputePath(x0, z0, goal, path);
		for (size_t i = 0; i < path.m_Waypoints.size(); ++i)
			printf("%d: %f %f\n", (int)i, path.m_Waypoints[i].x.ToDouble(), path.m_Waypoints[i].z.ToDouble());
#endif

		srand(1234);
		for (size_t j = 0; j < 2560; ++j)
		{
			entity_pos_t x0 = entity_pos_t::FromInt(rand() % 512);
			entity_pos_t z0 = entity_pos_t::FromInt(rand() % 512);
			entity_pos_t x1 = entity_pos_t::FromInt(rand() % 512);
			entity_pos_t z1 = entity_pos_t::FromInt(rand() % 512);
			ICmpPathfinder::Goal goal = { ICmpPathfinder::Goal::POINT, x1, z1 };

			ICmpPathfinder::Path path;
			cmp->ComputePath(x0, z0, goal, path);
		}
	}
};
