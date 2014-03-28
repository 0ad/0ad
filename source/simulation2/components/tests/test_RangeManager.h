/* Copyright (C) 2011 Wildfire Games.
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

#include "maths/Random.h"

#include <boost/random/uniform_real.hpp>

class MockVision : public ICmpVision
{
public:
	DEFAULT_MOCK_COMPONENT()

	virtual entity_pos_t GetRange() { return entity_pos_t::FromInt(66); }
	virtual bool GetRetainInFog() { return false; }
	virtual bool GetAlwaysVisible() { return false; }
};

class MockPosition : public ICmpPosition
{
public:
	DEFAULT_MOCK_COMPONENT()

	virtual bool IsInWorld() { return true; }
	virtual void MoveOutOfWorld() { }
	virtual void MoveTo(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z)) { }
	virtual void MoveAndTurnTo(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z), entity_angle_t UNUSED(a)) { }
	virtual void JumpTo(entity_pos_t UNUSED(x), entity_pos_t UNUSED(z)) { }
	virtual void SetHeightOffset(entity_pos_t UNUSED(dy)) { }
	virtual entity_pos_t GetHeightOffset() { return entity_pos_t::Zero(); }
	virtual void SetHeightFixed(entity_pos_t UNUSED(y)) { }
	virtual bool IsFloating() { return false; }
	virtual CFixedVector3D GetPosition() { return CFixedVector3D(); }
	virtual CFixedVector2D GetPosition2D() { return CFixedVector2D(); }
	virtual CFixedVector3D GetPreviousPosition() { return CFixedVector3D(); }
	virtual CFixedVector2D GetPreviousPosition2D() { return CFixedVector2D(); }
	virtual void TurnTo(entity_angle_t UNUSED(y)) { }
	virtual void SetYRotation(entity_angle_t UNUSED(y)) { }
	virtual void SetXZRotation(entity_angle_t UNUSED(x), entity_angle_t UNUSED(z)) { }
	virtual CFixedVector3D GetRotation() { return CFixedVector3D(); }
	virtual fixed GetDistanceTravelled() { return fixed::Zero(); }
	virtual void GetInterpolatedPosition2D(float UNUSED(frameOffset), float& x, float& z, float& rotY) { x = z = rotY = 0; }
	virtual CMatrix3D GetInterpolatedTransform(float UNUSED(frameOffset), bool UNUSED(forceFloating)) { return CMatrix3D(); }
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

	void test_basic()
	{
		ComponentTestHelper test(ScriptInterface::CreateRuntime());

		ICmpRangeManager* cmp = test.Add<ICmpRangeManager>(CID_RangeManager, "");

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

		WELL512 rng;
		for (size_t i = 0; i < 1024; ++i)
		{
			double x = boost::uniform_real<>(0.0, 512.0)(rng);
			double z = boost::uniform_real<>(0.0, 512.0)(rng);
			{ CMessagePositionChanged msg(100, true, entity_pos_t::FromDouble(x), entity_pos_t::FromDouble(z), entity_angle_t::Zero()); cmp->HandleMessage(msg, false); }
			cmp->Verify();
		}
	}
};
