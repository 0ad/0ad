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

#include "simulation2/components/ICmpPosition.h"

class TestCmpPosition : public CxxTest::TestSuite
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

	static CFixedVector3D fixedvec(int x, int y, int z)
	{
		return CFixedVector3D(fixed::FromInt(x), fixed::FromInt(y), fixed::FromInt(z));
	}

	void test_basic()
	{
		ComponentTestHelper test(g_ScriptRuntime);

		MockTerrain terrain;
		test.AddMock(SYSTEM_ENTITY, IID_Terrain, terrain);

		ICmpPosition* cmp = test.Add<ICmpPosition>(CID_Position, "<Anchor>upright</Anchor><Altitude>23</Altitude><Floating>false</Floating>");

		// Defaults
		TS_ASSERT(!cmp->IsInWorld());
		TS_ASSERT_EQUALS(cmp->GetHeightOffset(), entity_pos_t::FromInt(23));
		TS_ASSERT_EQUALS(cmp->GetRotation(), fixedvec(0, 0, 0));

		// Change height offset
		cmp->SetHeightOffset(entity_pos_t::FromInt(10));
		TS_ASSERT_EQUALS(cmp->GetHeightOffset(), entity_pos_t::FromInt(10));

		// Move out of world, while currently out of world
		cmp->MoveOutOfWorld();
		TS_ASSERT(!cmp->IsInWorld());

		// Jump into world
		cmp->JumpTo(entity_pos_t::FromInt(0), entity_pos_t::FromInt(0));
		TS_ASSERT(cmp->IsInWorld());
		// Move out of world, while currently in world
		cmp->MoveOutOfWorld();
		TS_ASSERT(!cmp->IsInWorld());

		// Move into world
		cmp->MoveTo(entity_pos_t::FromInt(100), entity_pos_t::FromInt(200));
		TS_ASSERT(cmp->IsInWorld());
		// Position computed from move plus terrain
		TS_ASSERT_EQUALS(cmp->GetPosition(), fixedvec(100, 60, 200));
		// Interpolated position should be constant
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(0.0f, false).GetTranslation(), CVector3D(100, 60, 200));
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(0.5f, false).GetTranslation(), CVector3D(100, 60, 200));
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(1.0f, false).GetTranslation(), CVector3D(100, 60, 200));

		// No TurnStart message, so this move doesn't affect the interpolation
		cmp->MoveTo(entity_pos_t::FromInt(0), entity_pos_t::FromInt(0));
		// Move smoothly to new position
		cmp->MoveTo(entity_pos_t::FromInt(200), entity_pos_t::FromInt(0));
		// Position computed from move plus terrain
		TS_ASSERT_EQUALS(cmp->GetPosition(), fixedvec(200, 60, 0));
		// Interpolated position should vary, from original move into world to new move
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(0.0f, false).GetTranslation(), CVector3D(100, 60, 200));
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(0.5f, false).GetTranslation(), CVector3D(150, 60, 100));
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(1.0f, false).GetTranslation(), CVector3D(200, 60, 0));

		// Latch new position for interpolation
		CMessageTurnStart msg;
		test.HandleMessage(cmp, msg, false);

		// Move smoothly to new position
		cmp->MoveTo(entity_pos_t::FromInt(400), entity_pos_t::FromInt(300));
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(0.0f, false).GetTranslation(), CVector3D(200, 60, 0));
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(0.5f, false).GetTranslation(), CVector3D(300, 60, 150));
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(1.0f, false).GetTranslation(), CVector3D(400, 60, 300));

		// Jump to new position
		cmp->JumpTo(entity_pos_t::FromInt(300), entity_pos_t::FromInt(100));
		// Position computed from jump plus terrain
		TS_ASSERT_EQUALS(cmp->GetPosition(), fixedvec(300, 60, 100));
		// Interpolated position should be constant after jump
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(0.0f, false).GetTranslation(), CVector3D(300, 60, 100));
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(0.5f, false).GetTranslation(), CVector3D(300, 60, 100));
		TS_ASSERT_EQUALS(cmp->GetInterpolatedTransform(1.0f, false).GetTranslation(), CVector3D(300, 60, 100));

		// TODO: Test the rotation methods
	}

	void test_serialize()
	{
		ComponentTestHelper test(g_ScriptRuntime);

		MockTerrain terrain;
		test.AddMock(SYSTEM_ENTITY, IID_Terrain, terrain);

		ICmpPosition* cmp = test.Add<ICmpPosition>(CID_Position, "<Anchor>upright</Anchor><Altitude>5</Altitude><Floating>false</Floating>");

		test.Roundtrip();

		cmp->SetHeightOffset(entity_pos_t::FromInt(20));
		cmp->SetXZRotation(entity_angle_t::FromInt(1), entity_angle_t::FromInt(2));
		cmp->SetYRotation(entity_angle_t::FromInt(3));

		test.Roundtrip();

		cmp->JumpTo(entity_pos_t::FromInt(10), entity_pos_t::FromInt(20));
		cmp->MoveTo(entity_pos_t::FromInt(123), entity_pos_t::FromInt(456));

		test.Roundtrip();
	}
};
