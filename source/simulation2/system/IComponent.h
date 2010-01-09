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

#ifndef INCLUDED_ICOMPONENT
#define INCLUDED_ICOMPONENT

#include "Components.h"
#include "Message.h"
#include "Entity.h"

#include "scriptinterface/ScriptTypes.h"

class CParamNode;
class CSimContext;
class CMessage;
class ISerializer;
class IDeserializer;

class IComponent
{
public:
	virtual ~IComponent();

	virtual void Init(const CSimContext& context, const CParamNode& paramNode) = 0;
	virtual void Deinit(const CSimContext& context) = 0;

	virtual void HandleMessage(const CSimContext& context, const CMessage& msg);

	entity_id_t GetEntityId() const { return m_EntityId; }
	void SetEntityId(entity_id_t ent) { m_EntityId = ent; }

	static u8 GetSerializationVersion() { return 0; }
	virtual void Serialize(ISerializer& serialize) = 0;
	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& deserialize) = 0;

	virtual JSClass* GetJSClass() const;
	virtual jsval GetJSInstance() const;

private:
	entity_id_t m_EntityId;
};

#endif // INCLUDED_ICOMPONENT
