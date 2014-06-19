/* Copyright (C) 2011 Wildfire Games.
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
#include "simulation2/helpers/Player.h"
#include "ps/Filesystem.h"

#include <boost/random/linear_congruential.hpp>
#include <boost/unordered_map.hpp>

#include <map>

class IComponent;
class CParamNode;
class CMessage;
class CSimContext;
class CDynamicSubscription;

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
		std::string schema; // RelaxNG fragment
		CScriptValRooted ctor; // only valid if type == CT_Script
	};
	
	struct FindJSONFilesCallbackData
	{
		VfsPath path;
		std::vector<std::string> templates;
	};

public:
	CComponentManager(CSimContext&, shared_ptr<ScriptRuntime> rt, bool skipScriptFunctions = false);
	~CComponentManager();

	void LoadComponentTypes();

	/**
	 * Load a script and execute it in a new function scope.
	 * @param filename VFS path to load
	 * @param hotload set to true if this script has been loaded before, and redefinitions of
	 * existing components should not be considered errors
	 */
	bool LoadScript(const VfsPath& filename, bool hotload = false);

	void RegisterMessageType(MessageTypeId mtid, const char* name);

	void RegisterComponentType(InterfaceId, ComponentTypeId, AllocFunc, DeallocFunc, const char*, const std::string& schema);
	void RegisterComponentTypeScriptWrapper(InterfaceId, ComponentTypeId, AllocFunc, DeallocFunc, const char*, const std::string& schema);
	
	void MarkScriptedComponentForSystemEntity(CComponentManager::ComponentTypeId cid);

	/**
	 * Subscribe the current component type to the given message type.
	 * Each component's HandleMessage will be called on any BroadcastMessage of this message type,
	 * or on any PostMessage of this type targeted at the component's entity.
	 * Must only be called by a component type's ClassInit.
	 */
	void SubscribeToMessageType(MessageTypeId mtid);

	/**
	 * Subscribe the current component type to all messages of the given message type.
	 * Each component's HandleMessage will be called on any BroadcastMessage or PostMessage of this message type,
	 * regardless of the entity.
	 * Must only be called by a component type's ClassInit.
	 */
	void SubscribeGloballyToMessageType(MessageTypeId mtid);

	/**
	 * Subscribe the given component instance to all messages of the given message type.
	 * The component's HandleMessage will be called on any BroadcastMessage or PostMessage of
	 * this message type, regardless of the entity.
	 *
	 * This can be called at any time (including inside the HandleMessage callback for this message type).
	 *
	 * The component type must not have statically subscribed to this message type in its ClassInit.
	 *
	 * The subscription status is not saved or network-synchronised. Components must remember to
	 * resubscribe in their Deserialize methods if they still want the message.
	 *
	 * This is primarily intended for Interpolate and RenderSubmit messages, to avoid the cost of
	 * sending the message to components that do not currently need to do any rendering.
	 */
	void DynamicSubscriptionNonsync(MessageTypeId mtid, IComponent* component, bool enabled);

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
	 * Set up an empty SYSTEM_ENTITY. Must be called after ResetState() and before GetSystemEntity().
	 */
	void InitSystemEntity();

	/**
	 * Returns a CEntityHandle with id SYSTEM_ENTITY.
	 */
	CEntityHandle GetSystemEntity() { ASSERT(m_SystemEntity.GetId() == SYSTEM_ENTITY); return m_SystemEntity; }

	/**
	 * Returns a CEntityHandle with id @p ent.
	 * If @p allowCreate is true and there is no existing CEntityHandle, a new handle will be allocated.
	 */
	CEntityHandle LookupEntityHandle(entity_id_t ent, bool allowCreate = false);

	/**
	 * Returns a new entity ID that has never been used before.
	 * This affects the simulation state so it must only be called in network-synchronised ways.
	 */
	entity_id_t AllocateNewEntity();

	/**
	 * Returns a new local entity ID that has never been used before.
	 * This entity will not be synchronised over the network, stored in saved games, etc.
	 */
	entity_id_t AllocateNewLocalEntity();

	/**
	 * Returns a new entity ID that has never been used before.
	 * If possible, returns preferredId, and ensures this ID won't be allocated again.
	 * This affects the simulation state so it must only be called in network-synchronised ways.
	 */
	entity_id_t AllocateNewEntity(entity_id_t preferredId);

	/**
	 * Constructs a component of type 'cid', initialised with data 'paramNode',
	 * and attaches it to entity 'ent'.
	 *
	 * @return true on success; false on failure, and logs an error message
	 */
	bool AddComponent(CEntityHandle ent, ComponentTypeId cid, const CParamNode& paramNode);

	/**
	 * Add all system components to the system entity (skip the scripted components or the AI components on demand)
	 */
	void AddSystemComponents(bool skipScriptedComponents, bool skipAI);

	/**
	 * Adds an externally-created component, so that it is returned by QueryInterface
	 * but does not get destroyed and does not receive messages from the component manager.
	 * (This is intended for unit tests that need to add mock objects the tested components
	 * expect to exist.)
	 */
	void AddMockComponent(CEntityHandle ent, InterfaceId iid, IComponent& component);

	/**
	 * Allocates a component object of type 'cid', and attaches it to entity 'ent'.
	 * (The component's Init is not called here - either Init or Deserialize must be called
	 * before using the returned object.)
	 */
	IComponent* ConstructComponent(CEntityHandle ent, ComponentTypeId cid);

	/**
	 * Constructs an entity based on the given template, and adds it the world with
	 * entity ID @p ent. There should not be any existing components with that entity ID.
	 * @return ent, or INVALID_ENTITY on error
	 */
	entity_id_t AddEntity(const std::wstring& templateName, entity_id_t ent);

	/**
	 * Destroys all the components belonging to the specified entity when FlushDestroyedComponents is called.
	 * Has no effect if the entity does not exist, or has already been added to the destruction queue.
	 */
	void DestroyComponentsSoon(entity_id_t ent);

	/**
	 * Does the actual destruction of components from DestroyComponentsSoon.
	 * This must not be called if the component manager is on the call stack (since it
	 * will break internal iterators).
	 */
	void FlushDestroyedComponents();

	IComponent* QueryInterface(entity_id_t ent, InterfaceId iid) const;

	typedef std::vector<std::pair<entity_id_t, IComponent*> > InterfaceList;
	typedef boost::unordered_map<entity_id_t, IComponent*> InterfaceListUnordered;

	InterfaceList GetEntitiesWithInterface(InterfaceId iid) const;
	const InterfaceListUnordered& GetEntitiesWithInterfaceUnordered(InterfaceId iid) const;

	/**
	 * Send a message, targeted at a particular entity. The message will be received by any
	 * components of that entity which subscribed to the message type, and by any other components
	 * that subscribed globally to the message type.
	 */
	void PostMessage(entity_id_t ent, const CMessage& msg);

	/**
	 * Send a message, not targeted at any particular entity. The message will be received by any
	 * components that subscribed (either globally or not) to the message type.
	 */
	void BroadcastMessage(const CMessage& msg);

	/**
	 * Resets the dynamic simulation state (deletes all entities, resets entity ID counters;
	 * doesn't unload/reload component scripts).
	 */
	void ResetState();

	// Various state serialization functions:
	bool ComputeStateHash(std::string& outHash, bool quick);
	bool DumpDebugState(std::ostream& stream, bool includeDebugInfo);
	// FlushDestroyedComponents must be called before SerializeState (since the destruction queue
	// won't get serialized)
	bool SerializeState(std::ostream& stream);
	bool DeserializeState(std::istream& stream);

	std::string GenerateSchema();

	ScriptInterface& GetScriptInterface() { return m_ScriptInterface; }

private:
	// Implementations of functions exposed to scripts
	static void Script_RegisterComponentType_Common(ScriptInterface::CxPrivate* pCxPrivate, int iid, std::string cname, CScriptVal ctor, bool reRegister, bool systemComponent);
	static void Script_RegisterComponentType(ScriptInterface::CxPrivate* pCxPrivate, int iid, std::string cname, CScriptVal ctor);
	static void Script_RegisterSystemComponentType(ScriptInterface::CxPrivate* pCxPrivate, int iid, std::string cname, CScriptVal ctor);
	static void Script_ReRegisterComponentType(ScriptInterface::CxPrivate* pCxPrivate, int iid, std::string cname, CScriptVal ctor);
	static void Script_RegisterInterface(ScriptInterface::CxPrivate* pCxPrivate, std::string name);
	static void Script_RegisterMessageType(ScriptInterface::CxPrivate* pCxPrivate, std::string name);
	static void Script_RegisterGlobal(ScriptInterface::CxPrivate* pCxPrivate, std::string name, CScriptVal value);
	static IComponent* Script_QueryInterface(ScriptInterface::CxPrivate* pCxPrivate, int ent, int iid);
	static std::vector<int> Script_GetEntitiesWithInterface(ScriptInterface::CxPrivate* pCxPrivate, int iid);
	static std::vector<IComponent*> Script_GetComponentsWithInterface(ScriptInterface::CxPrivate* pCxPrivate, int iid);
	static void Script_PostMessage(ScriptInterface::CxPrivate* pCxPrivate, int ent, int mtid, CScriptVal data);
	static void Script_BroadcastMessage(ScriptInterface::CxPrivate* pCxPrivate, int mtid, CScriptVal data);
	static int Script_AddEntity(ScriptInterface::CxPrivate* pCxPrivate, std::string templateName);
	static int Script_AddLocalEntity(ScriptInterface::CxPrivate* pCxPrivate, std::string templateName);
	static void Script_DestroyEntity(ScriptInterface::CxPrivate* pCxPrivate, int ent);
	static CScriptVal Script_ReadJSONFile(ScriptInterface::CxPrivate* pCxPrivate, std::wstring fileName);
	static CScriptVal Script_ReadCivJSONFile(ScriptInterface::CxPrivate* pCxPrivate, std::wstring fileName);
	static std::vector<std::string> Script_FindJSONFiles(ScriptInterface::CxPrivate* pCxPrivate, std::wstring subPath, bool recursive);
	static CScriptVal ReadJSONFile(ScriptInterface::CxPrivate* pCxPrivate, std::wstring filePath, std::wstring fileName);
	
	// callback function to handle recursively finding files in a directory
	static Status FindJSONFilesCallback(const VfsPath&, const CFileInfo&, const uintptr_t);

	CMessage* ConstructMessage(int mtid, CScriptVal data);
	void SendGlobalMessage(entity_id_t ent, const CMessage& msg);

	void FlattenDynamicSubscriptions();
	void RemoveComponentDynamicSubscriptions(IComponent* component);

	ComponentTypeId GetScriptWrapper(InterfaceId iid);

	CEntityHandle AllocateEntityHandle(entity_id_t ent);

	ScriptInterface m_ScriptInterface;
	CSimContext& m_SimContext;

	CEntityHandle m_SystemEntity;

	ComponentTypeId m_CurrentComponent; // used when loading component types
	bool m_CurrentlyHotloading;

	// TODO: some of these should be vectors
	std::map<ComponentTypeId, ComponentType> m_ComponentTypesById;
	std::vector<CComponentManager::ComponentTypeId> m_ScriptedSystemComponents;
	std::vector<boost::unordered_map<entity_id_t, IComponent*> > m_ComponentsByInterface; // indexed by InterfaceId
	std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> > m_ComponentsByTypeId;
	std::map<MessageTypeId, std::vector<ComponentTypeId> > m_LocalMessageSubscriptions;
	std::map<MessageTypeId, std::vector<ComponentTypeId> > m_GlobalMessageSubscriptions;
	std::map<std::string, ComponentTypeId> m_ComponentTypeIdsByName;
	std::map<std::string, MessageTypeId> m_MessageTypeIdsByName;
	std::map<MessageTypeId, std::string> m_MessageTypeNamesById;
	std::map<std::string, InterfaceId> m_InterfaceIdsByName;

	std::map<MessageTypeId, CDynamicSubscription> m_DynamicMessageSubscriptionsNonsync;
	std::map<IComponent*, std::set<MessageTypeId> > m_DynamicMessageSubscriptionsNonsyncByComponent;

	std::map<entity_id_t, SEntityComponentCache*> m_ComponentCaches;

	// TODO: maintaining both ComponentsBy* is nasty; can we get rid of one,
	// while keeping QueryInterface and PostMessage sufficiently efficient?

	std::vector<entity_id_t> m_DestructionQueue;

	ComponentTypeId m_NextScriptComponentTypeId;
	entity_id_t m_NextEntityId;
	entity_id_t m_NextLocalEntityId;

	boost::rand48 m_RNG;

	friend class TestComponentManager;
};

#endif // INCLUDED_COMPONENTMANAGER
