/* Copyright (C) 2013 Wildfire Games.
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

#include "ComponentManager.h"
#include "IComponent.h"
#include "ParamNode.h"

#include "simulation2/MessageTypes.h"

#include "simulation2/serialization/DebugSerializer.h"
#include "simulation2/serialization/HashSerializer.h"
#include "simulation2/serialization/StdSerializer.h"
#include "simulation2/serialization/StdDeserializer.h"

#include "simulation2/components/ICmpTemplateManager.h"

#include "ps/CLogger.h"

std::string SerializeRNG(const boost::rand48& rng)
{
	std::stringstream s;
	s << rng;
	return s.str();
}

void DeserializeRNG(const std::string& str, boost::rand48& rng)
{
	std::stringstream s;
	s << str;
	s >> rng;
}

bool CComponentManager::DumpDebugState(std::ostream& stream, bool includeDebugInfo)
{
	CDebugSerializer serializer(m_ScriptInterface, stream, includeDebugInfo);

	serializer.StringASCII("rng", SerializeRNG(m_RNG), 0, 32);

	serializer.TextLine("entities:");

	// We want the output to be grouped by entity ID, so invert the CComponentManager data structures
	std::map<entity_id_t, std::map<ComponentTypeId, IComponent*> > components;
	//std::map<ComponentTypeId, std::string> names;

	std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::const_iterator ctit = m_ComponentsByTypeId.begin();
	for (; ctit != m_ComponentsByTypeId.end(); ++ctit)
	{
		std::map<entity_id_t, IComponent*>::const_iterator eit = ctit->second.begin();
		for (; eit != ctit->second.end(); ++eit)
		{
			components[eit->first][ctit->first] = eit->second;
		}
	}

	std::map<entity_id_t, std::map<ComponentTypeId, IComponent*> >::const_iterator cit = components.begin();
	for (; cit != components.end(); ++cit)
	{
		std::stringstream n;
		n << "- id: " << cit->first;
		serializer.TextLine(n.str());

		if (ENTITY_IS_LOCAL(cit->first))
			serializer.TextLine("  type: local");

		std::map<ComponentTypeId, IComponent*>::const_iterator ctit = cit->second.begin();
		for (; ctit != cit->second.end(); ++ctit)
		{
			std::stringstream n;
			n << "  " << LookupComponentTypeName(ctit->first) << ":";
			serializer.TextLine(n.str());
			serializer.Indent(4);
			ctit->second->Serialize(serializer);
			serializer.Dedent(4);
		}
		serializer.TextLine("");
	}

	// TODO: catch exceptions
	return true;
}

bool CComponentManager::ComputeStateHash(std::string& outHash, bool quick)
{
	// Hash serialization: this includes the minimal data necessary to detect
	// differences in the state, and ignores things like counts and names

	// If 'quick' is set, this checks even fewer things, so that it will
	// be fast enough to run every turn but will typically detect any
	// out-of-syncs fairly soon

	CHashSerializer serializer(m_ScriptInterface);

	serializer.StringASCII("rng", SerializeRNG(m_RNG), 0, 32);
	serializer.NumberU32_Unbounded("next entity id", m_NextEntityId);

	std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::const_iterator cit = m_ComponentsByTypeId.begin();
	for (; cit != m_ComponentsByTypeId.end(); ++cit)
	{
		// In quick mode, only check unit positions
		if (quick && !(cit->first == CID_Position))
			continue;

		// Only emit component types if they have a component that will be serialized
		bool needsSerialization = false;
		for (std::map<entity_id_t, IComponent*>::const_iterator eit = cit->second.begin(); eit != cit->second.end(); ++eit)
		{
			// Don't serialize local entities
			if (ENTITY_IS_LOCAL(eit->first))
				continue;

			needsSerialization = true;
			break;
		}

		if (!needsSerialization)
			continue;

		serializer.NumberI32_Unbounded("component type id", cit->first);

		for (std::map<entity_id_t, IComponent*>::const_iterator eit = cit->second.begin(); eit != cit->second.end(); ++eit)
		{
			// Don't serialize local entities
			if (ENTITY_IS_LOCAL(eit->first))
				continue;

			serializer.NumberU32_Unbounded("entity id", eit->first);
			eit->second->Serialize(serializer);
		}
	}

	outHash = std::string((const char*)serializer.ComputeHash(), serializer.GetHashLength());

	// TODO: catch exceptions
	return true;
}

/*
 * Simulation state serialization format:
 *
 * TODO: Global version number.
 * Number of SYSTEM_ENTITY component types
 * For each SYSTEM_ENTITY component type:
 *   Component type name
 *   TODO: Component type version number.
 *   Component state.
 * Number of (non-empty, non-SYSTEM_ENTITY-only) component types.
 * For each component type:
 *   Component type name.
 *   TODO: Component type version number.
 *   Number of entities.
 *   For each entity:
 *     Entity id.
 *     Component state.
 *
 * Rationale:
 * Saved games should be valid across patches, which might change component
 * type IDs. Thus the names are serialized, not the IDs.
 * Version numbers are used so saved games from future versions can be rejected,
 * and those from older versions can be fixed up to work with the latest version.
 * (These aren't really needed for networked games (where everyone will have the same
 * version), but it doesn't seem worth having a separate codepath for that.)
 */

bool CComponentManager::SerializeState(std::ostream& stream)
{
	CStdSerializer serializer(m_ScriptInterface, stream);

	// We don't serialize the destruction queue, since we'd have to be careful to skip local entities etc
	// and it's (hopefully) easier to just expect callers to flush the queue before serializing
	ENSURE(m_DestructionQueue.empty());

	serializer.StringASCII("rng", SerializeRNG(m_RNG), 0, 32);
	serializer.NumberU32_Unbounded("next entity id", m_NextEntityId);

	std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::const_iterator cit;
	
	uint32_t numSystemComponentTypes = 0;
	uint32_t numComponentTypes = 0;
	std::set<ComponentTypeId> serializedSystemComponentTypes;
	std::set<ComponentTypeId> serializedComponentTypes;

	for (cit = m_ComponentsByTypeId.begin(); cit != m_ComponentsByTypeId.end(); ++cit)
	{
		// Only emit component types if they have a component that will be serialized
		bool needsSerialization = false;
		for (std::map<entity_id_t, IComponent*>::const_iterator eit = cit->second.begin(); eit != cit->second.end(); ++eit)
		{
			// Don't serialize local entities, and handle SYSTEM_ENTITY separately
			if (ENTITY_IS_LOCAL(eit->first) || eit->first == SYSTEM_ENTITY)
				continue;

			needsSerialization = true;
			break;
		}

		if (needsSerialization)
		{
			numComponentTypes++;
			serializedComponentTypes.insert(cit->first);
		}

		if (cit->second.find(SYSTEM_ENTITY) != cit->second.end())
		{
			numSystemComponentTypes++;
			serializedSystemComponentTypes.insert(cit->first);
		}
	}

	serializer.NumberU32_Unbounded("num system component types", numSystemComponentTypes);

	for (cit = m_ComponentsByTypeId.begin(); cit != m_ComponentsByTypeId.end(); ++cit)
	{
		if (serializedSystemComponentTypes.find(cit->first) == serializedSystemComponentTypes.end())
			continue;

		std::map<ComponentTypeId, ComponentType>::const_iterator ctit = m_ComponentTypesById.find(cit->first);
		if (ctit == m_ComponentTypesById.end())
		{
			debug_warn(L"Invalid ctit"); // this should never happen
			return false;
		}

		serializer.StringASCII("name", ctit->second.name, 0, 255);

		std::map<entity_id_t, IComponent*>::const_iterator eit = cit->second.find(SYSTEM_ENTITY);
		if (eit == cit->second.end())
		{
			debug_warn(L"Invalid eit"); // this should never happen
			return false;
		}
		eit->second->Serialize(serializer);
	}

	serializer.NumberU32_Unbounded("num component types", numComponentTypes);

	for (cit = m_ComponentsByTypeId.begin(); cit != m_ComponentsByTypeId.end(); ++cit)
	{
		if (serializedComponentTypes.find(cit->first) == serializedComponentTypes.end())
			continue;

		std::map<ComponentTypeId, ComponentType>::const_iterator ctit = m_ComponentTypesById.find(cit->first);
		if (ctit == m_ComponentTypesById.end())
		{
			debug_warn(L"Invalid ctit"); // this should never happen
			return false;
		}

		serializer.StringASCII("name", ctit->second.name, 0, 255);

		// Count the components before serializing any of them
		uint32_t numComponents = 0;
		for (std::map<entity_id_t, IComponent*>::const_iterator eit = cit->second.begin(); eit != cit->second.end(); ++eit)
		{
			// Don't serialize local entities or SYSTEM_ENTITY
			if (ENTITY_IS_LOCAL(eit->first) || eit->first == SYSTEM_ENTITY)
				continue;

			numComponents++;
		}

		// Emit the count
		serializer.NumberU32_Unbounded("num components", numComponents);

		// Serialize the components now
		for (std::map<entity_id_t, IComponent*>::const_iterator eit = cit->second.begin(); eit != cit->second.end(); ++eit)
		{
			// Don't serialize local entities or SYSTEM_ENTITY
			if (ENTITY_IS_LOCAL(eit->first) || eit->first == SYSTEM_ENTITY)
				continue;

			serializer.NumberU32_Unbounded("entity id", eit->first);
			eit->second->Serialize(serializer);
		}
	}

	// TODO: catch exceptions
	return true;
}

bool CComponentManager::DeserializeState(std::istream& stream)
{
	try
	{
		CStdDeserializer deserializer(m_ScriptInterface, stream);

		ResetState();
		InitSystemEntity();

		std::string rng;
		deserializer.StringASCII("rng", rng, 0, 32);
		DeserializeRNG(rng, m_RNG);

		deserializer.NumberU32_Unbounded("next entity id", m_NextEntityId); // TODO: use sensible bounds

		uint32_t numSystemComponentTypes;
		deserializer.NumberU32_Unbounded("num system component types", numSystemComponentTypes);

		ICmpTemplateManager* templateManager = NULL;
		CParamNode noParam;

		for (size_t i = 0; i < numSystemComponentTypes; ++i)
		{
			std::string ctname;
			deserializer.StringASCII("name", ctname, 0, 255);

			ComponentTypeId ctid = LookupCID(ctname);
			if (ctid == CID__Invalid)
			{
				LOGERROR(L"Deserialization saw unrecognised component type '%hs'", ctname.c_str());
				return false;
			}

			IComponent* component = ConstructComponent(m_SystemEntity, ctid);
			if (!component)
				return false;

			component->Deserialize(noParam, deserializer);

			// If this was the template manager, remember it so we can use it when
			// deserializing any further non-system entities
			if (ctid == CID_TemplateManager)
				templateManager = static_cast<ICmpTemplateManager*> (component);
		}

		uint32_t numComponentTypes;
		deserializer.NumberU32_Unbounded("num component types", numComponentTypes);

		for (size_t i = 0; i < numComponentTypes; ++i)
		{
			std::string ctname;
			deserializer.StringASCII("name", ctname, 0, 255);

			ComponentTypeId ctid = LookupCID(ctname);
			if (ctid == CID__Invalid)
			{
				LOGERROR(L"Deserialization saw unrecognised component type '%hs'", ctname.c_str());
				return false;
			}

			uint32_t numComponents;
			deserializer.NumberU32_Unbounded("num components", numComponents);

			for (size_t j = 0; j < numComponents; ++j)
			{
				entity_id_t ent;
				deserializer.NumberU32_Unbounded("entity id", ent);
				IComponent* component = ConstructComponent(LookupEntityHandle(ent, true), ctid);
				if (!component)
					return false;

				// Try to find the template for this entity
				const CParamNode* entTemplate = NULL;
				if (templateManager)
					entTemplate = templateManager->LoadLatestTemplate(ent);

				// Deserialize, with the appropriate template for this component
				if (entTemplate)
					component->Deserialize(entTemplate->GetChild(ctname.c_str()), deserializer);
				else
					component->Deserialize(noParam, deserializer);
			}
		}

		if (stream.peek() != EOF)
		{
			LOGERROR(L"Deserialization didn't reach EOF");
			return false;
		}

		// Allow components to do some final reinitialisation after everything is loaded
		CMessageDeserialized msg;
		BroadcastMessage(msg);

		return true;
	}
	catch (PSERROR_Deserialize& e)
	{
		LOGERROR(L"Deserialization failed: %hs", e.what());
		return false;
	}
}
