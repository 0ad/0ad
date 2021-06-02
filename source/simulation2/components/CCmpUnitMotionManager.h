/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_CCMPUNITMOTIONMANAGER
#define INCLUDED_CCMPUNITMOTIONMANAGER

#include "simulation2/system/Component.h"
#include "ICmpUnitMotionManager.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/system/EntityMap.h"

class CCmpUnitMotion;

class CCmpUnitMotionManager : public ICmpUnitMotionManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_TerrainChanged);
		componentManager.SubscribeToMessageType(MT_TurnStart);
		componentManager.SubscribeToMessageType(MT_Update_Final);
		componentManager.SubscribeToMessageType(MT_Update_MotionUnit);
		componentManager.SubscribeToMessageType(MT_Update_MotionFormation);
	}

	DEFAULT_COMPONENT_ALLOCATOR(UnitMotionManager)

	// Persisted state for each unit.
	struct MotionState
	{
		MotionState(CmpPtr<ICmpPosition> cmpPos, CCmpUnitMotion* cmpMotion);

		// Component references - these must be kept alive for the duration of motion.
		// NB: this is generally not something one should do, but because of the tight coupling here it's doable.
		CmpPtr<ICmpPosition> cmpPosition;
		CCmpUnitMotion* cmpUnitMotion;

		// Position before units start moving
		CFixedVector2D initialPos;
		// Transient position during the movement.
		CFixedVector2D pos;

		// Accumulated "pushing" from nearby units.
		CFixedVector2D push;

		fixed initialAngle;
		fixed angle;

		// Used for formations - units with the same control group won't push at a distance.
		// (this is required because formations may be tight and large units may end up never settling.
		entity_id_t controlGroup = INVALID_ENTITY;

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

	// Multiplier for the pushing radius. Pre-multiplied by the circle-square correction factor.
	// "Template" state, not serialized (cannot be changed mid-game).
	entity_pos_t m_PushingRadius;

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

	virtual void Init(const CParamNode& UNUSED(paramNode));

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
		ResetSubdivisions();
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
			case MT_TerrainChanged:
			{
				CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
				if (cmpTerrain->GetVerticesPerSide() != m_MovingUnits.width())
					ResetSubdivisions();
				break;
			}
			case MT_TurnStart:
			{
				OnTurnStart();
				break;
			}
			case MT_Update_MotionFormation:
			{
				fixed dt = static_cast<const CMessageUpdate_MotionFormation&>(msg).turnLength;
				m_ComputingMotion = true;
				MoveFormations(dt);
				m_ComputingMotion = false;
				break;
			}
			case MT_Update_MotionUnit:
			{
				fixed dt = static_cast<const CMessageUpdate_MotionUnit&>(msg).turnLength;
				m_ComputingMotion = true;
				MoveUnits(dt);
				m_ComputingMotion = false;
				break;
			}
		}
	}

	virtual void Register(CCmpUnitMotion* component, entity_id_t ent, bool formationController);
	virtual void Unregister(entity_id_t ent);

	virtual bool ComputingMotion() const
	{
		return m_ComputingMotion;
	}

	virtual bool IsPushingActivated() const
	{
		return m_PushingRadius != entity_pos_t::Zero();
	}

private:
	void ResetSubdivisions();
	void OnTurnStart();

	void MoveUnits(fixed dt);
	void MoveFormations(fixed dt);
	void Move(EntityMap<MotionState>& ents, fixed dt);

	void Push(EntityMap<MotionState>::value_type& a, EntityMap<MotionState>::value_type& b, fixed dt);
};

void CCmpUnitMotionManager::ResetSubdivisions()
{
	CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
	if (!cmpTerrain)
		return;

	size_t size = cmpTerrain->GetMapSize();
	u16 gridSquareSize = static_cast<u16>(size / 20 + 1);
	m_MovingUnits.resize(gridSquareSize, gridSquareSize);
}

REGISTER_COMPONENT_TYPE(UnitMotionManager)

#endif // INCLUDED_CCMPUNITMOTIONMANAGER
