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

#include "simulation2/system/ComponentTest.h"

#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpPathfinder.h"

#include "graphics/MapReader.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureManager.h"
#include "lib/timer.h"
#include "lib/tex/tex.h"
#include "ps/Loader.h"
#include "ps/Pyrogenesis.h"
#include "simulation2/Simulation2.h"

class TestCmpPathfinder : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		g_VFS = CreateVfs();
		g_VFS->Mount(L"", DataDir()/"mods"/"mod", VFS_MOUNT_MUST_EXIST);
		g_VFS->Mount(L"", DataDir()/"mods"/"public", VFS_MOUNT_MUST_EXIST, 1); // ignore directory-not-found errors
		TS_ASSERT_OK(g_VFS->Mount(L"cache", DataDir()/"_testcache"));

		CXeromyces::Startup();

		// Need some stuff for terrain movement costs:
		// (TODO: this ought to be independent of any graphics code)
		new CTerrainTextureManager;
		g_TexMan.LoadTerrainTextures();
	}

	void tearDown()
	{
		delete &g_TexMan;
		CXeromyces::Terminate();
		g_VFS.reset();
		DeleteDirectory(DataDir()/"_testcache");
	}

	void test_namespace()
	{
		// Check that Pathfinding::NAVCELL_SIZE is actually an integer and that the definitions
		// of Pathfinding::NAVCELL_SIZE_INT and Pathfinding::NAVCELL_SIZE_LOG2 match
		TS_ASSERT_EQUALS(Pathfinding::NAVCELL_SIZE.ToInt_RoundToNegInfinity(), Pathfinding::NAVCELL_SIZE.ToInt_RoundToInfinity());
		TS_ASSERT_EQUALS(Pathfinding::NAVCELL_SIZE.ToInt_RoundToNearest(), Pathfinding::NAVCELL_SIZE_INT);
		TS_ASSERT_EQUALS((Pathfinding::NAVCELL_SIZE >> 1).ToInt_RoundToZero(), Pathfinding::NAVCELL_SIZE_LOG2);
	}

	void test_pathgoal_nearest_distance()
	{
		entity_pos_t i = Pathfinding::NAVCELL_SIZE;
		CFixedVector2D u(i*1, i*0);
		CFixedVector2D v(i*0, i*1);

		{
			PathGoal goal = { PathGoal::POINT, i*8, i*6 };
			TS_ASSERT_EQUALS(goal.NearestPointOnGoal(u*8 + v*4), u*8 + v*6);
			TS_ASSERT_EQUALS(goal.DistanceToPoint(u*8 + v*4), i*2);
			TS_ASSERT_EQUALS(goal.NearestPointOnGoal(u*0 + v*0), u*8 + v*6);
			TS_ASSERT_EQUALS(goal.DistanceToPoint(u*0 + v*0), i*10);
			TS_ASSERT(goal.RectContainsGoal(i*4, i*3, i*12, i*9));
			TS_ASSERT(goal.RectContainsGoal(i*4, i*3, i*8, i*6));
			TS_ASSERT(goal.RectContainsGoal(i*8, i*6, i*12, i*9));
			TS_ASSERT(!goal.RectContainsGoal(i*4, i*3, i*7, i*5));
			TS_ASSERT(!goal.RectContainsGoal(i*9, i*7, i*13, i*15));
		}

		{
			PathGoal goal = { PathGoal::CIRCLE, i*8, i*6, i*5 };
			TS_ASSERT_EQUALS(goal.NearestPointOnGoal(u*8 + v*4), u*8 + v*4);
			TS_ASSERT_EQUALS(goal.DistanceToPoint(u*8 + v*4), i*0);
			TS_ASSERT_EQUALS(goal.NearestPointOnGoal(u*0 + v*0), u*4 + v*3);
			TS_ASSERT_EQUALS(goal.DistanceToPoint(u*0 + v*0), i*5);
			TS_ASSERT(goal.RectContainsGoal(i*7, i*5, i*9, i*7)); // fully inside
			TS_ASSERT(goal.RectContainsGoal(i*3, i*1, i*13, i*11)); // fully outside
			TS_ASSERT(goal.RectContainsGoal(i*4, i*3, i*8, i*6)); // partially inside
			TS_ASSERT(goal.RectContainsGoal(i*4, i*0, i*12, i*1)); // touching the edge
		}

		{
			PathGoal goal = { PathGoal::INVERTED_CIRCLE, i*8, i*6, i*5 };
			TS_ASSERT_EQUALS(goal.NearestPointOnGoal(u*8 + v*4), u*8 + v*1);
			TS_ASSERT_EQUALS(goal.DistanceToPoint(u*8 + v*4), i*3);
			TS_ASSERT_EQUALS(goal.NearestPointOnGoal(u*0 + v*0), u*0 + v*0);
			TS_ASSERT_EQUALS(goal.DistanceToPoint(u*0 + v*0), i*0);
			TS_ASSERT(!goal.RectContainsGoal(i*7, i*5, i*9, i*7)); // fully inside
			TS_ASSERT(goal.RectContainsGoal(i*3, i*1, i*13, i*11)); // fully outside
			TS_ASSERT(goal.RectContainsGoal(i*4, i*3, i*8, i*6)); // partially inside
			TS_ASSERT(goal.RectContainsGoal(i*4, i*0, i*12, i*1)); // touching the edge
		}

		{
			PathGoal goal = { PathGoal::SQUARE, i*8, i*6, i*4, i*3, u, v };
			TS_ASSERT_EQUALS(goal.NearestPointOnGoal(u*8 + v*4), u*8 + v*4);
			TS_ASSERT_EQUALS(goal.DistanceToPoint(u*8 + v*4), i*0);
			TS_ASSERT_EQUALS(goal.NearestPointOnGoal(u*0 + v*0), u*4 + v*3);
			TS_ASSERT_EQUALS(goal.DistanceToPoint(u*0 + v*0), i*5);
			TS_ASSERT(goal.RectContainsGoal(i*7, i*5, i*9, i*7)); // fully inside
			TS_ASSERT(goal.RectContainsGoal(i*3, i*1, i*13, i*11)); // fully outside
			TS_ASSERT(goal.RectContainsGoal(i*4, i*3, i*8, i*6)); // partially inside
			TS_ASSERT(goal.RectContainsGoal(i*4, i*2, i*12, i*3)); // touching the edge
			TS_ASSERT(goal.RectContainsGoal(i*3, i*0, i*4, i*10)); // touching the edge
		}

		{
			PathGoal goal = { PathGoal::INVERTED_SQUARE, i*8, i*6, i*4, i*3, u, v };
			TS_ASSERT_EQUALS(goal.NearestPointOnGoal(u*8 + v*4), u*8 + v*3);
			TS_ASSERT_EQUALS(goal.DistanceToPoint(u*8 + v*4), i*1);
			TS_ASSERT_EQUALS(goal.NearestPointOnGoal(u*0 + v*0), u*0 + v*0);
			TS_ASSERT_EQUALS(goal.DistanceToPoint(u*0 + v*0), i*0);
			TS_ASSERT(!goal.RectContainsGoal(i*7, i*5, i*9, i*7)); // fully inside
			TS_ASSERT(goal.RectContainsGoal(i*3, i*1, i*13, i*11)); // fully outside
			TS_ASSERT(!goal.RectContainsGoal(i*4, i*3, i*8, i*6)); // inside, touching (should fail)
			TS_ASSERT(goal.RectContainsGoal(i*4, i*2, i*12, i*3)); // touching the edge
			TS_ASSERT(goal.RectContainsGoal(i*3, i*0, i*4, i*10)); // touching the edge
		}
	}

	void test_performance_DISABLED()
	{
		CTerrain terrain;

		CSimulation2 sim2(NULL, g_ScriptRuntime, &terrain);
		sim2.LoadDefaultScripts();
		sim2.ResetState();

		std::unique_ptr<CMapReader> mapReader(new CMapReader());

		LDR_BeginRegistering();
		mapReader->LoadMap(L"maps/skirmishes/Median Oasis (2).pmp",
			sim2.GetScriptInterface().GetJSRuntime(), JS::UndefinedHandleValue,
			&terrain, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			&sim2, &sim2.GetSimContext(), -1, false);
		LDR_EndRegistering();
		TS_ASSERT_OK(LDR_NonprogressiveLoad());

		sim2.PreInitGame();
		sim2.InitGame();
		sim2.Update(0);

		CmpPtr<ICmpPathfinder> cmp(sim2, SYSTEM_ENTITY);

#if 0
		entity_pos_t x0 = entity_pos_t::FromInt(10);
		entity_pos_t z0 = entity_pos_t::FromInt(495);
		entity_pos_t x1 = entity_pos_t::FromInt(500);
		entity_pos_t z1 = entity_pos_t::FromInt(495);
		ICmpPathfinder::Goal goal = { ICmpPathfinder::Goal::POINT, x1, z1 };

		WaypointPath path;
		cmp->ComputePath(x0, z0, goal, cmp->GetPassabilityClass("default"), path);
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
			PathGoal goal = { PathGoal::POINT, x1, z1 };

			WaypointPath path;
			cmp->ComputePath(x0, z0, goal, cmp->GetPassabilityClass("default"), path);
		}

		t = timer_Time() - t;
		printf("[%f]", t);
	}

	void test_performance_short_DISABLED()
	{
		CTerrain terrain;
		terrain.Initialize(5, NULL);

		CSimulation2 sim2(NULL, g_ScriptRuntime, &terrain);
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

		PathGoal goal = { PathGoal::POINT, range, range };
		WaypointPath path = cmpPathfinder->ComputeShortPath(ShortPathRequest{ 0, range/3, range/3, fixed::FromInt(2), range, goal, 0, false, 0, 0 });
		for (size_t i = 0; i < path.m_Waypoints.size(); ++i)
			printf("# %d: %f %f\n", (int)i, path.m_Waypoints[i].x.ToFloat(), path.m_Waypoints[i].z.ToFloat());
	}

	template<typename T>
	void DumpGrid(std::ostream& stream, const Grid<T>& grid, int mask)
	{
		for (u16 j = 0; j < grid.m_H; ++j)
		{
			for (u16 i = 0; i < grid.m_W; )
			{
				if (!(grid.get(i, j) & mask))
				{
					i++;
					continue;
				}

				u16 i0 = i;
				for (i = i0+1; ; ++i)
				{
					if (i >= grid.m_W || !(grid.get(i, j) & mask))
					{
						stream << "  <rect x='" << i0 << "' y='" << j << "' width='" << (i-i0) << "' height='1'/>\n";
						break;
					}
				}
			}
		}
	}

	void test_perf2_DISABLED()
	{
		CTerrain terrain;

		CSimulation2 sim2(NULL, g_ScriptRuntime, &terrain);
		sim2.LoadDefaultScripts();
		sim2.ResetState();

		std::unique_ptr<CMapReader> mapReader(new CMapReader());

		LDR_BeginRegistering();
		mapReader->LoadMap(L"maps/scenarios/Peloponnese.pmp",
			sim2.GetScriptInterface().GetJSRuntime(), JS::UndefinedHandleValue,
			&terrain, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			&sim2, &sim2.GetSimContext(), -1, false);
		LDR_EndRegistering();
		TS_ASSERT_OK(LDR_NonprogressiveLoad());

		sim2.PreInitGame();
		sim2.InitGame();
		sim2.Update(0);

		std::ofstream stream(OsString("perf2.html").c_str(), std::ofstream::out | std::ofstream::trunc);

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(sim2, SYSTEM_ENTITY);
		CmpPtr<ICmpPathfinder> cmpPathfinder(sim2, SYSTEM_ENTITY);

		pass_class_t obstructionsMask = cmpPathfinder->GetPassabilityClass("default");
		const Grid<NavcellData>& obstructions = cmpPathfinder->GetPassabilityGrid();

		int scale = 1;
		stream << "<!DOCTYPE html>\n";
		stream << "<style>\n";
		stream << ".passability rect { fill: black; }\n";
		stream << ".astar-open rect { fill: rgba(255,255,0,0.75); }\n";
		stream << ".astar-closed rect { fill: rgba(255,0,0,0.75); }\n";
//		stream << ".astar-closed rect { fill: rgba(0,255,0,0.75); }\n";
		stream << ".path { stroke: rgba(0,0,255,0.75); stroke-width: 1; fill: none; }\n";
		stream << "</style>\n";
		stream << "<svg width='" << obstructions.m_W*scale << "' height='" << obstructions.m_H*scale << "'>\n";
		stream << "<g transform='translate(0 " << obstructions.m_H*scale << ") scale(" << scale << " -" << scale << ")'>\n";

		stream << " <g class='passability'>\n";
		DumpGrid(stream, obstructions, obstructionsMask);
		stream << " </g>\n";

		DumpPath(stream, 128*4, 256*4, 128*4, 384*4, cmpPathfinder);
// 	  	RepeatPath(500, 128*4, 256*4, 128*4, 384*4, cmpPathfinder);
//
// 		DumpPath(stream, 128*4, 204*4, 192*4, 204*4, cmpPathfinder);
//
// 		DumpPath(stream, 128*4, 230*4, 32*4, 230*4, cmpPathfinder);

		stream << "</g>\n";
		stream << "</svg>\n";
	}

	void test_perf3_DISABLED()
	{
		CTerrain terrain;

		CSimulation2 sim2(NULL, g_ScriptRuntime, &terrain);
		sim2.LoadDefaultScripts();
		sim2.ResetState();

		std::unique_ptr<CMapReader> mapReader(new CMapReader());

		LDR_BeginRegistering();
		mapReader->LoadMap(L"maps/scenarios/Peloponnese.pmp",
			sim2.GetScriptInterface().GetJSRuntime(), JS::UndefinedHandleValue,
			&terrain, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			&sim2, &sim2.GetSimContext(), -1, false);
		LDR_EndRegistering();
		TS_ASSERT_OK(LDR_NonprogressiveLoad());

		sim2.PreInitGame();
		sim2.InitGame();
		sim2.Update(0);

		std::ofstream stream(OsString("perf3.html").c_str(), std::ofstream::out | std::ofstream::trunc);

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(sim2, SYSTEM_ENTITY);
		CmpPtr<ICmpPathfinder> cmpPathfinder(sim2, SYSTEM_ENTITY);

		pass_class_t obstructionsMask = cmpPathfinder->GetPassabilityClass("default");
		const Grid<NavcellData>& obstructions = cmpPathfinder->GetPassabilityGrid();

		int scale = 31;
		stream << "<!DOCTYPE html>\n";
		stream << "<style>\n";
		stream << ".passability rect { fill: black; }\n";
		stream << ".astar-open rect { fill: rgba(255,255,0,0.75); }\n";
		stream << ".astar-closed rect { fill: rgba(255,0,0,0.75); }\n";
		stream << ".path { stroke: rgba(0,0,255,0.75); stroke-width: " << (1.0f / scale) << "; fill: none; }\n";
		stream << "</style>\n";
		stream << "<svg width='" << obstructions.m_W*scale << "' height='" << obstructions.m_H*scale << "'>\n";
		stream << "<defs><pattern id='grid' width='1' height='1' patternUnits='userSpaceOnUse'>\n";
		stream << "<rect fill='none' stroke='#888' stroke-width='" << (1.0f / scale) << "' width='" << scale << "' height='" << scale << "'/>\n";
		stream << "</pattern></defs>\n";
		stream << "<g transform='translate(0 " << obstructions.m_H*scale << ") scale(" << scale << " -" << scale << ")'>\n";

		stream << " <g class='passability'>\n";
		DumpGrid(stream, obstructions, obstructionsMask);
		stream << " </g>\n";

		for (int j = 160; j < 190; ++j)
			for (int i = 220; i < 290; ++i)
				DumpPath(stream, 230, 178, i, j, cmpPathfinder);

		stream << "<rect fill='url(#grid)' width='100%' height='100%'/>\n";
		stream << "</g>\n";
		stream << "</svg>\n";
	}

	void DumpPath(std::ostream& stream, int i0, int j0, int i1, int j1, CmpPtr<ICmpPathfinder>& cmpPathfinder)
	{
		entity_pos_t x0 = entity_pos_t::FromInt(i0);
		entity_pos_t z0 = entity_pos_t::FromInt(j0);
		entity_pos_t x1 = entity_pos_t::FromInt(i1);
		entity_pos_t z1 = entity_pos_t::FromInt(j1);

		PathGoal goal = { PathGoal::POINT, x1, z1 };

		WaypointPath path;
		cmpPathfinder->ComputePath(x0, z0, goal, cmpPathfinder->GetPassabilityClass("default"), path);

		u32 debugSteps;
		double debugTime;
		Grid<u8> debugGrid;
		cmpPathfinder->GetDebugData(debugSteps, debugTime, debugGrid);
// 		stream << " <g style='visibility:hidden'>\n";

		stream << " <g>\n";
// 		stream << " <g class='astar-open'>\n";
// 		DumpGrid(stream, debugGrid, 1);
// 		stream << " </g>\n";
// 		stream << " <g class='astar-closed'>\n";
// 		DumpGrid(stream, debugGrid, 2);
// 		stream << " </g>\n";
// 		stream << " <g class='astar-closed'>\n";
// 		DumpGrid(stream, debugGrid, 3);
// 		stream << " </g>\n";
		stream << " </g>\n";

		stream << " <polyline";
		stream << " onmouseover='this.previousElementSibling.style.visibility=\"visible\"' onmouseout='this.previousElementSibling.style.visibility=\"hidden\"'";
		double length = 0;
		for (ssize_t i = 0; i < (ssize_t)path.m_Waypoints.size()-1; ++i)
		{
			double dx = (path.m_Waypoints[i+1].x - path.m_Waypoints[i].x).ToDouble();
			double dz = (path.m_Waypoints[i+1].z - path.m_Waypoints[i].z).ToDouble();
			length += sqrt(dx*dx + dz*dz);
		}
		stream << " title='length: " << length << "; tiles explored: " << debugSteps << "; time: " << debugTime*1000 << " msec'";
		stream << " class='path' points='";
		for (size_t i = 0; i < path.m_Waypoints.size(); ++i)
			stream << path.m_Waypoints[i].x.ToDouble()*Pathfinding::NAVCELLS_PER_TILE/TERRAIN_TILE_SIZE << "," << path.m_Waypoints[i].z.ToDouble()*Pathfinding::NAVCELLS_PER_TILE/TERRAIN_TILE_SIZE << " ";
		stream << "'/>\n";
	}

	void RepeatPath(int n, int i0, int j0, int i1, int j1, CmpPtr<ICmpPathfinder>& cmpPathfinder)
	{
		entity_pos_t x0 = entity_pos_t::FromInt(i0);
		entity_pos_t z0 = entity_pos_t::FromInt(j0);
		entity_pos_t x1 = entity_pos_t::FromInt(i1);
		entity_pos_t z1 = entity_pos_t::FromInt(j1);

		PathGoal goal = { PathGoal::POINT, x1, z1 };

		double t = timer_Time();
		for (int i = 0; i < n; ++i)
		{
			WaypointPath path;
			cmpPathfinder->ComputePath(x0, z0, goal, cmpPathfinder->GetPassabilityClass("default"), path);
		}
		t = timer_Time() - t;
		debug_printf("### RepeatPath %fms each (%fs total)\n", 1000*t / n, t);
	}

};
