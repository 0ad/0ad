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

#ifndef INCLUDED_COMPONENTMANAGER
#define INCLUDED_COMPONENTMANAGER

#include "Entity.h"
#include "Components.h"
#include "scriptinterface/ScriptInterface.h"

#include <map>

class IComponent;
class CParamNode;
class CMessage;
class CSimContext;

class CComponentManager
{
	NONCOPYABLE(CComponentManager);
public:
	// We can't use EInterfaceId/etc directly, since scripts dynamically generate new IDs
	// and casting arbitrary ints to enums is undefined behaviour, so use 'int' typedefs
	typedef int InterfaceId;
	typedef int ComponentTypeId;
	typedef int MessageTypeId;

private:
	// Component allocation types
	typedef IComponent* (*AllocFunc)(ScriptInterface& scriptInterface, jsval ctor);
	typedef void (*DeallocFunc)(IComponent*);

	// ComponentTypes come in three types:
	//   Native: normal C++ component
	//   ScriptWrapper: C++ component that wraps a JS component implementation
	//   Script: a ScriptWrapper linked to a specific JS component implementation
	enum EComponentTypeType
	{
		CT_Native,
		CT_ScriptWrapper,
		CT_Script
	};

	// Representation of a component type, to be used when instantiating components
	struct ComponentType
	{
		EComponentTypeType type;
		InterfaceId iid;
		AllocFunc alloc;
		DeallocFunc dealloc;
		std::string name;
		jsval ctor; // only valid if type == CT_Script
	};

public:
	CComponentManager(const CSimContext&, bool skipScriptFunctions = false);
	~CComponentManager();

	void LoadComponentTypes();

	/**
	 * Load a script and execute it in a new function scope.
	 * @param filename VFS path to load
	 * @param hotload set to true if this script has been loaded before, and redefinitions of
	 * existing components should not be considered errors
	 */
	bool LoadScript(const std::wstring& filename, bool hotload = false);

	void RegisterMessageType(MessageTypeId mtid, const char* name);

	void RegisterComponentType(InterfaceId, ComponentTypeId, AllocFunc, DeallocFunc, const char*);
	void RegisterComponentTypeScriptWrapper(InterfaceId, ComponentTypeId, AllocFunc, DeallocFunc, const char*);
	void SubscribeToMessageType(ComponentTypeId, MessageTypeId);

	/**
	 * @param cname Requested component type name (not including any "CID" or "CCmp" prefix)
	 * @return The component type id, or CID__Invalid if not found
	 */
	ComponentTypeId LookupCID(const std::string& cname) const;

	/**
	 * @return The name of the given component type, or "" if not found
	 */
	std::string LookupComponentTypeName(ComponentTypeId cid) const;

	/**
	 * Constructs a component of type 'cid', initialised with data 'paramNode',
	 * and attaches it to entity 'ent'.
	 *
	 * @return true on success; false on failure, and logs an error message
	 */
	bool AddComponent(entity_id_t ent, ComponentTypeId cid, const CParamNode& paramNode);

	/**
	 * Adds an externally-created component, so that it is returned by QueryInterface
	 * but does not get destroyed and does not receive messages from the component manager.
	 * (This is intended for unit tests that need to add mock objects the tested components
	 * expect to exist.)
	 */
	void AddMockComponent(entity_id_t ent, InterfaceId iid, IComponent& component);

	/**
	 * Allocates a component object of type 'cid', and attaches it to entity 'ent'.
	 * (The component's Init is not called here - either Init or Deserialize must be called
	 * before using the returned object.)
	 */
	IComponent* ConstructComponent(entity_id_t ent, ComponentTypeId cid);

	IComponent* QueryInterface(entity_id_t ent, InterfaceId iid) const;
	const std::map<entity_id_t, IComponent*>& GetEntitiesWithInterface(InterfaceId iid) const;

	void PostMessage(entity_id_t ent, const CMessage& msg) const;
	void BroadcastMessage(const CMessage& msg) const;

	void DestroyAllComponents();

	bool ComputeStateHash(std::string& outHash);
	bool DumpDebugState(std::ostream& stream);
	bool SerializeState(std::ostream& stream);
	bool DeserializeState(std::istream& stream);

	ScriptInterface& GetScriptInterface() { return m_ScriptInterface; }

private:
	// Implementations of functions exposed to scripts
	static void Script_RegisterComponentType(void* cbdata, int iid, std::string cname, CScriptVal ctor);
	static void Script_RegisterGlobal(void* cbdata, std::string name, CScriptVal value);
	static IComponent* Script_QueryInterface(void* cbdata, int ent, int iid);
	static void Script_PostMessage(void* cbdata, int ent, int mtid, CScriptVal data);
	static void Script_BroadcastMessage(void* cbdata, int mtid, CScriptVal data);

	ComponentTypeId GetScriptWrapper(InterfaceId iid);

	ScriptInterface m_ScriptInterface;
	const CSimContext& m_SimContext;

	ComponentTypeId m_CurrentComponent;
	bool m_CurrentlyHotloading;

	// TODO: some of these should be vectors
	std::map<ComponentTypeId, ComponentType> m_ComponentTypesById;
	std::map<InterfaceId, std::map<entity_id_t, IComponent*> > m_ComponentsByInterface;
	std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> > m_ComponentsByTypeId;
	std::map<MessageTypeId, std::vector<ComponentTypeId> > m_ComponentTypeIdsByMessageType;
	std::map<std::string, ComponentTypeId> m_ComponentTypeIdsByName;
	std::map<std::string, MessageTypeId> m_MessageTypeIdsByName;
	// TODO: maintaining both ComponentsBy* is nasty; can we get rid of one,
	// while keeping QueryInterface and PostMessage sufficiently efficient?

	ComponentTypeId m_NextScriptComponentTypeId;

	friend class TestComponentManager;
};

#endif // INCLUDED_COMPONENTMANAGER
