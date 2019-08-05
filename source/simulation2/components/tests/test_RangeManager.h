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
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpVision.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>

class MockVision : public ICmpVision
{
public:
	DEFAULT_MOCK_COMPONENT()

	virtual entity_pos_t GetRange() const { return entity_pos_t::FromInt(66); }
	virtual bool GetRevealShore() const { return false; }
};

class MockPosition : public ICmpPosition
{
public:
	DEFAULT_MOCK_COMPONENT()

	virtual void SetTurretParent(entity_id_t UNUSED(id), const CFixedVector3D& UNUSED(pos)) {}
	virtual entity_id_t GetTurretParent() const {return INVALID_ENTITY;}
	virtual void UpdateTurretPosition() {}
	virtual std::set<entity_id_t>* GetTurrets() { return NULL; }
	virtual bool IsInWorld() const { return true; }
	virtual void MoveOutOfWorld() { }
	virtual void MoveTo(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z)) { }
	virtual void MoveAndTurnTo(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z), entity_angle_t UNUSED(a)) { }
	virtual void JumpTo(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z)) { }
	virtual void SetHeightOffset(entity_pos_t UNUSED(dy)) { }
	virtual entity_pos_t GetHeightOffset() const { return entity_pos_t::Zero(); }
	virtual void SetHeightFixed(entity_pos_t UNUSED(y)) { }
	virtual entity_pos_t GetHeightFixed() const { return entity_pos_t::Zero(); }
	virtual bool IsHeightRelative() const { return true; }
	virtual void SetHeightRelative(bool UNUSED(relative)) { }
	virtual bool CanFloat() const { return false; }
	virtual void SetFloating(bool UNUSED(flag)) { }
	virtual void SetActorFloating(bool UNUSED(flag)) { }
	virtual void SetConstructionProgress(fixed UNUSED(progress)) { }
	virtual CFixedVector3D GetPosition() const { return CFixedVector3D(); }
	virtual CFixedVector2D GetPosition2D() const { return CFixedVector2D(); }
	virtual CFixedVector3D GetPreviousPosition() const { return CFixedVector3D(); }
	virtual CFixedVector2D GetPreviousPosition2D() const { return CFixedVector2D(); }
	virtual void TurnTo(entity_angle_t UNUSED(y)) { }
	virtual void SetYRotation(entity_angle_t UNUSED(y)) { }
	virtual void SetXZRotation(entity_angle_t UNUSED(x), entity_angle_t UNUSED(z)) { }
	virtual CFixedVector3D GetRotation() const { return CFixedVector3D(); }
	virtual fixed GetDistanceTravelled() const { return fixed::Zero(); }
	virtual void GetInterpolatedPosition2D(float UNUSED(frameOffset), float& x, float& z, float& rotY) const { x = z = rotY = 0; }
	virtual CMatrix3D GetInterpolatedTransform(float UNUSED(frameOffset)) const { return CMatrix3D(); }
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
		ComponentTestHelper test(g_ScriptRuntime);

		ICmpRangeManager* cmp = test.Add<ICmpRangeManager>(CID_RangeManager, "", SYSTEM_ENTITY);

		MockVision vision;
		test.AddMock(100, IID_Vision, vision);

		MockPosition position;
		test.AddMock(100, IID_Position, position);

		// This tests that the incremental computation produces the correct result
		// in various edge cases

		cmp->SetBounds(entity_pos_t::FromInt(0), entity_pos_t::FromInt(0), entity_pos_t::FromInt(512), entity_pos_t::FromInt(512), 512/TERRAIN_TILE_SIZE + 1);
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
};
