/* Copyright (C) 2020 Wildfire Games.
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
#include "simulation2/components/ICmpObstruction.h"

class MockObstruction : public ICmpObstruction
{
public:
	DEFAULT_MOCK_COMPONENT()
	ICmpObstructionManager::ObstructionSquare obstruction;

	virtual ICmpObstructionManager::tag_t GetObstruction() const { return ICmpObstructionManager::tag_t(); }
	virtual bool GetObstructionSquare(ICmpObstructionManager::ObstructionSquare& out) const { out = obstruction; return true; }
	virtual bool GetPreviousObstructionSquare(ICmpObstructionManager::ObstructionSquare& UNUSED(out)) const { return true; }
	virtual entity_pos_t GetSize() const { return entity_pos_t::Zero(); }
	virtual CFixedVector2D GetStaticSize() const { return CFixedVector2D(); }
	virtual EObstructionType GetObstructionType() const { return ICmpObstruction::STATIC; }
	virtual void SetUnitClearance(const entity_pos_t& UNUSED(clearance)) { }
	virtual bool IsControlPersistent() const { return true; }
	virtual bool CheckShorePlacement() const { return true; }
	virtual EFoundationCheck CheckFoundation(const std::string& UNUSED(className)) const { return EFoundationCheck(); }
	virtual EFoundationCheck CheckFoundation(const std::string& UNUSED(className), bool UNUSED(onlyCenterPoint)) const { return EFoundationCheck(); }
	virtual std::string CheckFoundation_wrapper(const std::string& UNUSED(className), bool UNUSED(onlyCenterPoint)) const { return std::string(); }
	virtual bool CheckDuplicateFoundation() const { return true; }
	virtual std::vector<entity_id_t> GetEntitiesByFlags(ICmpObstructionManager::flags_t UNUSED(flags)) const { return std::vector<entity_id_t>(); }
	virtual std::vector<entity_id_t> GetEntitiesBlockingMovement() const { return std::vector<entity_id_t>(); }
	virtual std::vector<entity_id_t> GetEntitiesBlockingConstruction() const { return std::vector<entity_id_t>(); }
	virtual std::vector<entity_id_t> GetEntitiesDeletedUponConstruction() const { return std::vector<entity_id_t>(); }
	virtual void ResolveFoundationCollisions() const { }
	virtual void SetActive(bool UNUSED(active)) { }
	virtual void SetMovingFlag(bool UNUSED(enabled)) { }
	virtual void SetDisableBlockMovementPathfinding(bool UNUSED(movementDisabled), bool UNUSED(pathfindingDisabled), int32_t UNUSED(shape)) { }
	virtual bool GetBlockMovementFlag() const { return true; }
	virtual void SetControlGroup(entity_id_t UNUSED(group)) { }
	virtual entity_id_t GetControlGroup() const { return INVALID_ENTITY; }
	virtual void SetControlGroup2(entity_id_t UNUSED(group2)) { }
	virtual entity_id_t GetControlGroup2() const { return INVALID_ENTITY; }
};

class TestCmpObstructionManager : public CxxTest::TestSuite
{
	typedef ICmpObstructionManager::tag_t tag_t;
	typedef ICmpObstructionManager::ObstructionSquare ObstructionSquare;

	// some variables for setting up a scene with 3 shapes
	entity_id_t ent1, ent2, ent3; // entity IDs
	entity_angle_t ent1a; // angles
	entity_pos_t ent1x, ent1z, ent1w, ent1h, // positions/dimensions
	             ent2x, ent2z, ent2c,
	             ent3x, ent3z, ent3c;
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
		ent2c = fixed::FromFloat(1);
		ent2x = ent1x;
		ent2z = ent1z;
		ent2g = ent1g1;

		ent3 = 3;
		ent3c = fixed::FromFloat(3);
		ent3x = ent2x;
		ent3z = ent2z + ent2c + ent3c; // ensure it just touches the border of ent2
		ent3g = ent3;

		testHelper = new ComponentTestHelper(g_ScriptContext);
		cmp = testHelper->Add<ICmpObstructionManager>(CID_ObstructionManager, "", SYSTEM_ENTITY);
		cmp->SetBounds(fixed::FromInt(0), fixed::FromInt(0), fixed::FromInt(1000), fixed::FromInt(1000));

		shape1 = cmp->AddStaticShape(ent1, ent1x, ent1z, ent1a, ent1w, ent1h,
			ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION |
			ICmpObstructionManager::FLAG_BLOCK_MOVEMENT |
			ICmpObstructionManager::FLAG_MOVING, ent1g1, ent1g2);

		shape2 = cmp->AddUnitShape(ent2, ent2x, ent2z, ent2c,
			ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION |
			ICmpObstructionManager::FLAG_BLOCK_FOUNDATION, ent2g);

		shape3 = cmp->AddUnitShape(ent3, ent3x, ent3z, ent3c,
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

		cmp->TestUnitShape(nullFilter, ent2x, ent2z, ent2c/2, &out);
		TS_ASSERT_EQUALS(2U, out.size());
		TS_ASSERT_VECTOR_CONTAINS(out, ent1);
		TS_ASSERT_VECTOR_CONTAINS(out, ent2);
		out.clear();

		cmp->TestStaticShape(nullFilter, ent2x, ent2z, fixed::Zero(), ent2c, ent2c, &out);
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

		cmp->TestUnitShape(ignoreShape2, ent2x, ent2z, ent2c/2, &out);
		TS_ASSERT_EQUALS(1U, out.size());
		TS_ASSERT_EQUALS(ent1, out[0]);
		out.clear();

		cmp->TestStaticShape(ignoreShape2, ent2x, ent2z, fixed::Zero(), ent2c, ent2c, &out);
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
		             ent4x = ent3x + ent3c + ent4w/2, // make ent4 adjacent to ent3
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

		TS_ASSERT_EQUALS(obSquare2.hh, ent2c);
		TS_ASSERT_EQUALS(obSquare2.hw, ent2c);
		TS_ASSERT_EQUALS(obSquare2.x, ent2x);
		TS_ASSERT_EQUALS(obSquare2.z, ent2z);
		TS_ASSERT_EQUALS(obSquare2.u, CFixedVector2D(fixed::FromInt(1), fixed::FromInt(0)));
		TS_ASSERT_EQUALS(obSquare2.v, CFixedVector2D(fixed::FromInt(0), fixed::FromInt(1)));

		TS_ASSERT_EQUALS(obSquare3.hh, ent3c);
		TS_ASSERT_EQUALS(obSquare3.hw, ent3c);
		TS_ASSERT_EQUALS(obSquare3.x, ent3x);
		TS_ASSERT_EQUALS(obSquare3.z, ent3z);
		TS_ASSERT_EQUALS(obSquare3.u, CFixedVector2D(fixed::FromInt(1), fixed::FromInt(0)));
		TS_ASSERT_EQUALS(obSquare3.v, CFixedVector2D(fixed::FromInt(0), fixed::FromInt(1)));
	}

	/**
	 * Verifies the calculations of distances between shapes.
	 */
	void test_distance_to()
	{
		// Create two more entities to have non-zero distances
		entity_id_t ent4 = 4,
		            ent4g1 = ent4,
		            ent4g2 = INVALID_ENTITY,
		            ent5 = 5,
		            ent5g1 = ent5,
		            ent5g2 = INVALID_ENTITY;

		entity_pos_t ent4a = fixed::Zero(),
		             ent4w = fixed::FromInt(6),
		             ent4h = fixed::Zero(),
		             ent4x = ent1x,
		             ent4z = fixed::FromInt(20),
		             ent5a = fixed::Zero(),
		             ent5w = fixed::FromInt(2),
		             ent5h = fixed::FromInt(4),
		             ent5x = fixed::FromInt(20),
		             ent5z = ent1z;

		tag_t shape4 = cmp->AddStaticShape(ent4, ent4x, ent4z, ent4a, ent4w, ent4h,
			ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION |
			ICmpObstructionManager::FLAG_BLOCK_MOVEMENT |
			ICmpObstructionManager::FLAG_MOVING, ent4g1, ent4g2);

		tag_t shape5 = cmp->AddStaticShape(ent5, ent5x, ent5z, ent5a, ent5w, ent5h,
			ICmpObstructionManager::FLAG_BLOCK_CONSTRUCTION |
			ICmpObstructionManager::FLAG_BLOCK_MOVEMENT |
			ICmpObstructionManager::FLAG_MOVING, ent5g1, ent5g2);

		MockObstruction obstruction1, obstruction2, obstruction3, obstruction4, obstruction5;
		testHelper->AddMock(ent1, IID_Obstruction, obstruction1);
		testHelper->AddMock(ent2, IID_Obstruction, obstruction2);
		testHelper->AddMock(ent3, IID_Obstruction, obstruction3);
		testHelper->AddMock(ent4, IID_Obstruction, obstruction4);
		testHelper->AddMock(ent5, IID_Obstruction, obstruction5);
		obstruction1.obstruction = cmp->GetObstruction(shape1);
		obstruction2.obstruction = cmp->GetObstruction(shape2);
		obstruction3.obstruction = cmp->GetObstruction(shape3);
		obstruction4.obstruction = cmp->GetObstruction(shape4);
		obstruction5.obstruction = cmp->GetObstruction(shape5);

		TS_ASSERT_EQUALS(fixed::Zero(), cmp->DistanceToTarget(ent1, ent2));
		TS_ASSERT_EQUALS(fixed::Zero(), cmp->DistanceToTarget(ent2, ent1));
		TS_ASSERT_EQUALS(fixed::Zero(), cmp->DistanceToTarget(ent2, ent3));
		TS_ASSERT_EQUALS(fixed::Zero(), cmp->DistanceToTarget(ent3, ent2));

		// Due to rounding errors we need to use some leeway
		TS_ASSERT_DELTA(fixed::FromFloat(std::sqrt(80)), cmp->MaxDistanceToTarget(ent2, ent3), fixed::FromFloat(0.0001));
		TS_ASSERT_DELTA(fixed::FromFloat(std::sqrt(80)), cmp->MaxDistanceToTarget(ent3, ent2), fixed::FromFloat(0.0001));

		TS_ASSERT_EQUALS(fixed::Zero(), cmp->DistanceToTarget(ent1, ent3));
		TS_ASSERT_EQUALS(fixed::Zero(), cmp->DistanceToTarget(ent3, ent1));

		TS_ASSERT_EQUALS(fixed::FromInt(6), cmp->DistanceToTarget(ent1, ent4));
		TS_ASSERT_EQUALS(fixed::FromInt(6), cmp->DistanceToTarget(ent4, ent1));
		TS_ASSERT_DELTA(fixed::FromFloat(std::sqrt(125) + 3), cmp->MaxDistanceToTarget(ent1, ent4), fixed::FromFloat(0.0001));
		TS_ASSERT_DELTA(fixed::FromFloat(std::sqrt(125) + 3), cmp->MaxDistanceToTarget(ent4, ent1), fixed::FromFloat(0.0001));

		TS_ASSERT_EQUALS(fixed::FromInt(7), cmp->DistanceToTarget(ent1, ent5));
		TS_ASSERT_EQUALS(fixed::FromInt(7), cmp->DistanceToTarget(ent5, ent1));
		TS_ASSERT_DELTA(fixed::FromFloat(std::sqrt(178)), cmp->MaxDistanceToTarget(ent1, ent5), fixed::FromFloat(0.0001));
		TS_ASSERT_DELTA(fixed::FromFloat(std::sqrt(178)), cmp->MaxDistanceToTarget(ent5, ent1), fixed::FromFloat(0.0001));

		TS_ASSERT(cmp->IsInTargetRange(ent1, ent2, fixed::Zero(), fixed::FromInt(1), true));
		TS_ASSERT(cmp->IsInTargetRange(ent1, ent2, fixed::Zero(), fixed::FromInt(1), false));
		TS_ASSERT(cmp->IsInTargetRange(ent1, ent2, fixed::FromInt(1), fixed::FromInt(1), true));
		TS_ASSERT(!cmp->IsInTargetRange(ent1, ent2, fixed::FromInt(1), fixed::FromInt(1), false));

		TS_ASSERT(cmp->IsInTargetRange(ent1, ent5, fixed::Zero(), fixed::FromInt(10), true));
		TS_ASSERT(cmp->IsInTargetRange(ent1, ent5, fixed::Zero(), fixed::FromInt(10), false));
		TS_ASSERT(cmp->IsInTargetRange(ent1, ent5, fixed::FromInt(1), fixed::FromInt(10), true));
		TS_ASSERT(!cmp->IsInTargetRange(ent1, ent5, fixed::FromInt(1), fixed::FromInt(5), false));
		TS_ASSERT(!cmp->IsInTargetRange(ent1, ent5, fixed::FromInt(10), fixed::FromInt(10), false));
		TS_ASSERT(cmp->IsInTargetRange(ent1, ent5, fixed::FromInt(10), fixed::FromInt(10), true));
	}
};
