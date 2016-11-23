/* Copyright (C) 2012 Wildfire Games.
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

#include "ps/CStr.h"
#include "graphics/Terrain.h"
#include "graphics/TerritoryBoundary.h"
#include "simulation2/helpers/Grid.h"

class TestCmpTerritoryManager : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		CxxTest::setAbortTestOnFail(true);
	}

	void tearDown()
	{

	}

	void test_boundaries()
	{
		Grid<u8> grid = GetGrid("--------"
		                        "777777--"
								"777777--"
								"777777--"
								"--------", 8, 5);

		std::vector<STerritoryBoundary> boundaries = CTerritoryBoundaryCalculator::ComputeBoundaries(&grid);
		TS_ASSERT_EQUALS(1U, boundaries.size());
		TS_ASSERT_EQUALS(18U, boundaries[0].points.size()); // 2x6 + 2x3
		TS_ASSERT_EQUALS((player_id_t)7, boundaries[0].owner);
		TS_ASSERT_EQUALS(false, boundaries[0].blinking); // high bits aren't set by GetGrid

		// assumes CELL_SIZE is 2; dealt with in TestBoundaryPointsEqual
		int expectedPoints[][2] = {{ 4, 8}, {12, 8}, {20, 8}, {28, 8}, {36, 8}, {44, 8},
		                           {48,12}, {48,20}, {48,28},
		                           {44,32}, {36,32}, {28,32}, {20,32}, {12,32}, { 4,32},
		                           { 0,28}, { 0,20}, { 0,12}};

		TestBoundaryPointsEqual(boundaries[0].points, expectedPoints);
	}

	void test_nested_boundaries1()
	{
		// test case from ticket #918; contains single-tile territories with double borders
		Grid<u8> grid1 = GetGrid("--------"
		                         "-111111-"
								 "-1-1213-"
								 "-111111-"
								 "--------", 8, 5);

		std::vector<STerritoryBoundary> boundaries = CTerritoryBoundaryCalculator::ComputeBoundaries(&grid1);

		size_t expectedNumBoundaries = 5;
		TS_ASSERT_EQUALS(expectedNumBoundaries, boundaries.size());

		STerritoryBoundary* onesOuter = NULL;
		STerritoryBoundary* onesInner0 = NULL; // inner border around the neutral tile
		STerritoryBoundary* onesInner2 = NULL; // inner border around the '2' tile
		STerritoryBoundary* twosOuter = NULL;
		STerritoryBoundary* threesOuter = NULL;

		// expected number of points (!) in the inner boundaries for terrain 1 (there are two with the same size)
		size_t onesInnerNumExpectedPoints = 4;

		for (size_t i=0; i<expectedNumBoundaries; i++)
		{
			STerritoryBoundary& boundary = boundaries[i];
			switch (boundary.owner)
			{
			case 1:
				// to figure out which 1-boundary is which, we can use the number of points to distinguish between outer and inner,
				// and within the inners we can split them by their X value (onesInner0 is the leftmost one, onesInner1 the
				// rightmost one).
				if (boundary.points.size() != onesInnerNumExpectedPoints)
				{
					TSM_ASSERT_EQUALS("Found multiple outer boundaries for territory owned by player 1", onesOuter, (STerritoryBoundary*) NULL);
					onesOuter = &boundary;
				}
				else
				{
					TS_ASSERT_EQUALS(onesInnerNumExpectedPoints, boundary.points.size()); // all inner boundaries are of size 2
					if (boundary.points[0].X < 24.f)
					{
						// leftmost inner boundary, i.e. onesInner0
						TSM_ASSERT_EQUALS("Found multiple leftmost inner boundaries for territory owned by player 1", onesInner0, (STerritoryBoundary*) NULL);
						onesInner0 = &boundary;
					}
					else
					{
						TSM_ASSERT_EQUALS("Found multiple rightmost inner boundaries for territory owned by player 1", onesInner2, (STerritoryBoundary*) NULL);
						onesInner2 = &boundary;
					}
				}
				break;
			case 2:
				TSM_ASSERT_EQUALS("Too many boundaries for territory owned by player 2", twosOuter, (STerritoryBoundary*) NULL);
				twosOuter = &boundary;
				break;

			case 3:
				TSM_ASSERT_EQUALS("Too many boundaries for territory owned by player 3", threesOuter, (STerritoryBoundary*) NULL);
				threesOuter = &boundary;
				break;

			default:
				TS_FAIL("Unexpected tile owner");
				break;
			}
		}

		TS_ASSERT_DIFFERS(onesOuter,   (STerritoryBoundary*) NULL);
		TS_ASSERT_DIFFERS(onesInner0,  (STerritoryBoundary*) NULL);
		TS_ASSERT_DIFFERS(onesInner2,  (STerritoryBoundary*) NULL);
		TS_ASSERT_DIFFERS(twosOuter,   (STerritoryBoundary*) NULL);
		TS_ASSERT_DIFFERS(threesOuter, (STerritoryBoundary*) NULL);

		TS_ASSERT_EQUALS(onesOuter->points.size(), 20U);
		TS_ASSERT_EQUALS(onesInner0->points.size(), 4U);
		TS_ASSERT_EQUALS(onesInner2->points.size(), 4U);
		TS_ASSERT_EQUALS(twosOuter->points.size(), 4U);
		TS_ASSERT_EQUALS(threesOuter->points.size(), 4U);

		int onesOuterExpectedPoints[][2] = {{12, 8}, {20, 8}, {28, 8}, {36, 8}, {44, 8}, {52, 8},
		                                    {56,12}, {52,16}, {48,20}, {52,24}, {56,28},
		                                    {52,32}, {44,32}, {36,32}, {28,32}, {20,32}, {12,32},
		                                    { 8,28}, { 8,20}, { 8,12}};
		int onesInner0ExpectedPoints[][2] = {{20,24}, {24,20}, {20,16}, {16,20}};
		int onesInner2ExpectedPoints[][2] = {{36,24}, {40,20}, {36,16}, {32,20}};
		int twosOuterExpectedPoints[][2]  = {{36,16}, {40,20}, {36,24}, {32,20}};
		int threesOuterExpectedPoints[][2] = {{52,16}, {56,20}, {52,24}, {48,20}};

		TestBoundaryPointsEqual(onesOuter->points, onesOuterExpectedPoints);
		TestBoundaryPointsEqual(onesInner0->points, onesInner0ExpectedPoints);
		TestBoundaryPointsEqual(onesInner2->points, onesInner2ExpectedPoints);
		TestBoundaryPointsEqual(twosOuter->points, twosOuterExpectedPoints);
		TestBoundaryPointsEqual(threesOuter->points, threesOuterExpectedPoints);
	}

	void test_nested_boundaries2()
	{
		Grid<u8> grid1 = GetGrid("-22222-"
								 "-2---2-"
								 "-2-1123"
								 "-2-1123"
								 "-2-2223"
								 "-222333", 7, 6);

		std::vector<STerritoryBoundary> boundaries = CTerritoryBoundaryCalculator::ComputeBoundaries(&grid1);

		// There should be two boundaries found for the territory of 2's (one outer and one inner edge), plus two regular
		// outer edges of the territories of 1's and 3's. The order in which they're returned doesn't matter though, so
		// we should first detect which one is which.
		size_t expectedNumBoundaries = 4;
		TS_ASSERT_EQUALS(expectedNumBoundaries, boundaries.size());

		STerritoryBoundary* onesOuter = NULL;
		STerritoryBoundary* twosOuter = NULL;
		STerritoryBoundary* twosInner = NULL;
		STerritoryBoundary* threesOuter = NULL;

		for (size_t i=0; i < expectedNumBoundaries; i++)
		{
			STerritoryBoundary& boundary = boundaries[i];
			switch (boundary.owner)
			{
				case 1:
					TSM_ASSERT_EQUALS("Too many boundaries for territory owned by player 1", onesOuter, (STerritoryBoundary*) NULL);
					onesOuter = &boundary;
					break;

				case 3:
					TSM_ASSERT_EQUALS("Too many boundaries for territory owned by player 3", threesOuter, (STerritoryBoundary*) NULL);
					threesOuter = &boundary;
					break;

				case 2:
					// assign twosOuter first, then twosInner last; we'll swap them afterwards if needed
					if (twosOuter == NULL)
						twosOuter = &boundary;
					else if (twosInner == NULL)
						twosInner = &boundary;
					else
						TS_FAIL("Too many boundaries for territory owned by player 2");

					break;

				default:
					TS_FAIL("Unexpected tile owner");
					break;
			}
		}

		TS_ASSERT_DIFFERS(onesOuter,   (STerritoryBoundary*) NULL);
		TS_ASSERT_DIFFERS(twosOuter,   (STerritoryBoundary*) NULL);
		TS_ASSERT_DIFFERS(twosInner,   (STerritoryBoundary*) NULL);
		TS_ASSERT_DIFFERS(threesOuter, (STerritoryBoundary*) NULL);

		TS_ASSERT_EQUALS(onesOuter->points.size(), 8U);
		TS_ASSERT_EQUALS(twosOuter->points.size(), 22U);
		TS_ASSERT_EQUALS(twosInner->points.size(), 14U);
		TS_ASSERT_EQUALS(threesOuter->points.size(), 14U);

		// See if we need to swap the outer and inner edges of the twos territories (uses the extremely simplistic
		// heuristic of comparing the amount of points to determine which one is the outer one and which one the inner
		// one (which does happen to work in this case though).

		if (twosOuter->points.size() < twosInner->points.size())
		{
			STerritoryBoundary* tmp = twosOuter;
			twosOuter = twosInner;
			twosInner = tmp;
		}

		int onesOuterExpectedPoints[][2] = {{28,16}, {36,16}, {40,20}, {40,28}, {36,32}, {28,32}, {24,28}, {24,20}};
		int twosOuterExpectedPoints[][2] = {{12, 0}, {20, 0}, {28, 0}, {32, 4}, {36, 8}, {44, 8},
		                                    {48,12}, {48,20}, {48,28}, {48,36}, {48,44},
											{44,48}, {36,48}, {28,48}, {20,48}, {12,48},
											{ 8,44}, { 8,36}, { 8,28}, { 8,20}, { 8,12}, { 8, 4}};
		int twosInnerExpectedPoints[][2] = {{20,40}, {28,40}, {36,40}, {40,36}, {40,28}, {40,20}, {36,16},
		                                    {28,16}, {24,12}, {20, 8}, {16,12}, {16,20}, {16,28}, {16,36}};
		int threesOuterExpectedPoints[][2] = {{36, 0}, {44, 0}, {52, 0}, {56, 4}, {56,12}, {56,20}, {56,28}, {52,32},
		                                      {48,28}, {48,20}, {48,12}, {44, 8}, {36, 8}, {32, 4}};

		TestBoundaryPointsEqual(onesOuter->points, onesOuterExpectedPoints);
		TestBoundaryPointsEqual(twosOuter->points, twosOuterExpectedPoints);
		TestBoundaryPointsEqual(twosInner->points, twosInnerExpectedPoints);
		TestBoundaryPointsEqual(threesOuter->points, threesOuterExpectedPoints);
	}

private:
	/// Parses a string representation of a grid into an actual Grid structure, such that the (i,j) axes are located in the bottom
	/// left hand side of the map. Note: leaves all custom bits in the grid values at zero (anything outside
	/// ICmpTerritoryManager::TERRITORY_PLAYER_MASK).
	Grid<u8> GetGrid(std::string def, u16 w, u16 h)
	{
		Grid<u8> grid(w, h);
		const char* chars = def.c_str();

		for (u16 y=0; y<h; y++)
		{
			for (u16 x=0; x<w; x++)
			{
				char gridDefChar = chars[x+y*w];
				if (gridDefChar == '-')
					continue;

				ENSURE('0' <= gridDefChar && gridDefChar <= '9');
				u8 playerId = gridDefChar - '0';
				grid.set(x, h-1-y, playerId);
			}
		}

		return grid;
	}

	void TestBoundaryPointsEqual(std::vector<CVector2D> points, int expectedPoints[][2])
	{
		// TODO: currently relies on an exact point match, i.e. expectedPoints must be specified going CCW or CW (depending on
		// whether we're testing an inner or an outer edge) starting from the exact same point that the algorithm happened to
		// decide to start the run from. This is an algorithmic detail and is not considered to be part of the specification
		// of the return value. Hence, this method should also accept 'expectedPoints' to be a cyclically shifted
		// version of 'points', so that the starting position doesn't need to match exactly.
		for (size_t i = 0; i < points.size(); i++)
		{
			// the input numbers in expectedPoints are defined under the assumption that CELL_SIZE is 2, so let's include
			// a scaling factor to protect against that should CELL_SIZE ever change
			TS_ASSERT_DELTA(points[i].X, float(expectedPoints[i][0]) * 4.f / TERRAIN_TILE_SIZE, 1e-7);
			TS_ASSERT_DELTA(points[i].Y, float(expectedPoints[i][1]) * 4.f / TERRAIN_TILE_SIZE, 1e-7);
		}
	}
};
