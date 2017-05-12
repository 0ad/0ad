/* Copyright (C) 2017 Wildfire Games.
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

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpOwnership.h"

#include "simulation2/MessageTypes.h"

/**
 * Basic ICmpOwnership implementation.
 */
class CCmpOwnership : public ICmpOwnership
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Destroy);
	}

	DEFAULT_COMPONENT_ALLOCATOR(Ownership)

	player_id_t m_Owner;

	static std::string GetSchema()
	{
		return
			"<a:example/>"
			"<a:help>Allows this entity to be owned by players.</a:help>"
			"<empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_Owner = INVALID_PLAYER;
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		serialize.NumberI32_Unbounded("owner", m_Owner);
	}

	virtual void Deserialize(const CParamNode& UNUSED(paramNode), IDeserializer& deserialize)
	{
		deserialize.NumberI32_Unbounded("owner", m_Owner);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Destroy:
		{
			// Reset the owner so this entity is e.g. removed from population counts
			SetOwner(INVALID_PLAYER);
			break;
		}
		}
	}

	virtual player_id_t GetOwner() const
	{
		return m_Owner;
	}

	virtual void SetOwner(player_id_t playerID)
	{
		if (playerID == m_Owner)
			return;

		player_id_t old = m_Owner;
		m_Owner = playerID;

		CMessageOwnershipChanged msg(GetEntityId(), old, playerID);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	virtual void SetOwnerQuiet(player_id_t playerID)
	{
		if (playerID != m_Owner)
			m_Owner = playerID;
	}
};

REGISTER_COMPONENT_TYPE(Ownership)
