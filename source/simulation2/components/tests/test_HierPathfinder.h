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

#define TEST

#include "maths/Vector2D.h"
#include "simulation2/helpers/HierarchicalPathfinder.h"

class TestHierarchicalPathfinder : public CxxTest::TestSuite
{
public:
	void setUp()
	{
	}

	void tearDown()
	{
	}

	const pass_class_t PASS_1 = 1;
	const pass_class_t PASS_2 = 2;
	const pass_class_t NON_PASS_1 = 4;

	const u16 mapSize = 240;

	std::map<std::string, pass_class_t> pathClassMask;
	std::map<std::string, pass_class_t> nonPathClassMask;

	void debug_grid(Grid<NavcellData>& grid) {
		for (size_t i = 0; i < grid.m_W; ++i)
		{
			for (size_t j = 0; j < grid.m_H; ++j)
				printf("%i", grid.get(i,j));
			printf("\n");
		}
	}

	void debug_grid_points(Grid<NavcellData>& grid, u16 i1, u16 j1, u16 i2, u16 j2) {
		for (size_t i = 0; i < grid.m_W; ++i)
		{
			for (size_t j = 0; j < grid.m_H; ++j)
			{
				if (i == i1 && j == j1)
					printf("A");
				else if (i == i2 && j == j2)
					printf("B");
				else
					printf("%i", grid.get(i,j));
			}
			printf("\n");
		}
	}

	void assert_blank(HierarchicalPathfinder& hierPath)
	{
		u16 i = 89;
		u16 j = 34;
		hierPath.FindNearestPassableNavcell(i, j, PASS_1);
		TS_ASSERT(i == 89 && j == 34);

		for (auto& chunk : hierPath.m_Chunks[PASS_1])
			TS_ASSERT(chunk.m_RegionsID.size() == 1);

		// number of connected regions: 4 in the middle, 2 in the corners.
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(120, 120, PASS_1)].size() == 4);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(20,  20,  PASS_1)].size() == 2);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(220, 220, PASS_1)].size() == 2);

		std::set<HierarchicalPathfinder::RegionID> reachables;
		hierPath.FindReachableRegions(hierPath.Get(120, 120, PASS_1), reachables, PASS_1);
		TS_ASSERT(reachables.size() == 9);
		reachables.clear();
		hierPath.FindReachableRegions(hierPath.Get(20, 20, PASS_1), reachables, PASS_1);
		TS_ASSERT(reachables.size() == 9);
	}

	void test_reachability_and_update()
	{
		pathClassMask = {
			{ "1", 1 },
			{ "2", 2 },
		};
		nonPathClassMask = {
			{ "3", 4 }
		};

		HierarchicalPathfinder hierPath;
		Grid<NavcellData> grid(mapSize, mapSize);
		Grid<u8> dirtyGrid(mapSize, mapSize);

		// Entirely passable for PASS_1, not for others;
		for (size_t i = 0; i < mapSize; ++i)
			for (size_t j = 0; j < mapSize; ++j)
				grid.set(i, j, 6);

		hierPath.Recompute(&grid, nonPathClassMask, pathClassMask);

		assert_blank(hierPath);

		//////////////////////////////////////////////////////
		// Split the map in two in the middle.
		for (u16 j = 0; j < mapSize; ++j)
		{
			grid.set(125, j, 7);
			dirtyGrid.set(125, j, 1);
		}

		hierPath.Update(&grid, dirtyGrid);

		for (size_t j = 0; j < mapSize; ++j)
			TS_ASSERT(hierPath.Get(125, j, PASS_1).r == 0);

		// number of connected regions: 3 in the middle (both sides), 2 in the corners.
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(120, 120, PASS_1)].size() == 3);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(170, 120, PASS_1)].size() == 3);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(20,  20,  PASS_1)].size() == 2);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(220, 220, PASS_1)].size() == 2);

		std::set<HierarchicalPathfinder::RegionID> reachables;
		hierPath.FindReachableRegions(hierPath.Get(120, 120, PASS_1), reachables, PASS_1);
		TS_ASSERT(reachables.size() == 6);
		reachables.clear();
		hierPath.FindReachableRegions(hierPath.Get(170, 120, PASS_1), reachables, PASS_1);
		TS_ASSERT(reachables.size() == 6);

		//////////////////////////////////////////////////////
		// Un-split the map in two in the middle.
		for (u16 j = 0; j < mapSize; ++j)
		{
			grid.set(125, j, 6);
			dirtyGrid.set(125, j, 1);
		}
		hierPath.Update(&grid, dirtyGrid);
		assert_blank(hierPath);

		//////////////////////////////////////////////////////
		// Partial split in the middle chunk - no actual connectivity change
		for (u16 j = 120; j < 150; ++j)
		{
			grid.set(125, j, 7);
			dirtyGrid.set(125, j, 1);
		}
		hierPath.Update(&grid, dirtyGrid);
		reachables.clear();
		hierPath.FindReachableRegions(hierPath.Get(170, 120, PASS_1), reachables, PASS_1);
		TS_ASSERT(reachables.size() == 9);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(170, 120, PASS_1)].size() == 4);

		//////////////////////////////////////////////////////
		// Block a strip along the edge, but regions are still connected.
		for (u16 j = 70; j < 200; ++j)
		{
			grid.set(96, j, 7);
			dirtyGrid.set(96, j, 1);
		}
		hierPath.Update(&grid, dirtyGrid);
		reachables.clear();
		hierPath.FindReachableRegions(hierPath.Get(170, 120, PASS_1), reachables, PASS_1);
		TS_ASSERT(reachables.size() == 9);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(20, 120, PASS_1)].size() == 2);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(170, 120, PASS_1)].size() == 3);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(200, 120, PASS_1)].size() == 3);

		//////////////////////////////////////////////////////
		// Block the other edge
		for (u16 j = 70; j < 200; ++j)
		{
			grid.set(192, j, 7);
			dirtyGrid.set(192, j, 1);
		}
		hierPath.Update(&grid, dirtyGrid);
		reachables.clear();
		hierPath.FindReachableRegions(hierPath.Get(170, 120, PASS_1), reachables, PASS_1);
		TS_ASSERT(reachables.size() == 9);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(20, 120, PASS_1)].size() == 2);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(170, 120, PASS_1)].size() == 2);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(200, 120, PASS_1)].size() == 2);

		//////////////////////////////////////////////////////
		// Create an isolated region in the middle chunk
		for (u16 i = 96; i < 140; ++i)
		{
			grid.set(i, 110, 7);
			dirtyGrid.set(i, 110, 1);
		}
		for (u16 i = 96; i < 140; ++i)
		{
			grid.set(i, 140, 7);
			dirtyGrid.set(i, 140, 1);
		}
		for (u16 j = 110; j < 141; ++j)
		{
			grid.set(140, j, 7);
			dirtyGrid.set(140, j, 1);
		}
		hierPath.Update(&grid, dirtyGrid);

		reachables.clear();
		hierPath.FindReachableRegions(hierPath.Get(170, 120, PASS_1), reachables, PASS_1);
		TS_ASSERT(reachables.size() == 9);
		reachables.clear();
		hierPath.FindReachableRegions(hierPath.Get(120, 120, PASS_1), reachables, PASS_1);
		TS_ASSERT(reachables.size() == 1);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(120, 120, PASS_1)].size() == 0);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(20, 120, PASS_1)].size() == 2);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(170, 120, PASS_1)].size() == 2);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(200, 120, PASS_1)].size() == 2);

		//////////////////////////////////////////////////////
		// Open it
		for (u16 j = 110; j < 141; ++j)
		{
			grid.set(140, j, 6);
			dirtyGrid.set(140, j, 1);
		}
		hierPath.Update(&grid, dirtyGrid);

		reachables.clear();
		hierPath.FindReachableRegions(hierPath.Get(170, 120, PASS_1), reachables, PASS_1);
		TS_ASSERT(reachables.size() == 9);
		reachables.clear();
		hierPath.FindReachableRegions(hierPath.Get(120, 120, PASS_1), reachables, PASS_1);
		TS_ASSERT(reachables.size() == 9);
		TS_ASSERT(hierPath.m_Edges[PASS_1][hierPath.Get(120, 120, PASS_1)].size() == 2);
	}

	u16 manhattan(u16 i, u16 j, u16 gi, u16 gj) {
		return abs(i - gi) + abs(j - gj);
	}

	double euclidian(u16 i, u16 j, u16 gi, u16 gj) {
		return sqrt((i - gi)*(i - gi) + (j - gj)*(j - gj));
	}
	void test_passability()
	{
		pathClassMask = {
			{ "1", 1 },
			{ "2", 2 },
		};
		nonPathClassMask = {
			{ "3", 4 }
		};

		// 0 is passable, 1 is not.
		// i is vertical, j is horizontal;
#define _ 0
#define X 1
		NavcellData gridDef[40][40] = {
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,X,X,X,X,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,X,_,_,X,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,X,X,X,_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,X,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X,X,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
			{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_}
		};
#undef _
#undef X
		// upscaled 5 times
		HierarchicalPathfinder hierPath;
		Grid<NavcellData> grid(40*5, 40*5);
		Grid<u8> dirtyGrid(40*5, 40*5);

		for (size_t i = 0; i < 40; ++i)
			for (size_t j = 0; j < 40; ++j)
				for (size_t ii = 0; ii < 5; ++ii)
					for (size_t jj = 0; jj < 5; ++jj)
						grid.set(i * 5 + ii, j * 5 + jj, gridDef[i][j]);

		hierPath.Recompute(&grid, nonPathClassMask, pathClassMask);

		u16 i = 5, j = 5;
		hierPath.FindNearestPassableNavcell(i, j, PASS_1);
		TS_ASSERT(i == 5 && j == 5);

		// use a macro so the lines reported by tests are accurate
#define check_closest_passable(i, j, expected_manhattan) \
	oi = i; oj = j; \
	pi = i; pj = j; \
	TS_ASSERT(!IS_PASSABLE(grid.get(pi, pj), PASS_1)); \
	hierPath.FindNearestPassableNavcell(pi, pj, PASS_1); \
\
	if (expected_manhattan == -1) \
	{ \
		TS_ASSERT(oi == pi && oj == pj); \
	} else { \
		TS_ASSERT(IS_PASSABLE(grid.get(pi, pj), PASS_1)); \
		TS_ASSERT(manhattan(pi, pj, oi, oj) == expected_manhattan); \
	}
		u16 oi, oj, pi, pj;

		check_closest_passable(4 * 5, 4 * 5, 1);
		check_closest_passable(4 * 5 + 1, 4 * 5 + 1, 2);
		check_closest_passable(14 * 5 + 2, 7 * 5 + 2, 8);
		check_closest_passable(14 * 5 + 2, 7 * 5 + 4, 6);
		check_closest_passable(14 * 5 + 2, 7 * 5 + 5, 5);
		check_closest_passable(14 * 5 + 2, 7 * 5 + 6, 4);
		check_closest_passable(5 * 5 + 3, 7 * 5 + 2, 2);
#undef check_closest_passable

		PathGoal goal;
		goal.type = PathGoal::POINT;

		// from the left of the C, goal is unreachable, expect closest navcell to goal
		oi = 5 * 5 + 3; oj = 3 * 5 + 3;
		pi = 5 * 5 + 3; pj = 7 * 5 + 2; goal.x = fixed::FromInt(pi); goal.z = fixed::FromInt(pj);

		hierPath.MakeGoalReachable(oi, oj, goal, PASS_1);
		hierPath.FindNearestPassableNavcell(pi, pj, PASS_1);
		TS_ASSERT(pi == goal.x.ToInt_RoundToNegInfinity() && pj == goal.z.ToInt_RoundToNegInfinity());

		// random reachable point.
		oi = 5 * 5 + 3; oj = 3 * 5 + 3;
		pi = 26 * 5 + 3; pj = 5 * 5 + 2; goal.x = fixed::FromInt(pi) + fixed::FromInt(1)/3; goal.z = fixed::FromInt(pj) + fixed::FromInt(1)/3;
		hierPath.MakeGoalReachable(oi, oj, goal, PASS_1);
		TS_ASSERT(pi == goal.x.ToInt_RoundToNegInfinity() && pj == goal.z.ToInt_RoundToNegInfinity());

		// top-left corner
		goal.x = fixed::FromInt(pi); goal.z = fixed::FromInt(pj);
		hierPath.MakeGoalReachable(oi, oj, goal, PASS_1);
		TS_ASSERT(pi == goal.x.ToInt_RoundToNegInfinity() && pj == goal.z.ToInt_RoundToNegInfinity());

		// near bottom-right corner
		goal.x = fixed::FromInt(pi) + fixed::FromInt(3)/4; goal.z = fixed::FromInt(pj) + fixed::FromInt(3)/4;
		hierPath.MakeGoalReachable(oi, oj, goal, PASS_1);
		TS_ASSERT(pi == goal.x.ToInt_RoundToNegInfinity() && pj == goal.z.ToInt_RoundToNegInfinity());

		// Circle
		goal.type = PathGoal::CIRCLE;
		goal.hw = fixed::FromInt(1) / 2;

		// from the left of the C, goal is unreachable, expect closest navcell to goal
		oi = 5 * 5 + 3; oj = 3 * 5 + 3;
		pi = 5 * 5 + 3; pj = 7 * 5 + 2; goal.x = fixed::FromInt(pi); goal.z = fixed::FromInt(pj);
		hierPath.MakeGoalReachable(oi, oj, goal, PASS_1);
		hierPath.FindNearestPassableNavcell(pi, pj, PASS_1);
		TS_ASSERT(pi == goal.x.ToInt_RoundToNegInfinity() && pj == goal.z.ToInt_RoundToNegInfinity());

		// same position, goal is reachable, expect closest navcell to goal
		goal.hw = fixed::FromInt(3);
		goal.x = fixed::FromInt(pi); goal.z = fixed::FromInt(pj);
		hierPath.MakeGoalReachable(oi, oj, goal, PASS_1);
		hierPath.FindNearestPassableNavcell(pi, pj, PASS_1);
		TS_ASSERT(pi == goal.x.ToInt_RoundToNegInfinity() && pj == goal.z.ToInt_RoundToNegInfinity());

		// Square
		goal.type = PathGoal::SQUARE;
		goal.hw = fixed::FromInt(1) / 2;
		goal.hh = fixed::FromInt(1) / 2;

		// from the left of the C, goal is unreachable, expect closest navcell to goal
		oi = 5 * 5 + 3; oj = 3 * 5 + 3;
		pi = 5 * 5 + 3; pj = 7 * 5 + 2; goal.x = fixed::FromInt(pi); goal.z = fixed::FromInt(pj);
		hierPath.MakeGoalReachable(oi, oj, goal, PASS_1);
		hierPath.FindNearestPassableNavcell(pi, pj, PASS_1);
		TS_ASSERT(pi == goal.x.ToInt_RoundToNegInfinity() && pj == goal.z.ToInt_RoundToNegInfinity());

		// same position, goal is reachable, expect closest navcell to goal
		goal.hw = fixed::FromInt(3);
		goal.hh = fixed::FromInt(3);
		goal.x = fixed::FromInt(pi); goal.z = fixed::FromInt(pj);
		hierPath.MakeGoalReachable(oi, oj, goal, PASS_1);
		hierPath.FindNearestPassableNavcell(pi, pj, PASS_1);
		TS_ASSERT(pi == goal.x.ToInt_RoundToNegInfinity() && pj == goal.z.ToInt_RoundToNegInfinity());

		// Goal is reachable diagonally (1 cell away)
		goal.hw = fixed::FromInt(1);
		goal.hh = fixed::FromInt(1);
		oi = 5 * 5 + 3; oj = 3 * 5 + 3;
		pi = 5 * 5 - 1; pj = 7 * 5 + 3; goal.x = fixed::FromInt(pi); goal.z = fixed::FromInt(pj);
		hierPath.MakeGoalReachable(oi, oj, goal, PASS_1);
		hierPath.FindNearestPassableNavcell(pi, pj, PASS_1);
		TS_ASSERT(pi == goal.x.ToInt_RoundToNegInfinity() && pj == goal.z.ToInt_RoundToNegInfinity());

		// Huge Circle goal, expect point closest to us.
		goal.type = PathGoal::CIRCLE;
		goal.hw = fixed::FromInt(20);

		oi = 5 * 5 + 3; oj = 3 * 5 + 3;
		pi = 36 * 5 + 3; pj = 7 * 5 + 2; goal.x = fixed::FromInt(pi); goal.z = fixed::FromInt(pj);

		hierPath.MakeGoalReachable(oi, oj, goal, PASS_1);
		// bit of leeway for cell placement
		TS_ASSERT(abs(euclidian(goal.x.ToInt_RoundToNegInfinity(), goal.z.ToInt_RoundToNegInfinity(), pi, pj)-20) < 1.5f);
		TS_ASSERT(abs(euclidian(goal.x.ToInt_RoundToNegInfinity(), goal.z.ToInt_RoundToNegInfinity(), oi, oj) - euclidian(pi, pj, oi, oj)) < 22.0f);
	}

	void test_regions_flood_fill()
	{
		// Partial test of region inner flood filling.
		// This highlights that internal region IDs can become higher than the number of regions.
		pathClassMask = {
			{ "1", 1 },
			{ "2", 2 },
		};
		nonPathClassMask = {
			{ "3", 4 }
		};

		// 0 is passable, 1 is not.
		// i is vertical, j is horizontal;
#define _ 0
#define X 1
		NavcellData gridDef[5][5] = {
			{X,_,X,_,_},
			{_,_,X,X,_},
			{X,_,X,_,_},
			{_,_,X,X,_},
			{X,_,X,_,_}
		};
#undef _
#undef X
		HierarchicalPathfinder hierPath;
		Grid<NavcellData> grid(5, 5);
		Grid<u8> dirtyGrid(5, 5);
		for (size_t i = 0; i < 5; ++i)
			for (size_t j = 0; j < 5; ++j)
					grid.set(i, j, gridDef[i][j]);
		hierPath.Recompute(&grid, nonPathClassMask, pathClassMask);

		TS_ASSERT_EQUALS(hierPath.m_Chunks[pathClassMask["1"]][0].m_RegionsID.size(), 2);
		TS_ASSERT_EQUALS(hierPath.m_Chunks[pathClassMask["1"]][0].m_RegionsID.back(), 4);
	}
};
