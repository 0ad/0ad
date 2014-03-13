/* Copyright (C) 2013 Wildfire Games.
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

#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpPathfinder.h"

#include "graphics/MapReader.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureManager.h"
#include "lib/timer.h"
#include "lib/tex/tex.h"
#include "ps/Loader.h"
#include "simulation2/Simulation2.h"

class TestCmpPathfinder : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		CXeromyces::Startup();

		g_VFS = CreateVfs(20 * MiB);
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir()/"mods"/"public", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(g_VFS->Mount(L"cache/", DataDir()/"cache"));

		// Need some stuff for terrain movement costs:
		// (TODO: this ought to be independent of any graphics code)
		new CTerrainTextureManager;
		g_TexMan.LoadTerrainTextures();
	}

	void tearDown()
	{
		delete &g_TexMan;

		g_VFS.reset();

		CXeromyces::Terminate();
	}

	// disabled by default; run tests with the "-test TestCmpPathfinder" flag to enable
	void test_performance_DISABLED()
	{
		CTerrain terrain;

		CSimulation2 sim2(NULL, ScriptInterface::CreateRuntime(), &terrain);
		sim2.LoadDefaultScripts();
		sim2.ResetState();

		CMapReader* mapReader = new CMapReader(); // it'll call "delete this" itself

		LDR_BeginRegistering();
		mapReader->LoadMap(L"maps/scenarios/Median Oasis 01.pmp", CScriptValRooted(), &terrain, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			&sim2, &sim2.GetSimContext(), -1, false);
		LDR_EndRegistering();
		TS_ASSERT_OK(LDR_NonprogressiveLoad());

		sim2.Update(0);

		CmpPtr<ICmpPathfinder> cmp(sim2, SYSTEM_ENTITY);

#if 0
		entity_pos_t x0 = entity_pos_t::FromInt(10);
		entity_pos_t z0 = entity_pos_t::FromInt(495);
		entity_pos_t x1 = entity_pos_t::FromInt(500);
		entity_pos_t z1 = entity_pos_t::FromInt(495);
		ICmpPathfinder::Goal goal = { ICmpPathfinder::Goal::POINT, x1, z1 };

		ICmpPathfinder::Path path;
		cmp->ComputePath(x0, z0, goal, cmp->GetPassabilityClass("default"), cmp->GetCostClass("default"), path);
		for (size_t i = 0; i < path.m_Waypoints.size(); ++i)
			printf("%d: %f %f\n", (int)i, path.m_Waypoints[i].x.ToDouble(), path.m_Waypoints[i].z.ToDouble());
#endif

		double t = timer_Time();

		srand(1234);
		for (size_t j = 0; j < 1024*2; ++j)
		{
			entity_pos_t x0 = entity_pos_t::FromInt(rand() % 512);
			entity_pos_t z0 = entity_pos_t::FromInt(rand() % 512);
			entity_pos_t x1 = x0 + entity_pos_t::FromInt(rand() % 64);
			entity_pos_t z1 = z0 + entity_pos_t::FromInt(rand() % 64);
			ICmpPathfinder::Goal goal = { ICmpPathfinder::Goal::POINT, x1, z1 };

			ICmpPathfinder::Path path;
			cmp->ComputePath(x0, z0, goal, cmp->GetPassabilityClass("default"), cmp->GetCostClass("default"), path);
		}

		t = timer_Time() - t;
		printf("[%f]", t);
	}

	void test_performance_short_DISABLED()
	{
		CTerrain terrain;
		terrain.Initialize(5, NULL);

		CSimulation2 sim2(NULL, ScriptInterface::CreateRuntime(), &terrain);
		sim2.LoadDefaultScripts();
		sim2.ResetState();

		const entity_pos_t range = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*12);

		CmpPtr<ICmpObstructionManager> cmpObstructionMan(sim2, SYSTEM_ENTITY);
		CmpPtr<ICmpPathfinder> cmpPathfinder(sim2, SYSTEM_ENTITY);

		srand(0);
		for (size_t i = 0; i < 200; ++i)
		{
			fixed x = fixed::FromFloat(1.5f*range.ToFloat() * rand()/(float)RAND_MAX);
			fixed z = fixed::FromFloat(1.5f*range.ToFloat() * rand()/(float)RAND_MAX);
//			printf("# %f %f\n", x.ToFloat(), z.ToFloat());
			cmpObstructionMan->AddUnitShape(INVALID_ENTITY, x, z, fixed::FromInt(2), 0, INVALID_ENTITY);
		}

		NullObstructionFilter filter;
		ICmpPathfinder::Goal goal = { ICmpPathfinder::Goal::POINT, range, range };
		ICmpPathfinder::Path path;
		cmpPathfinder->ComputeShortPath(filter, range/3, range/3, fixed::FromInt(2), range, goal, 0, path);
		for (size_t i = 0; i < path.m_Waypoints.size(); ++i)
			printf("# %d: %f %f\n", (int)i, path.m_Waypoints[i].x.ToFloat(), path.m_Waypoints[i].z.ToFloat());
	}
};
