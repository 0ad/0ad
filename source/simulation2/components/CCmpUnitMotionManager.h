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
#include "simulation2/system/EntityMap.h"

class CCmpUnitMotion;

class CCmpUnitMotionManager : public ICmpUnitMotionManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_TurnStart);
		componentManager.SubscribeToMessageType(MT_Update_Final);
		componentManager.SubscribeToMessageType(MT_Update_MotionUnit);
		componentManager.SubscribeToMessageType(MT_Update_MotionFormation);
	}

	DEFAULT_COMPONENT_ALLOCATOR(UnitMotionManager)

	// Persisted state for each unit.
	struct MotionState
	{
		// Component references - these must be kept alive for the duration of motion.
		// NB: this is generally not something one should do, but because of the tight coupling here it's doable.
		CmpPtr<ICmpPosition> cmpPosition;
		CCmpUnitMotion* cmpUnitMotion;

		// Position before units start moving
		CFixedVector2D initialPos;
		// Transient position during the movement.
		CFixedVector2D pos;

		fixed initialAngle;
		fixed angle;

		// If true, the entity needs to be handled during movement.
		bool needUpdate;

		// 'Leak' from UnitMotion.
		bool wentStraight;
		bool wasObstructed;
	};

	EntityMap<MotionState> m_Units;
	EntityMap<MotionState> m_FormationControllers;

	// Temporary vector, reconstructed each turn (stored here to avoid memory reallocations).
	std::vector<EntityMap<MotionState>::iterator> m_MovingUnits;

	bool m_ComputingMotion;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_MovingUnits.reserve(40);
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
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

	void OnTurnStart();

	void MoveUnits(fixed dt);
	void MoveFormations(fixed dt);
	void Move(EntityMap<MotionState>& ents, fixed dt);
};

REGISTER_COMPONENT_TYPE(UnitMotionManager)

#endif // INCLUDED_CCMPUNITMOTIONMANAGER
