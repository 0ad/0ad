/* Copyright (C) 2023 Wildfire Games.
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

#include "maths/Matrix3D.h"
#include "simulation2/system/ComponentTest.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpObstruction.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpVision.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>

class MockVisionRgm : public ICmpVision
{
public:
	DEFAULT_MOCK_COMPONENT()

	entity_pos_t GetRange() const override { return entity_pos_t::FromInt(66); }
	bool GetRevealShore() const override { return false; }
};

class MockPositionRgm : public ICmpPosition
{
public:
	DEFAULT_MOCK_COMPONENT()

	void SetTurretParent(entity_id_t UNUSED(id), const CFixedVector3D& UNUSED(pos)) override {}
	entity_id_t GetTurretParent() const override {return INVALID_ENTITY;}
	void UpdateTurretPosition() override {}
	std::set<entity_id_t>* GetTurrets() override { return nullptr; }
	bool IsInWorld() const override { return true; }
	void MoveOutOfWorld() override { }
	void MoveTo(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z)) override { }
	void MoveAndTurnTo(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z), entity_angle_t UNUSED(a)) override { }
	void JumpTo(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z)) override { }
	void SetHeightOffset(entity_pos_t UNUSED(dy)) override { }
	entity_pos_t GetHeightOffset() const override { return entity_pos_t::Zero(); }
	void SetHeightFixed(entity_pos_t UNUSED(y)) override { }
	entity_pos_t GetHeightFixed() const override { return entity_pos_t::Zero(); }
	entity_pos_t GetHeightAtFixed(entity_pos_t, entity_pos_t) const override { return entity_pos_t::Zero(); }
	bool IsHeightRelative() const override { return true; }
	void SetHeightRelative(bool UNUSED(relative)) override { }
	bool CanFloat() const override { return false; }
	void SetFloating(bool UNUSED(flag)) override { }
	void SetActorFloating(bool UNUSED(flag)) override { }
	void SetConstructionProgress(fixed UNUSED(progress)) override { }
	CFixedVector3D GetPosition() const override { return m_Pos; }
	CFixedVector2D GetPosition2D() const override { return CFixedVector2D(m_Pos.X, m_Pos.Z); }
	CFixedVector3D GetPreviousPosition() const override { return CFixedVector3D(); }
	CFixedVector2D GetPreviousPosition2D() const override { return CFixedVector2D(); }
	fixed GetTurnRate() const override { return fixed::Zero(); }
	void TurnTo(entity_angle_t UNUSED(y)) override { }
	void SetYRotation(entity_angle_t UNUSED(y)) override { }
	void SetXZRotation(entity_angle_t UNUSED(x), entity_angle_t UNUSED(z)) override { }
	CFixedVector3D GetRotation() const override { return CFixedVector3D(); }
	fixed GetDistanceTravelled() const override { return fixed::Zero(); }
	void GetInterpolatedPosition2D(float UNUSED(frameOffset), float& x, float& z, float& rotY) const override { x = z = rotY = 0; }
	CMatrix3D GetInterpolatedTransform(float UNUSED(frameOffset)) const override { return CMatrix3D(); }

	CFixedVector3D m_Pos;
};

class MockObstructionRgm : public ICmpObstruction
{
public:
	DEFAULT_MOCK_COMPONENT();

	MockObstructionRgm(entity_pos_t s) : m_Size(s) {};

	ICmpObstructionManager::tag_t GetObstruction() const override { return {}; };
	bool GetObstructionSquare(ICmpObstructionManager::ObstructionSquare&) const override { return false; };
	bool GetPreviousObstructionSquare(ICmpObstructionManager::ObstructionSquare&) const override { return false; };
	entity_pos_t GetSize() const override { return m_Size; };
	CFixedVector2D GetStaticSize() const override { return {}; };
	EObstructionType GetObstructionType() const override { return {}; };
	void SetUnitClearance(const entity_pos_t&) override {};
	bool IsControlPersistent() const override { return {}; };
	bool CheckShorePlacement() const override { return {}; };
	EFoundationCheck CheckFoundation(const std::string&) const override { return {}; };
	EFoundationCheck CheckFoundation(const std::string& , bool) const override { return {}; };
	std::string CheckFoundation_wrapper(const std::string&, bool) const override { return {}; };
	bool CheckDuplicateFoundation() const override { return {}; };
	std::vector<entity_id_t> GetEntitiesByFlags(ICmpObstructionManager::flags_t) const override { return {}; };
	std::vector<entity_id_t> GetEntitiesBlockingMovement() const override { return {}; };
	std::vector<entity_id_t> GetEntitiesBlockingConstruction() const override { return {}; };
	std::vector<entity_id_t> GetEntitiesDeletedUponConstruction() const override { return {}; };
	void ResolveFoundationCollisions() const override {};
	void SetActive(bool) override {};
	void SetMovingFlag(bool) override {};
	void SetDisableBlockMovementPathfinding(bool, bool, int32_t) override {};
	bool GetBlockMovementFlag(bool) const override { return {}; };
	void SetControlGroup(entity_id_t) override {};
	entity_id_t GetControlGroup() const override { return {}; };
	void SetControlGroup2(entity_id_t) override {};
	entity_id_t GetControlGroup2() const override { return {}; };
private:
	entity_pos_t m_Size;
};

class TestCmpRangeManager : public CxxTest::TestSuite
{
public:
	void setUp()
	{
		CXeromyces::Startup();
	}

	void tearDown()
	{
		CXeromyces::Terminate();
	}

	// TODO It would be nice to call Verify() with the shore revealing system
	// but that means testing on an actual map, with water and land.

	void test_basic()
	{
		ComponentTestHelper test(*g_ScriptContext);

		ICmpRangeManager* cmp = test.Add<ICmpRangeManager>(CID_RangeManager, "", SYSTEM_ENTITY);

		MockVisionRgm vision;
		test.AddMock(100, IID_Vision, vision);

		MockPositionRgm position;
		test.AddMock(100, IID_Position, position);

		// This tests that the incremental computation produces the correct result
		// in various edge cases

		cmp->SetBounds(entity_pos_t::FromInt(0), entity_pos_t::FromInt(0), entity_pos_t::FromInt(512), entity_pos_t::FromInt(512));
		cmp->Verify();
		{ CMessageCreate msg(100); cmp->HandleMessage(msg, false); }
		cmp->Verify();
		{ CMessageOwnershipChanged msg(100, -1, 1); cmp->HandleMessage(msg, false); }
		cmp->Verify();
		{ CMessagePositionChanged msg(100, true, entity_pos_t::FromInt(247), entity_pos_t::FromDouble(257.95), entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
		cmp->Verify();
		{ CMessagePositionChanged msg(100, true, entity_pos_t::FromInt(247), entity_pos_t::FromInt(253), entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
		cmp->Verify();

		{ CMessagePositionChanged msg(100, true, entity_pos_t::FromInt(256), entity_pos_t::FromInt(256), entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
		cmp->Verify();

		{ CMessagePositionChanged msg(100, true, entity_pos_t::FromInt(256)+entity_pos_t::Epsilon(), entity_pos_t::FromInt(256), entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
		cmp->Verify();
		{ CMessagePositionChanged msg(100, true, entity_pos_t::FromInt(256)-entity_pos_t::Epsilon(), entity_pos_t::FromInt(256), entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
		cmp->Verify();
		{ CMessagePositionChanged msg(100, true, entity_pos_t::FromInt(256), entity_pos_t::FromInt(256)+entity_pos_t::Epsilon(), entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
		cmp->Verify();
		{ CMessagePositionChanged msg(100, true, entity_pos_t::FromInt(256), entity_pos_t::FromInt(256)-entity_pos_t::Epsilon(), entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
		cmp->Verify();

		{ CMessagePositionChanged msg(100, true, entity_pos_t::FromInt(383), entity_pos_t::FromInt(84), entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
		cmp->Verify();
		{ CMessagePositionChanged msg(100, true, entity_pos_t::FromInt(348), entity_pos_t::FromInt(83), entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
		cmp->Verify();

		boost::mt19937 rng;
		for (size_t i = 0; i < 1024; ++i)
		{
			double x = boost::random::uniform_real_distribution<double>(0.0, 512.0)(rng);
			double z = boost::random::uniform_real_distribution<double>(0.0, 512.0)(rng);
			{ CMessagePositionChanged msg(100, true, entity_pos_t::FromDouble(x), entity_pos_t::FromDouble(z), entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
			cmp->Verify();
		}

		// Test OwnershipChange, GetEntitiesByPlayer, GetNonGaiaEntities
		{
			player_id_t previousOwner = -1;
			for (player_id_t newOwner = 0; newOwner < 8; ++newOwner)
			{
				CMessageOwnershipChanged msg(100, previousOwner, newOwner);
				cmp->HandleMessage(msg, false);

				for (player_id_t i = 0; i < 8; ++i)
					TS_ASSERT_EQUALS(cmp->GetEntitiesByPlayer(i).size(), i == newOwner ? 1 : 0);

				TS_ASSERT_EQUALS(cmp->GetNonGaiaEntities().size(), newOwner > 0 ? 1 : 0);
				previousOwner = newOwner;
			}
		}
	}

	void test_queries()
	{
		ComponentTestHelper test(*g_ScriptContext);

		ICmpRangeManager* cmp = test.Add<ICmpRangeManager>(CID_RangeManager, "", SYSTEM_ENTITY);

		MockVisionRgm vision, vision2;
		MockPositionRgm position, position2;
		MockObstructionRgm obs(fixed::FromInt(2)), obs2(fixed::Zero());
		test.AddMock(100, IID_Vision, vision);
		test.AddMock(100, IID_Position, position);
		test.AddMock(100, IID_Obstruction, obs);

		test.AddMock(101, IID_Vision, vision2);
		test.AddMock(101, IID_Position, position2);
		test.AddMock(101, IID_Obstruction, obs2);

		cmp->SetBounds(entity_pos_t::FromInt(0), entity_pos_t::FromInt(0), entity_pos_t::FromInt(512), entity_pos_t::FromInt(512));
		cmp->Verify();
		{ CMessageCreate msg(100); cmp->HandleMessage(msg, false); }
		{ CMessageCreate msg(101); cmp->HandleMessage(msg, false); }

		{ CMessageOwnershipChanged msg(100, -1, 1); cmp->HandleMessage(msg, false); }
		{ CMessageOwnershipChanged msg(101, -1, 1); cmp->HandleMessage(msg, false); }

		auto move = [&cmp](entity_id_t ent, MockPositionRgm& pos, fixed x, fixed z) {
			pos.m_Pos = CFixedVector3D(x, fixed::Zero(), z);
			{ CMessagePositionChanged msg(ent, true, x, z, entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
		};

		move(100, position, fixed::FromInt(10), fixed::FromInt(10));
		move(101, position2, fixed::FromInt(10), fixed::FromInt(20));

		std::vector<entity_id_t> nearby = cmp->ExecuteQuery(100, fixed::FromInt(0), fixed::FromInt(4), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{});
		nearby = cmp->ExecuteQuery(100, fixed::FromInt(4), fixed::FromInt(50), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{101});

		move(101, position2, fixed::FromInt(10), fixed::FromInt(10));
		nearby = cmp->ExecuteQuery(100, fixed::FromInt(0), fixed::FromInt(4), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{101});
		nearby = cmp->ExecuteQuery(100, fixed::FromInt(4), fixed::FromInt(50), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{});

		move(101, position2, fixed::FromInt(10), fixed::FromInt(13));
		nearby = cmp->ExecuteQuery(100, fixed::FromInt(0), fixed::FromInt(4), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{101});
		nearby = cmp->ExecuteQuery(100, fixed::FromInt(4), fixed::FromInt(50), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{});

		move(101, position2, fixed::FromInt(10), fixed::FromInt(15));
		// In range thanks to self obstruction size.
		nearby = cmp->ExecuteQuery(100, fixed::FromInt(0), fixed::FromInt(4), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{101});
		// In range thanks to target obstruction size.
		nearby = cmp->ExecuteQuery(101, fixed::FromInt(0), fixed::FromInt(4), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{100});

		// Trickier: min-range is closest-to-closest, but rotation may change the real distance.
		nearby = cmp->ExecuteQuery(100, fixed::FromInt(2), fixed::FromInt(50), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{101});
		nearby = cmp->ExecuteQuery(100, fixed::FromInt(5), fixed::FromInt(50), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{101});
		nearby = cmp->ExecuteQuery(100, fixed::FromInt(6), fixed::FromInt(50), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{});
		nearby = cmp->ExecuteQuery(101, fixed::FromInt(5), fixed::FromInt(50), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{100});
		nearby = cmp->ExecuteQuery(101, fixed::FromInt(6), fixed::FromInt(50), {1}, 0, true);
		TS_ASSERT_EQUALS(nearby, std::vector<entity_id_t>{});

	}

	void test_IsInTargetParabolicRange()
	{
		ComponentTestHelper test(*g_ScriptContext);
		ICmpRangeManager* cmp = test.Add<ICmpRangeManager>(CID_RangeManager, "", SYSTEM_ENTITY);
		const entity_id_t source = 200;
		const entity_id_t target = 201;
		entity_pos_t range = fixed::FromInt(-3);
		entity_pos_t yOrigin = fixed::FromInt(-20);

		// Invalid range.
		TS_ASSERT_EQUALS(cmp->GetEffectiveParabolicRange(source, target, range, yOrigin), range);

		// No source ICmpPosition.
		range = fixed::FromInt(10);
		TS_ASSERT_EQUALS(cmp->GetEffectiveParabolicRange(source, target, range, yOrigin), NEVER_IN_RANGE);

		// No target ICmpPosition.
		MockPositionRgm cmpSourcePosition;
		test.AddMock(source, IID_Position, cmpSourcePosition);
		TS_ASSERT_EQUALS(cmp->GetEffectiveParabolicRange(source, target, range, yOrigin), NEVER_IN_RANGE);

		// Too much height difference.
		MockPositionRgm cmpTargetPosition;
		test.AddMock(target, IID_Position, cmpTargetPosition);
		TS_ASSERT_EQUALS(cmp->GetEffectiveParabolicRange(source, target, range, yOrigin), NEVER_IN_RANGE);

		// If no offset we get the range.
		range = fixed::FromInt(20);
		yOrigin = fixed::Zero();
		TS_ASSERT_EQUALS(cmp->GetEffectiveParabolicRange(source, target, range, yOrigin), range);
		TS_ASSERT_EQUALS(cmp->GetEffectiveParabolicRange(source, target, fixed::Zero(), yOrigin), fixed::Zero());

		// Normal case.
		yOrigin = fixed::FromInt(5);
		range = fixed::FromInt(10);
		TS_ASSERT_EQUALS(cmp->GetEffectiveParabolicRange(source, target, range, yOrigin), fixed::FromFloat(14.142136f));

		// Big range.
		range = fixed::FromInt(260);
		TS_ASSERT_EQUALS(cmp->GetEffectiveParabolicRange(source, target, range, yOrigin), fixed::FromFloat(264.952820f));
	}
};
