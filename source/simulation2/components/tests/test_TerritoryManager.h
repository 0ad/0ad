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

#include "maths/Matrix3D.h"
#include "ps/CStr.h"
#include "graphics/Terrain.h"
#include "graphics/TerritoryBoundary.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/components/ICmpTerritoryManager.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpTerritoryInfluence.h"
#include "simulation2/components/ICmpOwnership.h"

class MockPathfinderTerrMan : public ICmpPathfinder
{
public:
	DEFAULT_MOCK_COMPONENT()

	// Test data
	Grid<NavcellData> m_PassabilityGrid;

	virtual pass_class_t GetPassabilityClass(const std::string&) const override { return 0; }
	virtual const Grid<NavcellData>& GetPassabilityGrid() override { return m_PassabilityGrid; }

	// Irrelevant part of the mock.
	virtual void GetPassabilityClasses(std::map<std::string, pass_class_t>&) const override {}
	virtual void GetPassabilityClasses(std::map<std::string, pass_class_t>&, std::map<std::string, pass_class_t>&) const override {}
	virtual entity_pos_t GetClearance(pass_class_t) const override { return entity_pos_t::FromInt(1); }
	virtual entity_pos_t GetMaximumClearance() const override { return entity_pos_t::FromInt(1); }
	virtual const GridUpdateInformation& GetAIPathfinderDirtinessInformation() const override { static GridUpdateInformation gridInfo; return gridInfo; }
	virtual void FlushAIPathfinderDirtinessInformation() override {}
	virtual Grid<u16> ComputeShoreGrid(bool = false) override { return Grid<u16> {}; }
	virtual u32 ComputePathAsync(entity_pos_t, entity_pos_t, const PathGoal&, pass_class_t, entity_id_t) override { return 1; }
	virtual void ComputePathImmediate(entity_pos_t, entity_pos_t, const PathGoal&, pass_class_t, WaypointPath&) const override {}
	virtual u32 ComputeShortPathAsync(entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t, const PathGoal&, pass_class_t, bool, entity_id_t, entity_id_t) override { return 1; }
	virtual WaypointPath ComputeShortPathImmediate(const ShortPathRequest&) const override { return WaypointPath(); }
	virtual void SetDebugPath(entity_pos_t, entity_pos_t, const PathGoal&, pass_class_t) override {}
	virtual bool IsGoalReachable(entity_pos_t, entity_pos_t, const PathGoal&, pass_class_t) override { return false; }
	virtual bool CheckMovement(const IObstructionTestFilter&, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t, pass_class_t) const override { return false; }
	virtual ICmpObstruction::EFoundationCheck CheckUnitPlacement(const IObstructionTestFilter&, entity_pos_t, entity_pos_t, entity_pos_t, pass_class_t, bool = false) const override { return ICmpObstruction::FOUNDATION_CHECK_SUCCESS; }
	virtual ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter&, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t, entity_id_t, pass_class_t) const override { return ICmpObstruction::FOUNDATION_CHECK_SUCCESS; }
	virtual ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter&, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t, entity_pos_t, entity_id_t, pass_class_t, bool) const override { return ICmpObstruction::FOUNDATION_CHECK_SUCCESS; }
	virtual void SetDebugOverlay(bool) override {}
	virtual void SetHierDebugOverlay(bool) override {}
	virtual void SendRequestedPaths() override {}
	virtual void StartProcessingMoves(bool) override {}
	virtual void UpdateGrid() override {}
	virtual void GetDebugData(u32&, double&, Grid<u8>&) const override {}
	virtual void SetAtlasOverlay(bool, pass_class_t = 0) override {}
};

class MockPlayerMgrTerrMan : public ICmpPlayerManager
{
public:
	DEFAULT_MOCK_COMPONENT()

	int32_t GetNumPlayers() override { return 2; }
	entity_id_t GetPlayerByID(int32_t id) override { return id + 1; }
};

class MockTerrInfTerrMan : public ICmpTerritoryInfluence
{
public:
	DEFAULT_MOCK_COMPONENT()

	bool IsRoot() const override { return true; };
	u16 GetWeight() const override { return 10; };
	u32 GetRadius() const override { return m_Radius; };

	u32 m_Radius = 0;
};

class MockOwnershipTerrMan : public ICmpOwnership
{
public:
	DEFAULT_MOCK_COMPONENT()

	player_id_t GetOwner() const override { return 1; };
	void SetOwner(player_id_t) override {};
	void SetOwnerQuiet(player_id_t) override {};
};

class MockPositionTerrMan : public ICmpPosition
{
public:
	DEFAULT_MOCK_COMPONENT()

	void SetTurretParent(entity_id_t, const CFixedVector3D&) override {}
	entity_id_t GetTurretParent() const override { return INVALID_ENTITY; }
	void UpdateTurretPosition() override {}
	std::set<entity_id_t>* GetTurrets() override { return nullptr; }
	bool IsInWorld() const override { return true; }
	void MoveOutOfWorld() override {}
	void MoveTo(entity_pos_t, entity_pos_t) override {}
	void MoveAndTurnTo(entity_pos_t, entity_pos_t, entity_angle_t) override {}
	void JumpTo(entity_pos_t, entity_pos_t) override {}
	void SetHeightOffset(entity_pos_t) override {}
	entity_pos_t GetHeightOffset() const override { return entity_pos_t::Zero(); }
	void SetHeightFixed(entity_pos_t) override {}
	entity_pos_t GetHeightFixed() const override { return entity_pos_t::Zero(); }
	entity_pos_t GetHeightAtFixed(entity_pos_t, entity_pos_t) const override { return entity_pos_t::Zero(); }
	bool IsHeightRelative() const override { return true; }
	void SetHeightRelative(bool) override {}
	bool CanFloat() const override { return false; }
	void SetFloating(bool) override {}
	void SetActorFloating(bool) override {}
	void SetConstructionProgress(fixed) override {}
	CFixedVector3D GetPosition() const override { return m_Pos; }
	CFixedVector2D GetPosition2D() const override { return CFixedVector2D(m_Pos.X, m_Pos.Z); }
	CFixedVector3D GetPreviousPosition() const override { return CFixedVector3D(); }
	CFixedVector2D GetPreviousPosition2D() const override { return CFixedVector2D(); }
	fixed GetTurnRate() const override { return fixed::Zero(); }
	void TurnTo(entity_angle_t) override {}
	void SetYRotation(entity_angle_t) override {}
	void SetXZRotation(entity_angle_t, entity_angle_t) override {}
	CFixedVector3D GetRotation() const override { return CFixedVector3D(); }
	fixed GetDistanceTravelled() const override { return fixed::Zero(); }
	void GetInterpolatedPosition2D(float, float&, float&, float&) const override {}
	CMatrix3D GetInterpolatedTransform(float) const override { return CMatrix3D(); }

	CFixedVector3D m_Pos;
};


class TestCmpTerritoryManager : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		CxxTest::setAbortTestOnFail(true);
		g_VFS = CreateVfs();
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir() / "mods" / "_test.sim" / "", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(g_VFS->Mount(L"cache", DataDir() / "_testcache" / "", 0, VFS_MAX_PRIORITY));
		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();
		g_VFS.reset();
		DeleteDirectory(DataDir()/"_testcache");
	}

	// Regression test for D5181 / fix for rP27673 issue
	void test_calculate_territories_uninitialised()
	{
		ComponentTestHelper test(g_ScriptContext);
		ICmpTerritoryManager* cmp = test.Add<ICmpTerritoryManager>(CID_TerritoryManager, "", SYSTEM_ENTITY);

		MockPathfinderTerrMan pathfinder;
		test.AddMock(SYSTEM_ENTITY, IID_Pathfinder, pathfinder);

		MockPlayerMgrTerrMan playerMan;
		test.AddMock(SYSTEM_ENTITY, IID_PlayerManager, playerMan);

		pathfinder.m_PassabilityGrid.resize(ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE * 5, ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE * 5);

		MockTerrInfTerrMan terrInf;
		test.AddMock(5, IID_TerritoryInfluence, terrInf);
		MockOwnershipTerrMan ownership;
		test.AddMock(5, IID_Ownership, ownership);
		MockPositionTerrMan position;
		test.AddMock(5, IID_Position, position);

		position.m_Pos = CFixedVector3D(
			entity_pos_t::FromInt(ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE * 5 + ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE / 2),
			entity_pos_t::FromInt(1),
			entity_pos_t::FromInt(ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE * 5 + ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE / 2)
		);
		terrInf.m_Radius = 1;

		TS_ASSERT_EQUALS(cmp->GetTerritoryPercentage(0), 0);
		TS_ASSERT_EQUALS(cmp->GetTerritoryPercentage(1), 4); // 5*5 = 25 -> 1 tile out of 25 is 4%
		TS_ASSERT_EQUALS(cmp->GetTerritoryPercentage(2), 0);

		terrInf.m_Radius = ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE * 10;

		test.HandleMessage(cmp, CMessageTerrainChanged(0, 0, 0, 0), true);

		TS_ASSERT_EQUALS(cmp->GetTerritoryPercentage(0), 0);
		TS_ASSERT_EQUALS(cmp->GetTerritoryPercentage(1), 100);
		TS_ASSERT_EQUALS(cmp->GetTerritoryPercentage(2), 0);
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

		// assumes NAVCELLS_PER_TERRITORY_TILE is 8; dealt with in TestBoundaryPointsEqual
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
	Grid<u8> GetGrid(const std::string& def, u16 w, u16 h)
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

	void TestBoundaryPointsEqual(const std::vector<CVector2D>& points, int expectedPoints[][2])
	{
		// TODO: currently relies on an exact point match, i.e. expectedPoints must be specified going CCW or CW (depending on
		// whether we're testing an inner or an outer edge) starting from the exact same point that the algorithm happened to
		// decide to start the run from. This is an algorithmic detail and is not considered to be part of the specification
		// of the return value. Hence, this method should also accept 'expectedPoints' to be a cyclically shifted
		// version of 'points', so that the starting position doesn't need to match exactly.
		for (size_t i = 0; i < points.size(); i++)
		{
			// the input numbers in expectedPoints are defined under the assumption that NAVCELLS_PER_TERRITORY_TILE is 8, so let's include
			// a scaling factor to protect against that should NAVCELLS_PER_TERRITORY_TILE ever change
			TS_ASSERT_DELTA(points[i].X, float(expectedPoints[i][0]) * 8.f / ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE, 1e-7);
			TS_ASSERT_DELTA(points[i].Y, float(expectedPoints[i][1]) * 8.f / ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE, 1e-7);
		}
	}
};
