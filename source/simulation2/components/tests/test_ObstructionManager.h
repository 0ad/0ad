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

#include "simulation2/components/ICmpObstructionManager.h"

class TestCmpObstructionManager : public CxxTest::TestSuite
{
	typedef ICmpObstructionManager::tag_t tag_t;
	typedef ICmpObstructionManager::ObstructionSquare ObstructionSquare;

	// some variables for setting up a scene with 3 shapes
	entity_id_t ent1, ent2, ent3; // entity IDs
	entity_angle_t ent1a, ent2r, ent3r; // angles/radiuses
	entity_pos_t ent1x, ent1z, ent1w, ent1h, // positions/dimensions
	             ent2x, ent2z,
	             ent3x, ent3z;
	entity_id_t ent1g1, ent1g2, ent2g, ent3g; // control groups

	tag_t shape1, shape2, shape3;
	
	ICmpObstructionManager* cmp;
	ComponentTestHelper* testHelper;

public:
	void setUp()
	{
		CXeromyces::Startup();
		CxxTest::setAbortTestOnFail(true);

		// set up a simple scene with some predefined obstruction shapes
		// (we can't position shapes on the origin because the world bounds must range 
		// from 0 to X, so instead we'll offset things by, say, 10).

		ent1 = 1;
		ent1a = fixed::Zero();
		ent1w = fixed::FromFloat(4);
		ent1h = fixed::FromFloat(2);
		ent1x = fixed::FromInt(10);
		ent1z = fixed::FromInt(10);
		ent1g1 = ent1;
		ent1g2 = INVALID_ENTITY;

		ent2 = 2;
		ent2r = fixed::FromFloat(1);
		ent2x = ent1x;
		ent2z = ent1z;
		ent2g = ent1g1;

		ent3 = 3;
		ent3r = fixed::FromFloat(3);
		ent3x = ent2x;
		ent3z = ent2z + ent2r + ent3r; // ensure it just touches the border of ent2
		ent3g = ent3;

		testHelper = new ComponentTestHelper(g_ScriptRuntime);
		cmp = testHelper->Add<ICmpObstructionManager>(CID_ObstructionManager, "");
		cmp->SetBounds(fixed::FromInt(0), fixed::FromInt(0), fixed::FromInt(1000), fixed::FromInt(1000));

		shape1 = cmp->AddStaticShape(ent1, ent1x, ent1z, ent1a, ent1w, ent1h,
			ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION |
			ICmpObstructionManager::FLAG_BLOCK_MOVEMENT |
			ICmpObstructionManager::FLAG_MOVING, ent1g1, ent1g2);

		shape2 = cmp->AddUnitShape(ent2, ent2x, ent2z, ent2r,
			ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION |
			ICmpObstructionManager::FLAG_BLOCK_FOUNDATION, ent2g);

		shape3 = cmp->AddUnitShape(ent3, ent3x, ent3z, ent3r,
			ICmpObstructionManager::FLAG_BLOCK_MOVEMENT |
			ICmpObstructionManager::FLAG_BLOCK_FOUNDATION, ent3g);
	}

	void tearDown()
	{
		delete testHelper;
		cmp = NULL; // not our responsibility to deallocate

		CXeromyces::Terminate();
	}

	/**
	 * Verifies the collision testing procedure. Collision-tests some simple shapes against the shapes registered in
	 * the scene, and verifies the result of the test against the expected value.
	 */
	void test_simple_collisions()
	{
		std::vector<entity_id_t> out;
		NullObstructionFilter nullFilter;
		
		// Collision-test a simple shape nested inside shape3 against all shapes in the scene. Since the tested shape
		// overlaps only with shape 3, we should find only shape 3 in the result.
		
		cmp->TestUnitShape(nullFilter, ent3x, ent3z, fixed::FromInt(1), &out);
		TS_ASSERT_EQUALS(1U, out.size());
		TS_ASSERT_EQUALS(ent3, out[0]);
		out.clear();

		cmp->TestStaticShape(nullFilter, ent3x, ent3z, fixed::Zero(), fixed::FromInt(1), fixed::FromInt(1), &out);
		TS_ASSERT_EQUALS(1U, out.size());
		TS_ASSERT_EQUALS(ent3, out[0]);
		out.clear();

		// Similarly, collision-test a simple shape nested inside both shape1 and shape2. Since the tested shape overlaps
		// only with shapes 1 and 2, those are the only ones we should find in the result.
		
		cmp->TestUnitShape(nullFilter, ent2x, ent2z, ent2r/2, &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent1);
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		out.clear();

		cmp->TestStaticShape(nullFilter, ent2x, ent2z, fixed::Zero(), ent2r, ent2r, &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent1);
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		out.clear();
	}

	/**
	 * Verifies the behaviour of the null obstruction filter. Tests with this filter will be performed against all
	 * registered shapes.
	 */
	void test_filter_null()
	{
		std::vector<entity_id_t> out;

		// Collision test a scene-covering shape against all shapes in the scene. We should find all registered shapes
		// in the result.

		NullObstructionFilter nullFilter;

		cmp->TestUnitShape(nullFilter, ent1x, ent1z, fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(3U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent1);
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		TS_ASSERT_VECTOR_CONTAINS(out, ent3);
		out.clear();

		cmp->TestStaticShape(nullFilter, ent1x, ent1z, fixed::Zero(), fixed::FromInt(10), fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(3U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent1);
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		TS_ASSERT_VECTOR_CONTAINS(out, ent3);
		out.clear();
	}

	/**
	 * Verifies the behaviour of the StationaryOnlyObstructionFilter. Tests with this filter will be performed only
	 * against non-moving (stationary) shapes.
	 */
	void test_filter_stationary_only()
	{
		std::vector<entity_id_t> out;

		// Collision test a scene-covering shape against all shapes in the scene, but skipping shapes that are moving,
		// i.e. shapes that have the MOVING flag. Since only shape 1 is flagged as moving, we should find 
		// shapes 2 and 3 in each case.

		StationaryOnlyObstructionFilter ignoreMoving;

		cmp->TestUnitShape(ignoreMoving, ent1x, ent1z, fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		TS_ASSERT_VECTOR_CONTAINS(out, ent3);
		out.clear();

		cmp->TestStaticShape(ignoreMoving, ent1x, ent1z, fixed::Zero(), fixed::FromInt(10), fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		TS_ASSERT_VECTOR_CONTAINS(out, ent3);
		out.clear();
	}

	/**
	 * Verifies the behaviour of the SkipTagObstructionFilter. Tests with this filter will be performed against 
	 * all registered shapes that do not have the specified tag set.
	 */
	void test_filter_skip_tag()
	{
		std::vector<entity_id_t> out;

		// Collision-test shape 2's obstruction shape against all shapes in the scene, but skipping tests against
		// shape 2. Since shape 2 overlaps only with shape 1, we should find only shape 1's entity ID in the result.

		SkipTagObstructionFilter ignoreShape2(shape2);

		cmp->TestUnitShape(ignoreShape2, ent2x, ent2z, ent2r/2, &out);
		TS_ASSERT_EQUALS(1U, out.size());
		TS_ASSERT_EQUALS(ent1, out[0]);
		out.clear();

		cmp->TestStaticShape(ignoreShape2, ent2x, ent2z, fixed::Zero(), ent2r, ent2r, &out);
		TS_ASSERT_EQUALS(1U, out.size());
		TS_ASSERT_EQUALS(ent1, out[0]);
		out.clear();
	}

	/**
	 * Verifies the behaviour of the SkipTagFlagsObstructionFilter. Tests with this filter will be performed against
	 * all registered shapes that do not have the specified tag set, and that have at least one of required flags set.
	 */
	void test_filter_skip_tag_require_flag()
	{
		std::vector<entity_id_t> out;

		// Collision-test a scene-covering shape against all shapes in the scene, but skipping tests against shape 1
		// and requiring the BLOCK_MOVEMENT flag. Since shape 1 is being ignored and shape 2 does not have the required
		// flag, we should find only shape 3 in the results.

		SkipTagRequireFlagsObstructionFilter skipShape1RequireBlockMovement(shape1, ICmpObstructionManager::FLAG_BLOCK_MOVEMENT);

		cmp->TestUnitShape(skipShape1RequireBlockMovement, ent1x, ent1z, fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(1U, out.size());
		TS_ASSERT_EQUALS(ent3, out[0]);
		out.clear();

		cmp->TestStaticShape(skipShape1RequireBlockMovement, ent1x, ent1z, fixed::Zero(), fixed::FromInt(10), fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(1U, out.size());
		TS_ASSERT_EQUALS(ent3, out[0]);
		out.clear();

		// If we now do the same test, but require at least one of the entire set of available filters, we should find
		// all shapes that are not shape 1 and that have at least one flag set. Since all shapes in our testing scene
		// have at least one flag set, we should find shape 2 and shape 3 in the results.

		SkipTagRequireFlagsObstructionFilter skipShape1RequireAnyFlag(shape1, (ICmpObstructionManager::flags_t) -1);

		cmp->TestUnitShape(skipShape1RequireAnyFlag, ent1x, ent1z, fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		TS_ASSERT_VECTOR_CONTAINS(out, ent3);
		out.clear();

		cmp->TestStaticShape(skipShape1RequireAnyFlag, ent1x, ent1z, fixed::Zero(), fixed::FromInt(10), fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		TS_ASSERT_VECTOR_CONTAINS(out, ent3);
		out.clear();

		// And if we now do the same test yet again, but specify an empty set of flags, then it becomes impossible for
		// any shape to have at least one of the required flags, and we should hence find no shapes in the result.
		
		SkipTagRequireFlagsObstructionFilter skipShape1RejectAll(shape1, 0U);
		
		cmp->TestUnitShape(skipShape1RejectAll, ent1x, ent1z, fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(0U, out.size());
		out.clear();

		cmp->TestStaticShape(skipShape1RejectAll, ent1x, ent1z, fixed::Zero(), fixed::FromInt(10), fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(0U, out.size());
		out.clear();
	}

	/**
	 * Verifies the behaviour of SkipControlGroupsRequireFlagObstructionFilter. Tests with this filter will be performed 
	 * against all registered shapes that are members of neither specified control groups, and that have at least one of
	 * the specified flags set.
	 */
	void test_filter_skip_controlgroups_require_flag()
	{
		std::vector<entity_id_t> out;

		// Collision-test a shape that overlaps the entire scene, but ignoring shapes from shape1's control group
		// (which also includes shape 2), and requiring that either the BLOCK_FOUNDATION or the 
		// BLOCK_CONSTRUCTION flag is set, or both. Since shape 1 and shape 2 both belong to shape 1's control
		// group, and shape 3 has the BLOCK_FOUNDATION flag (but not BLOCK_CONSTRUCTION), we should find only 
		// shape 3 in the result.

		SkipControlGroupsRequireFlagObstructionFilter skipGroup1ReqFoundConstr(ent1g1, INVALID_ENTITY,
			ICmpObstructionManager::FLAG_BLOCK_FOUNDATION | ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION);

		cmp->TestUnitShape(skipGroup1ReqFoundConstr, ent1x, ent1z, fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(1U, out.size());
		TS_ASSERT_EQUALS(ent3, out[0]);
		out.clear();

		cmp->TestStaticShape(skipGroup1ReqFoundConstr, ent1x, ent1z, fixed::Zero(), fixed::FromInt(10), fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(1U, out.size());
		TS_ASSERT_EQUALS(ent3, out[0]);
		out.clear();

		// Perform the same test, but now also exclude shape 3's control group (in addition to shape 1's control
		// group). Despite shape 3 having at least one of the required flags set, it should now also be ignored,
		// yielding an empty result set.
		
		SkipControlGroupsRequireFlagObstructionFilter skipGroup1And3ReqFoundConstr(ent1g1, ent3g,
			ICmpObstructionManager::FLAG_BLOCK_FOUNDATION | ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION);

		cmp->TestUnitShape(skipGroup1And3ReqFoundConstr, ent1x, ent1z, fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(0U, out.size());
		out.clear();

		cmp->TestStaticShape(skipGroup1And3ReqFoundConstr, ent1x, ent1z, fixed::Zero(), fixed::FromInt(10), fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(0U, out.size());
		out.clear();

		// Same test, but this time excluding only shape 3's control group, and requiring any of the available flags
		// to be set. Since both shape 1 and shape 2 have at least one flag set and are both in a different control 
		// group, we should find them in the result.
		
		SkipControlGroupsRequireFlagObstructionFilter skipGroup3RequireAnyFlag(ent3g, INVALID_ENTITY,
			(ICmpObstructionManager::flags_t) -1);

		cmp->TestUnitShape(skipGroup3RequireAnyFlag, ent1x, ent1z, fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent1);
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		out.clear();

		cmp->TestStaticShape(skipGroup3RequireAnyFlag, ent1x, ent1z, fixed::Zero(), fixed::FromInt(10), fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent1);
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		out.clear();

		// Finally, the same test as the one directly above, now with an empty set of required flags. Since it now becomes
		// impossible for shape 1 and shape 2 to have at least one of the required flags set, and shape 3 is excluded by
		// virtue of the control group filtering, we should find an empty result.

		SkipControlGroupsRequireFlagObstructionFilter skipGroup3RequireNoFlags(ent3g, INVALID_ENTITY, 0U);

		cmp->TestUnitShape(skipGroup3RequireNoFlags, ent1x, ent1z, fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(0U, out.size());
		out.clear();

		cmp->TestStaticShape(skipGroup3RequireNoFlags, ent1x, ent1z, fixed::Zero(), fixed::FromInt(10), fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(0U, out.size());
		out.clear();

		// ------------------------------------------------------------------------------------

		// In the tests up until this point, the shapes have all been filtered out based on their primary control group.
		// Now, to verify that shapes are also filtered out based on their secondary control groups, add a fourth shape 
		// with arbitrarily-chosen dual control groups, and also change shape 1's secondary control group to another 
		// arbitrarily-chosen control group. Then, do a scene-covering collision test while filtering out a combination 
		// of shape 1's secondary control group, and one of shape 4's control groups. We should find neither ent1 nor ent4
		// in the result.

		entity_id_t ent4 = 4,
		            ent4g1 = 17,
		            ent4g2 = 19,
					ent1g2_new = 18; // new secondary control group for entity 1
		entity_pos_t ent4x = fixed::FromInt(4),
		             ent4z = fixed::Zero(),
		             ent4w = fixed::FromInt(1),
		             ent4h = fixed::FromInt(1);
		entity_angle_t ent4a = fixed::FromDouble(M_PI/3);

		cmp->AddStaticShape(ent4, ent4x, ent4z, ent4a, ent4w, ent4h, ICmpObstructionManager::FLAG_BLOCK_PATHFINDING, ent4g1, ent4g2);
		cmp->SetStaticControlGroup(shape1, ent1g1, ent1g2_new);

		// Exclude shape 1's and shape 4's secondary control groups from testing, and require any available flag to be set.
		// Since neither shape 2 nor shape 3 are part of those control groups and both have at least one available flag set,
		// the results should only those two shapes' entities.

		SkipControlGroupsRequireFlagObstructionFilter skipGroup1SecAnd4SecRequireAny(ent1g2_new, ent4g2,
			(ICmpObstructionManager::flags_t) -1);

		cmp->TestUnitShape(skipGroup1SecAnd4SecRequireAny, ent1x, ent1z, fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		TS_ASSERT_VECTOR_CONTAINS(out, ent3);
		out.clear();

		cmp->TestStaticShape(skipGroup1SecAnd4SecRequireAny, ent1x, ent1z, fixed::Zero(), fixed::FromInt(10), fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		TS_ASSERT_VECTOR_CONTAINS(out, ent3);
		out.clear();

		// Same as the above, but now exclude shape 1's secondary and shape 4's primary control group, while still requiring 
		// any available flag to be set. (Note that the test above used shape 4's secondary control group). Results should 
		// remain the same.
		
		SkipControlGroupsRequireFlagObstructionFilter skipGroup1SecAnd4PrimRequireAny(ent1g2_new, ent4g1,
			(ICmpObstructionManager::flags_t) -1);

		cmp->TestUnitShape(skipGroup1SecAnd4PrimRequireAny, ent1x, ent1z, fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		TS_ASSERT_VECTOR_CONTAINS(out, ent3);
		out.clear();

		cmp->TestStaticShape(skipGroup1SecAnd4PrimRequireAny, ent1x, ent1z, fixed::Zero(), fixed::FromInt(10), fixed::FromInt(10), &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		TS_ASSERT_VECTOR_CONTAINS(out, ent3);
		out.clear();

		cmp->SetStaticControlGroup(shape1, ent1g1, ent1g2); // restore shape 1's original secondary control group
	}

	void test_adjacent_shapes()
	{
		std::vector<entity_id_t> out;
		NullObstructionFilter nullFilter;
		SkipTagObstructionFilter ignoreShape1(shape1);
		SkipTagObstructionFilter ignoreShape2(shape2);
		SkipTagObstructionFilter ignoreShape3(shape3);

		// Collision-test a shape that is perfectly adjacent to shape3. This should be counted as a hit according to
		// the code at the time of writing.
		
		entity_angle_t ent4a = fixed::FromDouble(M_PI); // rotated 180 degrees, should not affect collision test
		entity_pos_t ent4w = fixed::FromInt(2),
		             ent4h = fixed::FromInt(1),
		             ent4x = ent3x + ent3r + ent4w/2, // make ent4 adjacent to ent3
		             ent4z = ent3z;

		cmp->TestStaticShape(nullFilter, ent4x, ent4z, ent4a, ent4w, ent4h, &out);
		TS_ASSERT_EQUALS(1U, out.size());
		TS_ASSERT_EQUALS(ent3, out[0]);
		out.clear();

		cmp->TestUnitShape(nullFilter, ent4x, ent4z, ent4w/2, &out);
		TS_ASSERT_EQUALS(1U, out.size());
		TS_ASSERT_EQUALS(ent3, out[0]);
		out.clear();

		// now do the same tests, but move the shape a little bit to the right so that it doesn't touch anymore

		cmp->TestStaticShape(nullFilter, ent4x + fixed::FromFloat(1e-5f), ent4z, ent4a, ent4w, ent4h, &out);
		TS_ASSERT_EQUALS(0U, out.size());
		out.clear();

		cmp->TestUnitShape(nullFilter, ent4x + fixed::FromFloat(1e-5f), ent4z, ent4w/2, &out);
		TS_ASSERT_EQUALS(0U, out.size());
		out.clear();
	}

	/**
	 * Verifies that fetching the registered shapes from the obstruction manager yields the correct results.
	 */
	void test_get_obstruction()
	{
		ObstructionSquare obSquare1 = cmp->GetObstruction(shape1);
		ObstructionSquare obSquare2 = cmp->GetObstruction(shape2);
		ObstructionSquare obSquare3 = cmp->GetObstruction(shape3);

		TS_ASSERT_EQUALS(obSquare1.hh, ent1h/2);
		TS_ASSERT_EQUALS(obSquare1.hw, ent1w/2);
		TS_ASSERT_EQUALS(obSquare1.x, ent1x);
		TS_ASSERT_EQUALS(obSquare1.z, ent1z);
		TS_ASSERT_EQUALS(obSquare1.u, CFixedVector2D(fixed::FromInt(1), fixed::FromInt(0)));
		TS_ASSERT_EQUALS(obSquare1.v, CFixedVector2D(fixed::FromInt(0), fixed::FromInt(1)));

		TS_ASSERT_EQUALS(obSquare2.hh, ent2r);
		TS_ASSERT_EQUALS(obSquare2.hw, ent2r);
		TS_ASSERT_EQUALS(obSquare2.x, ent2x);
		TS_ASSERT_EQUALS(obSquare2.z, ent2z);
		TS_ASSERT_EQUALS(obSquare2.u, CFixedVector2D(fixed::FromInt(1), fixed::FromInt(0)));
		TS_ASSERT_EQUALS(obSquare2.v, CFixedVector2D(fixed::FromInt(0), fixed::FromInt(1)));

		TS_ASSERT_EQUALS(obSquare3.hh, ent3r);
		TS_ASSERT_EQUALS(obSquare3.hw, ent3r);
		TS_ASSERT_EQUALS(obSquare3.x, ent3x);
		TS_ASSERT_EQUALS(obSquare3.z, ent3z);
		TS_ASSERT_EQUALS(obSquare3.u, CFixedVector2D(fixed::FromInt(1), fixed::FromInt(0)));
		TS_ASSERT_EQUALS(obSquare3.v, CFixedVector2D(fixed::FromInt(0), fixed::FromInt(1)));
	}
};
