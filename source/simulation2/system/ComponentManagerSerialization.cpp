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

#include "precompiled.h"

// Ugly hack: Boost disables rand48's operator<< in VC2005 and older, but we'd quite
// like to use it, so remove the macro that disables it (before we include
// linear_congruential.hpp)
#if MSC_VERSION && MSC_VERSION <= 1400
#undef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
#endif

#include "ComponentManager.h"
#include "IComponent.h"
#include "ParamNode.h"

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
	std::map<ComponentTypeId, std::string> names;

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

	std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::const_iterator cit = m_ComponentsByTypeId.begin();
	for (; cit != m_ComponentsByTypeId.end(); ++cit)
	{
		// Skip component types with no components
		if (cit->second.empty())
			continue;

		// In quick mode, only check unit positions
		if (quick && !(cit->first == CID_Position))
			continue;

		serializer.NumberI32_Unbounded("component type id", cit->first);

		std::map<entity_id_t, IComponent*>::const_iterator eit = cit->second.begin();
		for (; eit != cit->second.end(); ++eit)
		{
			// Don't hash local entities
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
 * Number of (non-empty) component types.
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

	uint32_t numComponentTypes = 0;

	std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::const_iterator cit;

	for (cit = m_ComponentsByTypeId.begin(); cit != m_ComponentsByTypeId.end(); ++cit)
	{
		if (cit->second.empty())
			continue;

		numComponentTypes++;
	}

	serializer.NumberU32_Unbounded("num component types", numComponentTypes);

	for (cit = m_ComponentsByTypeId.begin(); cit != m_ComponentsByTypeId.end(); ++cit)
	{
		if (cit->second.empty())
			continue;

		std::map<ComponentTypeId, ComponentType>::const_iterator ctit = m_ComponentTypesById.find(cit->first);
		if (ctit == m_ComponentTypesById.end())
		{
			debug_warn(L"Invalid ctit"); // this should never happen
			return false;
		}

		serializer.StringASCII("name", ctit->second.name, 0, 255);

		std::map<entity_id_t, IComponent*>::const_iterator eit;

		// Count the components before serializing any of them
		uint32_t numComponents = 0;
		for (eit = cit->second.begin(); eit != cit->second.end(); ++eit)
		{
			// Don't serialize local entities
			if (ENTITY_IS_LOCAL(eit->first))
				continue;

			numComponents++;
		}

		// Emit the count
		serializer.NumberU32_Unbounded("num components", numComponents);

		// Serialize the components now
		for (eit = cit->second.begin(); eit != cit->second.end(); ++eit)
		{
			// Don't serialize local entities
			if (ENTITY_IS_LOCAL(eit->first))
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
	CStdDeserializer deserializer(m_ScriptInterface, stream);

	ResetState();

	std::string rng;
	deserializer.StringASCII("rng", rng, 0, 32);
	DeserializeRNG(rng, m_RNG);

	deserializer.NumberU32_Unbounded("next entity id", m_NextEntityId); // TODO: use sensible bounds

	uint32_t numComponentTypes;
	deserializer.NumberU32_Unbounded("num component types", numComponentTypes);

	ICmpTemplateManager* templateManager = NULL;
	CParamNode noParam;

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
			IComponent* component = ConstructComponent(ent, ctid);
			if (!component)
				return false;

			// Try to find the template for this entity
			const CParamNode* entTemplate = NULL;
			if (templateManager && ent != SYSTEM_ENTITY) // (system entities don't use templates)
				entTemplate = templateManager->LoadLatestTemplate(ent);

			// Deserialize, with the appropriate template for this component
			if (entTemplate)
				component->Deserialize(entTemplate->GetChild(ctname.c_str()), deserializer);
			else
				component->Deserialize(noParam, deserializer);

			// If this was the template manager, remember it so we can use it when
			// deserializing any further non-system entities
			if (ent == SYSTEM_ENTITY && ctid == CID_TemplateManager)
				templateManager = static_cast<ICmpTemplateManager*> (component);
		}
	}

	if (stream.peek() != EOF)
	{
		LOGERROR(L"Deserialization didn't reach EOF");
		return false;
	}

	// TODO: catch exceptions
	return true;
}
