/* Copyright (C) 2022 Wildfire Games.
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

#ifndef INCLUDED_CCMPUNITMOTIONMANAGER
#define INCLUDED_CCMPUNITMOTIONMANAGER

#include "simulation2/system/Component.h"
#include "ICmpUnitMotionManager.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/system/EntityMap.h"

class CCmpUnitMotion;

class CCmpUnitMotionManager final : public ICmpUnitMotionManager
{
public:
	static void ClassInit(CComponentManager& componentManager);

	DEFAULT_COMPONENT_ALLOCATOR(UnitMotionManager)

	/**
	 * Maximum value for pushing pressure.
	 */
	static constexpr int MAX_PRESSURE = 255;

	// Persisted state for each unit.
	struct MotionState
	{
		MotionState(ICmpPosition* cmpPos, CCmpUnitMotion* cmpMotion);

		// Component references - these must be kept alive for the duration of motion.
		// NB: this is generally a super dangerous thing to do,
		// but the tight coupling with CCmpUnitMotion makes it workable.
		// NB: this assumes that components do _not_ move in memory,
		// which is currently a fair assumption but might change in the future.
		ICmpPosition* cmpPosition;
		CCmpUnitMotion* cmpUnitMotion;

		// Position before units start moving
		CFixedVector2D initialPos;
		// Transient position during the movement.
		CFixedVector2D pos;

		// Accumulated "pushing" from nearby units.
		CFixedVector2D push;

		fixed speed;

		fixed initialAngle;
		fixed angle;

		// Used for formations - units with the same control group won't push at a distance.
		// (this is required because formations may be tight and large units may end up never settling.
		entity_id_t controlGroup = INVALID_ENTITY;

		// This is a ad-hoc counter to store under how much pushing 'pressure' an entity is.
		// More pressure will slow the unit down and make it harder to push,
		// which effectively bogs down groups of colliding units.
		uint8_t pushingPressure = 0;

		// Meta-flag -> this entity won't push nor be pushed.
		// (used for entities that have their obstruction disabled).
		bool ignore = false;

		// If true, the entity needs to be handled during movement.
		bool needUpdate = false;

		bool wentStraight = false;
		bool wasObstructed = false;

		// Clone of the obstruction manager flag for efficiency
		bool isMoving = false;
	};

	// "Template" state, not serialized (cannot be changed mid-game).

	// The maximal distance at which units push each other is the combined unit clearances, multipled by this factor,
	// itself pre-multiplied by the circle-square correction factor.
	entity_pos_t m_PushingRadiusMultiplier;
	// Additive modifiers to the maximum pushing distance for moving units and idle units respectively.
	entity_pos_t m_MovingPushExtension;
	entity_pos_t m_StaticPushExtension;
	// Multiplier for the pushing 'spread'.
	// This should be understand as the % of the maximum distance where pushing will be "in full force".
	entity_pos_t m_MovingPushingSpread;
	entity_pos_t m_StaticPushingSpread;

	// Pushing forces below this value are ignored - this prevents units moving forever by very small increments.
	entity_pos_t m_MinimalPushing;

	// Multiplier for pushing pressure strength.
	entity_pos_t m_PushingPressureStrength;
	// Per-turn reduction in pushing pressure.
	entity_pos_t m_PushingPressureDecay;

	// These vectors are reconstructed on deserialization.

	EntityMap<MotionState> m_Units;
	EntityMap<MotionState> m_FormationControllers;

	// Turn-local state below, not serialised.

	Grid<std::vector<EntityMap<MotionState>::iterator>> m_MovingUnits;
	bool m_ComputingMotion;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	void Init(const CParamNode& UNUSED(paramNode)) override;

	void Deinit() override
	{
	}

	void Serialize(ISerializer& serialize) override;
	void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize) override;

	void HandleMessage(const CMessage& msg, bool global) override;

	void Register(CCmpUnitMotion* component, entity_id_t ent, bool formationController) override;
	void Unregister(entity_id_t ent) override;

	bool ComputingMotion() const override
	{
		return m_ComputingMotion;
	}

	bool IsPushingActivated() const override
	{
		return m_PushingRadiusMultiplier != entity_pos_t::Zero();
	}

private:
	void OnDeserialized();
	void ResetSubdivisions();
	void OnTurnStart();

	void MoveUnits(fixed dt);
	void MoveFormations(fixed dt);
	void Move(EntityMap<MotionState>& ents, fixed dt);

	void Push(EntityMap<MotionState>::value_type& a, EntityMap<MotionState>::value_type& b, fixed dt);
};

REGISTER_COMPONENT_TYPE(UnitMotionManager)

#endif // INCLUDED_CCMPUNITMOTIONMANAGER
