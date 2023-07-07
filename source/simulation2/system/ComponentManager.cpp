/* Copyright (C) 2023 Wildfire Games.
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

#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/scripting/JSInterface_VFS.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/Object.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/system/DynamicSubscription.h"
#include "simulation2/system/IComponent.h"
#include "simulation2/system/ParamNode.h"
#include "simulation2/system/SimContext.h"

#include <string_view>

/**
 * Used for script-only message types.
 */
class CMessageScripted final : public CMessage
{
public:
	virtual int GetType() const { return mtid; }
	virtual const char* GetScriptHandlerName() const { return handlerName.c_str(); }
	virtual const char* GetScriptGlobalHandlerName() const { return globalHandlerName.c_str(); }
	virtual JS::Value ToJSVal(const ScriptRequest& UNUSED(rq)) const { return msg.get(); }

	CMessageScripted(const ScriptRequest& rq, int mtid, const std::string& name, JS::HandleValue msg) :
		mtid(mtid), handlerName("On" + name), globalHandlerName("OnGlobal" + name), msg(rq.cx, msg)
	{
	}

	int mtid;
	std::string handlerName;
	std::string globalHandlerName;
	JS::PersistentRootedValue msg;
};

CComponentManager::CComponentManager(CSimContext& context, std::shared_ptr<ScriptContext> cx, bool skipScriptFunctions) :
	m_NextScriptComponentTypeId(CID__LastNative),
	m_ScriptInterface("Engine", "Simulation", cx),
	m_SimContext(context), m_CurrentlyHotloading(false)
{
	context.SetComponentManager(this);

	m_ScriptInterface.SetCallbackData(static_cast<void*> (this));
	m_ScriptInterface.ReplaceNondeterministicRNG(m_RNG);

	// For component script tests, the test system sets up its own scripted implementation of
	// these functions, so we skip registering them here in those cases
	if (!skipScriptFunctions)
	{
		JSI_VFS::RegisterScriptFunctions_ReadOnlySimulation(m_ScriptInterface);
		ScriptRequest rq(m_ScriptInterface);
		constexpr ScriptFunction::ObjectGetter<CComponentManager> Getter = &ScriptInterface::ObjectFromCBData<CComponentManager>;
		ScriptFunction::Register<&CComponentManager::Script_RegisterComponentType, Getter>(rq, "RegisterComponentType");
		ScriptFunction::Register<&CComponentManager::Script_RegisterSystemComponentType, Getter>(rq, "RegisterSystemComponentType");
		ScriptFunction::Register<&CComponentManager::Script_ReRegisterComponentType, Getter>(rq, "ReRegisterComponentType");
		ScriptFunction::Register<&CComponentManager::Script_RegisterInterface, Getter>(rq, "RegisterInterface");
		ScriptFunction::Register<&CComponentManager::Script_RegisterMessageType, Getter>(rq, "RegisterMessageType");
		ScriptFunction::Register<&CComponentManager::Script_RegisterGlobal, Getter>(rq, "RegisterGlobal");
		ScriptFunction::Register<&CComponentManager::Script_GetEntitiesWithInterface, Getter>(rq, "GetEntitiesWithInterface");
		ScriptFunction::Register<&CComponentManager::Script_GetComponentsWithInterface, Getter>(rq, "GetComponentsWithInterface");
		ScriptFunction::Register<&CComponentManager::Script_PostMessage, Getter>(rq, "PostMessage");
		ScriptFunction::Register<&CComponentManager::Script_BroadcastMessage, Getter>(rq, "BroadcastMessage");
		ScriptFunction::Register<&CComponentManager::Script_AddEntity, Getter>(rq, "AddEntity");
		ScriptFunction::Register<&CComponentManager::Script_AddLocalEntity, Getter>(rq, "AddLocalEntity");
		ScriptFunction::Register<&CComponentManager::QueryInterface, Getter>(rq, "QueryInterface");
		ScriptFunction::Register<&CComponentManager::DestroyComponentsSoon, Getter>(rq, "DestroyEntity");
		ScriptFunction::Register<&CComponentManager::FlushDestroyedComponents, Getter>(rq, "FlushDestroyedEntities");
		ScriptFunction::Register<&CComponentManager::Script_GetTemplate, Getter>(rq, "GetTemplate");

	}

	// Globalscripts may use VFS script functions
	m_ScriptInterface.LoadGlobalScripts();

	// Define MT_*, IID_* as script globals, and store their names
#define MESSAGE(name) m_ScriptInterface.SetGlobal("MT_" #name, (int)MT_##name);
#define INTERFACE(name) \
	m_ScriptInterface.SetGlobal("IID_" #name, (int)IID_##name); \
	m_InterfaceIdsByName[#name] = IID_##name;
#define COMPONENT(name)
#include "simulation2/TypeList.h"
#undef MESSAGE
#undef INTERFACE
#undef COMPONENT

	m_ScriptInterface.SetGlobal("INVALID_ENTITY", (int)INVALID_ENTITY);
	m_ScriptInterface.SetGlobal("INVALID_PLAYER", (int)INVALID_PLAYER);
	m_ScriptInterface.SetGlobal("SYSTEM_ENTITY", (int)SYSTEM_ENTITY);

	m_ComponentsByInterface.resize(IID__LastNative);

	ResetState();
}

CComponentManager::~CComponentManager()
{
	ResetState();
}

void CComponentManager::LoadComponentTypes()
{
#define MESSAGE(name) \
	RegisterMessageType(MT_##name, #name);
#define INTERFACE(name) \
	extern void RegisterComponentInterface_##name(ScriptInterface&); \
	RegisterComponentInterface_##name(m_ScriptInterface);
#define COMPONENT(name) \
	extern void RegisterComponentType_##name(CComponentManager&); \
	m_CurrentComponent = CID_##name; \
	RegisterComponentType_##name(*this);

#include "simulation2/TypeList.h"

	m_CurrentComponent = CID__Invalid;

#undef MESSAGE
#undef INTERFACE
#undef COMPONENT
}


bool CComponentManager::LoadScript(const VfsPath& filename, bool hotload)
{
	m_CurrentlyHotloading = hotload;
	CVFSFile file;
	PSRETURN loadOk = file.Load(g_VFS, filename);
	if (loadOk != PSRETURN_OK) // VFS will log the failed file and the reason
		return false;
	std::string content = file.DecodeUTF8(); // assume it's UTF-8
	bool ok = m_ScriptInterface.LoadScript(filename, content);

	m_CurrentlyHotloading = false;
	return ok;
}

void CComponentManager::Script_RegisterComponentType_Common(int iid, const std::string& cname, JS::HandleValue ctor, bool reRegister, bool systemComponent)
{
	ScriptRequest rq(m_ScriptInterface);

	// Find the C++ component that wraps the interface
	int cidWrapper = GetScriptWrapper(iid);
	if (cidWrapper == CID__Invalid)
	{
		ScriptException::Raise(rq, "Invalid interface id");
		return;
	}
	const ComponentType& ctWrapper = m_ComponentTypesById[cidWrapper];

	bool mustReloadComponents = false; // for hotloading

	ComponentTypeId cid = LookupCID(cname);
	if (cid == CID__Invalid)
	{
		if (reRegister)
		{
			ScriptException::Raise(rq, "ReRegistering component type that was not registered before '%s'", cname.c_str());
			return;
		}
		// Allocate a new cid number
		cid = m_NextScriptComponentTypeId++;
		m_ComponentTypeIdsByName[cname] = cid;
		if (systemComponent)
			MarkScriptedComponentForSystemEntity(cid);
	}
	else
	{
		// Component type is already loaded, so do hotloading:

		if (!m_CurrentlyHotloading && !reRegister)
		{
			ScriptException::Raise(rq, "Registering component type with already-registered name '%s'", cname.c_str());
			return;
		}

		const ComponentType& ctPrevious = m_ComponentTypesById[cid];

		// We can only replace scripted component types, not native ones
		if (ctPrevious.type != CT_Script)
		{
			ScriptException::Raise(rq, "Loading script component type with same name '%s' as native component", cname.c_str());
			return;
		}

		// We don't support changing the IID of a component type (it would require fiddling
		// around with m_ComponentsByInterface and being careful to guarantee uniqueness per entity)
		if (ctPrevious.iid != iid)
		{
			// ...though it only matters if any components exist with this type
			if (!m_ComponentsByTypeId[cid].empty())
			{
				ScriptException::Raise(rq, "Hotloading script component type mustn't change interface ID");
				return;
			}
		}

		// Remove the old component type's message subscriptions
		std::map<MessageTypeId, std::vector<ComponentTypeId> >::iterator it;
		for (it = m_LocalMessageSubscriptions.begin(); it != m_LocalMessageSubscriptions.end(); ++it)
		{
			std::vector<ComponentTypeId>& types = it->second;
			std::vector<ComponentTypeId>::iterator ctit = find(types.begin(), types.end(), cid);
			if (ctit != types.end())
				types.erase(ctit);
		}
		for (it = m_GlobalMessageSubscriptions.begin(); it != m_GlobalMessageSubscriptions.end(); ++it)
		{
			std::vector<ComponentTypeId>& types = it->second;
			std::vector<ComponentTypeId>::iterator ctit = find(types.begin(), types.end(), cid);
			if (ctit != types.end())
				types.erase(ctit);
		}

		mustReloadComponents = true;
	}

	JS::RootedValue protoVal(rq.cx);
	if (!Script::GetProperty(rq, ctor, "prototype", &protoVal))
	{
		ScriptException::Raise(rq, "Failed to get property 'prototype'");
		return;
	}
	if (!protoVal.isObject())
	{
		ScriptException::Raise(rq, "Component has no constructor");
		return;
	}
	std::string schema = "<empty/>";

	if (Script::HasProperty(rq, protoVal, "Schema"))
		Script::GetProperty(rq, protoVal, "Schema", schema);

	// Construct a new ComponentType, using the wrapper's alloc functions
	ComponentType ct{
		CT_Script,
		iid,
		ctWrapper.alloc,
		ctWrapper.dealloc,
		cname,
		schema,
		std::make_unique<JS::PersistentRootedValue>(rq.cx, ctor)
	};
	m_ComponentTypesById[cid] = std::move(ct);

	m_CurrentComponent = cid; // needed by Subscribe

	// Find all the ctor prototype's On* methods, and subscribe to the appropriate messages:
	std::vector<std::string> methods;

	if (!Script::EnumeratePropertyNames(rq, protoVal, false, methods))
	{
		ScriptException::Raise(rq, "Failed to enumerate component properties.");
		return;
	}

	for (const std::string& method : methods)
	{
		if (std::string_view{method}.substr(0, 2) != "On")
			continue;

		std::string_view name{std::string_view{method}.substr(2)}; // strip the "On" prefix

		// Handle "OnGlobalFoo" functions specially
		bool isGlobal = false;
		if (std::string_view{name}.substr(0, 6) == "Global")
		{
			isGlobal = true;
			name.remove_prefix(6);
		}

		auto mit = m_MessageTypeIdsByName.find(std::string{name});
		if (mit == m_MessageTypeIdsByName.end())
		{
			ScriptException::Raise(rq,
				"Registered component has unrecognized '%s' message handler method",
				method.c_str());
			return;
		}

		if (isGlobal)
			SubscribeGloballyToMessageType(mit->second);
		else
			SubscribeToMessageType(mit->second);
	}

	m_CurrentComponent = CID__Invalid;

	if (mustReloadComponents)
	{
		// For every script component with this cid, we need to switch its
		// prototype from the old constructor's prototype property to the new one's
		const std::map<entity_id_t, IComponent*>& comps = m_ComponentsByTypeId[cid];
		std::map<entity_id_t, IComponent*>::const_iterator eit = comps.begin();
		for (; eit != comps.end(); ++eit)
		{
			JS::RootedValue instance(rq.cx, eit->second->GetJSInstance());
			if (!instance.isNull())
				m_ScriptInterface.SetPrototype(instance, protoVal);
		}
	}
}

void CComponentManager::Script_RegisterComponentType(int iid, const std::string& cname, JS::HandleValue ctor)
{
	Script_RegisterComponentType_Common(iid, cname, ctor, false, false);
	m_ScriptInterface.SetGlobal(cname.c_str(), ctor, m_CurrentlyHotloading);
}

void CComponentManager::Script_RegisterSystemComponentType(int iid, const std::string& cname, JS::HandleValue ctor)
{
	Script_RegisterComponentType_Common(iid, cname, ctor, false, true);
	m_ScriptInterface.SetGlobal(cname.c_str(), ctor, m_CurrentlyHotloading);
}

void CComponentManager::Script_ReRegisterComponentType(int iid, const std::string& cname, JS::HandleValue ctor)
{
	Script_RegisterComponentType_Common(iid, cname, ctor, true, false);
}

void CComponentManager::Script_RegisterInterface(const std::string& name)
{
	std::map<std::string, InterfaceId>::iterator it = m_InterfaceIdsByName.find(name);
	if (it != m_InterfaceIdsByName.end())
	{
		// Redefinitions are fine (and just get ignored) when hotloading; otherwise
		// they're probably unintentional and should be reported
		if (!m_CurrentlyHotloading)
		{
			ScriptRequest rq(m_ScriptInterface);
			ScriptException::Raise(rq, "Registering interface with already-registered name '%s'", name.c_str());
		}
		return;
	}

	// IIDs start at 1, so size+1 is the next unused one
	size_t id = m_InterfaceIdsByName.size() + 1;
	m_InterfaceIdsByName[name] = (InterfaceId)id;
	m_ComponentsByInterface.resize(id+1); // add one so we can index by InterfaceId
	m_ScriptInterface.SetGlobal(("IID_" + name).c_str(), (int)id);
}

void CComponentManager::Script_RegisterMessageType(const std::string& name)
{
	std::map<std::string, MessageTypeId>::iterator it = m_MessageTypeIdsByName.find(name);
	if (it != m_MessageTypeIdsByName.end())
	{
		// Redefinitions are fine (and just get ignored) when hotloading; otherwise
		// they're probably unintentional and should be reported
		if (!m_CurrentlyHotloading)
		{
			ScriptRequest rq(m_ScriptInterface);
			ScriptException::Raise(rq, "Registering message type with already-registered name '%s'", name.c_str());
		}
		return;
	}

	// MTIDs start at 1, so size+1 is the next unused one
	size_t id = m_MessageTypeIdsByName.size() + 1;
	RegisterMessageType((MessageTypeId)id, name.c_str());
	m_ScriptInterface.SetGlobal(("MT_" + name).c_str(), (int)id);
}

void CComponentManager::Script_RegisterGlobal(const std::string& name, JS::HandleValue value)
{
	m_ScriptInterface.SetGlobal(name.c_str(), value, m_CurrentlyHotloading);
}

const CParamNode& CComponentManager::Script_GetTemplate(const std::string& templateName)
{
	static CParamNode nullNode(false);

	ICmpTemplateManager* cmpTemplateManager = static_cast<ICmpTemplateManager*> (QueryInterface(SYSTEM_ENTITY, IID_TemplateManager));
	if (!cmpTemplateManager)
	{
		LOGERROR("Template manager is not loaded");
		return nullNode;
	}

	const CParamNode* tmpl = cmpTemplateManager->GetTemplate(templateName);
	if (!tmpl)
		return nullNode;
	return *tmpl;
}

std::vector<int> CComponentManager::Script_GetEntitiesWithInterface(int iid)
{
	std::vector<int> ret;
	const InterfaceListUnordered& ents = GetEntitiesWithInterfaceUnordered(iid);
	for (InterfaceListUnordered::const_iterator it = ents.begin(); it != ents.end(); ++it)
		if (!ENTITY_IS_LOCAL(it->first))
			ret.push_back(it->first);
	std::sort(ret.begin(), ret.end());
	return ret;
}

std::vector<IComponent*> CComponentManager::Script_GetComponentsWithInterface(int iid)
{
	std::vector<IComponent*> ret;
	InterfaceList ents = GetEntitiesWithInterface(iid);
	for (InterfaceList::const_iterator it = ents.begin(); it != ents.end(); ++it)
		ret.push_back(it->second); // TODO: maybe we should exclude local entities
	return ret;
}

CMessage* CComponentManager::ConstructMessage(int mtid, JS::HandleValue data)
{
	if (mtid == MT__Invalid || mtid > (int)m_MessageTypeIdsByName.size()) // (IDs start at 1 so use '>' here)
		LOGERROR("PostMessage with invalid message type ID '%d'", mtid);

	ScriptRequest rq(m_ScriptInterface);
	if (mtid < MT__LastNative)
	{
		return CMessageFromJSVal(mtid, rq, data);
	}
	else
	{
		return new CMessageScripted(rq, mtid, m_MessageTypeNamesById[mtid], data);
	}
}

void CComponentManager::Script_PostMessage(int ent, int mtid, JS::HandleValue data)
{
	CMessage* msg = ConstructMessage(mtid, data);
	if (!msg)
		return; // error

	PostMessage(ent, *msg);

	delete msg;
}

void CComponentManager::Script_BroadcastMessage(int mtid, JS::HandleValue data)
{
	CMessage* msg = ConstructMessage(mtid, data);
	if (!msg)
		return; // error

	BroadcastMessage(*msg);

	delete msg;
}

int CComponentManager::Script_AddEntity(const std::wstring& templateName)
{
	// TODO: should validate the string to make sure it doesn't contain scary characters
	// that will let it access non-component-template files
	return AddEntity(templateName, AllocateNewEntity());
}

int CComponentManager::Script_AddLocalEntity(const std::wstring& templateName)
{
	// TODO: should validate the string to make sure it doesn't contain scary characters
	// that will let it access non-component-template files
	return AddEntity(templateName, AllocateNewLocalEntity());
}

void CComponentManager::ResetState()
{
	// Delete all dynamic message subscriptions
	m_DynamicMessageSubscriptionsNonsync.clear();
	m_DynamicMessageSubscriptionsNonsyncByComponent.clear();

	// Delete all IComponents in reverse order of creation.
	std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::reverse_iterator iit = m_ComponentsByTypeId.rbegin();
	for (; iit != m_ComponentsByTypeId.rend(); ++iit)
	{
		std::map<entity_id_t, IComponent*>::iterator eit = iit->second.begin();
		for (; eit != iit->second.end(); ++eit)
		{
			eit->second->Deinit();
			m_ComponentTypesById[iit->first].dealloc(eit->second);
		}
	}

	std::vector<std::unordered_map<entity_id_t, IComponent*> >::iterator ifcit = m_ComponentsByInterface.begin();
	for (; ifcit != m_ComponentsByInterface.end(); ++ifcit)
		ifcit->clear();

	m_ComponentsByTypeId.clear();

	// Delete all SEntityComponentCaches
	std::unordered_map<entity_id_t, SEntityComponentCache*>::iterator ccit = m_ComponentCaches.begin();
	for (; ccit != m_ComponentCaches.end(); ++ccit)
		free(ccit->second);
	m_ComponentCaches.clear();
	m_SystemEntity = CEntityHandle();

	m_DestructionQueue.clear();

	// Reset IDs
	m_NextEntityId = SYSTEM_ENTITY + 1;
	m_NextLocalEntityId = FIRST_LOCAL_ENTITY;
}

void CComponentManager::SetRNGSeed(u32 seed)
{
	m_RNG.seed(seed);
}

void CComponentManager::RegisterComponentType(InterfaceId iid, ComponentTypeId cid, AllocFunc alloc, DeallocFunc dealloc,
		const char* name, const std::string& schema)
{
	ComponentType c{ CT_Native, iid, alloc, dealloc, name, schema, std::unique_ptr<JS::PersistentRootedValue>() };
	m_ComponentTypesById.insert(std::make_pair(cid, std::move(c)));
	m_ComponentTypeIdsByName[name] = cid;
}

void CComponentManager::RegisterComponentTypeScriptWrapper(InterfaceId iid, ComponentTypeId cid, AllocFunc alloc,
		DeallocFunc dealloc, const char* name, const std::string& schema)
{
	ComponentType c{ CT_ScriptWrapper, iid, alloc, dealloc, name, schema, std::unique_ptr<JS::PersistentRootedValue>() };
	m_ComponentTypesById.insert(std::make_pair(cid, std::move(c)));
	m_ComponentTypeIdsByName[name] = cid;
	// TODO: merge with RegisterComponentType
}

void CComponentManager::MarkScriptedComponentForSystemEntity(CComponentManager::ComponentTypeId cid)
{
	m_ScriptedSystemComponents.push_back(cid);
}

void CComponentManager::RegisterMessageType(MessageTypeId mtid, const char* name)
{
	m_MessageTypeIdsByName[name] = mtid;
	m_MessageTypeNamesById[mtid] = name;
}

void CComponentManager::SubscribeToMessageType(MessageTypeId mtid)
{
	// TODO: verify mtid
	ENSURE(m_CurrentComponent != CID__Invalid);
	std::vector<ComponentTypeId>& types = m_LocalMessageSubscriptions[mtid];
	types.push_back(m_CurrentComponent);
	std::sort(types.begin(), types.end()); // TODO: just sort once at the end of LoadComponents
}

void CComponentManager::SubscribeGloballyToMessageType(MessageTypeId mtid)
{
	// TODO: verify mtid
	ENSURE(m_CurrentComponent != CID__Invalid);
	std::vector<ComponentTypeId>& types = m_GlobalMessageSubscriptions[mtid];
	types.push_back(m_CurrentComponent);
	std::sort(types.begin(), types.end()); // TODO: just sort once at the end of LoadComponents
}

void CComponentManager::FlattenDynamicSubscriptions()
{
	std::map<MessageTypeId, CDynamicSubscription>::iterator it;
	for (it = m_DynamicMessageSubscriptionsNonsync.begin();
	     it != m_DynamicMessageSubscriptionsNonsync.end(); ++it)
	{
		it->second.Flatten();
	}
}

void CComponentManager::DynamicSubscriptionNonsync(MessageTypeId mtid, IComponent* component, bool enable)
{
	if (enable)
	{
		bool newlyInserted = m_DynamicMessageSubscriptionsNonsyncByComponent[component].insert(mtid).second;
		if (newlyInserted)
			m_DynamicMessageSubscriptionsNonsync[mtid].Add(component);
	}
	else
	{
		size_t numRemoved = m_DynamicMessageSubscriptionsNonsyncByComponent[component].erase(mtid);
		if (numRemoved)
			m_DynamicMessageSubscriptionsNonsync[mtid].Remove(component);
	}
}

void CComponentManager::RemoveComponentDynamicSubscriptions(IComponent* component)
{
	std::map<IComponent*, std::set<MessageTypeId> >::iterator it = m_DynamicMessageSubscriptionsNonsyncByComponent.find(component);
	if (it == m_DynamicMessageSubscriptionsNonsyncByComponent.end())
		return;

	std::set<MessageTypeId>::iterator mtit;
	for (mtit = it->second.begin(); mtit != it->second.end(); ++mtit)
	{
		m_DynamicMessageSubscriptionsNonsync[*mtit].Remove(component);

		// Need to flatten the subscription lists immediately to avoid dangling IComponent* references
		m_DynamicMessageSubscriptionsNonsync[*mtit].Flatten();
	}

	m_DynamicMessageSubscriptionsNonsyncByComponent.erase(it);
}

CComponentManager::ComponentTypeId CComponentManager::LookupCID(const std::string& cname) const
{
	std::map<std::string, ComponentTypeId>::const_iterator it = m_ComponentTypeIdsByName.find(cname);
	if (it == m_ComponentTypeIdsByName.end())
		return CID__Invalid;
	return it->second;
}

std::string CComponentManager::LookupComponentTypeName(ComponentTypeId cid) const
{
	std::map<ComponentTypeId, ComponentType>::const_iterator it = m_ComponentTypesById.find(cid);
	if (it == m_ComponentTypesById.end())
		return "";
	return it->second.name;
}

CComponentManager::ComponentTypeId CComponentManager::GetScriptWrapper(InterfaceId iid)
{
	if (iid >= IID__LastNative && iid <= (int)m_InterfaceIdsByName.size()) // use <= since IDs start at 1
		return CID_UnknownScript;

	std::map<ComponentTypeId, ComponentType>::const_iterator it = m_ComponentTypesById.begin();
	for (; it != m_ComponentTypesById.end(); ++it)
		if (it->second.iid == iid && it->second.type == CT_ScriptWrapper)
			return it->first;

	std::map<std::string, InterfaceId>::const_iterator iiit = m_InterfaceIdsByName.begin();
	for (; iiit != m_InterfaceIdsByName.end(); ++iiit)
		if (iiit->second == iid)
		{
			LOGERROR("No script wrapper found for interface id %d '%s'", iid, iiit->first.c_str());
			return CID__Invalid;
		}

	LOGERROR("No script wrapper found for interface id %d", iid);
	return CID__Invalid;
}

entity_id_t CComponentManager::AllocateNewEntity()
{
	entity_id_t id = m_NextEntityId++;
	// TODO: check for overflow
	return id;
}

entity_id_t CComponentManager::AllocateNewLocalEntity()
{
	entity_id_t id = m_NextLocalEntityId++;
	// TODO: check for overflow
	return id;
}

entity_id_t CComponentManager::AllocateNewEntity(entity_id_t preferredId)
{
	// TODO: ensure this ID hasn't been allocated before
	// (this might occur with broken map files)
	// Trying to actually add two entities with the same id will fail in AddEntitiy
	entity_id_t id = preferredId;

	// Ensure this ID won't be allocated again
	if (id >= m_NextEntityId)
		m_NextEntityId = id+1;
	// TODO: check for overflow

	return id;
}

bool CComponentManager::AddComponent(CEntityHandle ent, ComponentTypeId cid, const CParamNode& paramNode)
{
	IComponent* component = ConstructComponent(ent, cid);
	if (!component)
		return false;

	component->Init(paramNode);
	return true;
}

void CComponentManager::AddSystemComponents(bool skipScriptedComponents, bool skipAI)
{
	CParamNode noParam;
	AddComponent(m_SystemEntity, CID_TemplateManager, noParam);
	AddComponent(m_SystemEntity, CID_CinemaManager, noParam);
	AddComponent(m_SystemEntity, CID_CommandQueue, noParam);
	AddComponent(m_SystemEntity, CID_ObstructionManager, noParam);
	AddComponent(m_SystemEntity, CID_ParticleManager, noParam);
	AddComponent(m_SystemEntity, CID_Pathfinder, noParam);
	AddComponent(m_SystemEntity, CID_ProjectileManager, noParam);
	AddComponent(m_SystemEntity, CID_RangeManager, noParam);
	AddComponent(m_SystemEntity, CID_SoundManager, noParam);
	AddComponent(m_SystemEntity, CID_Terrain, noParam);
	AddComponent(m_SystemEntity, CID_TerritoryManager, noParam);
	AddComponent(m_SystemEntity, CID_UnitMotionManager, noParam);
	AddComponent(m_SystemEntity, CID_UnitRenderer, noParam);
	AddComponent(m_SystemEntity, CID_WaterManager, noParam);

	// Add scripted system components:
	if (!skipScriptedComponents)
	{
		for (uint32_t i = 0; i < m_ScriptedSystemComponents.size(); ++i)
			AddComponent(m_SystemEntity, m_ScriptedSystemComponents[i], noParam);
		if (!skipAI)
			AddComponent(m_SystemEntity, CID_AIManager, noParam);
	}
}

IComponent* CComponentManager::ConstructComponent(CEntityHandle ent, ComponentTypeId cid)
{
	ScriptRequest rq(m_ScriptInterface);

	std::map<ComponentTypeId, ComponentType>::const_iterator it = m_ComponentTypesById.find(cid);
	if (it == m_ComponentTypesById.end())
	{
		LOGERROR("Invalid component id %d", cid);
		return NULL;
	}

	const ComponentType& ct = it->second;

	ENSURE((size_t)ct.iid < m_ComponentsByInterface.size());

	std::unordered_map<entity_id_t, IComponent*>& emap1 = m_ComponentsByInterface[ct.iid];
	if (emap1.find(ent.GetId()) != emap1.end())
	{
		LOGERROR("Multiple components for interface %d", ct.iid);
		return NULL;
	}

	std::map<entity_id_t, IComponent*>& emap2 = m_ComponentsByTypeId[cid];

	// If this is a scripted component, construct the appropriate JS object first
	JS::RootedValue obj(rq.cx);
	if (ct.type == CT_Script)
	{
		m_ScriptInterface.CallConstructor(*ct.ctor, JS::HandleValueArray::empty(), &obj);
		if (obj.isNull())
		{
			LOGERROR("Script component constructor failed");
			return NULL;
		}
	}

	// Construct the new component
	// NB: The unit motion manager relies on components not moving in memory once constructed.
	IComponent* component = ct.alloc(m_ScriptInterface, obj);
	ENSURE(component);

	component->SetEntityHandle(ent);
	component->SetSimContext(m_SimContext);

	// Store a reference to the new component
	emap1.insert(std::make_pair(ent.GetId(), component));
	emap2.insert(std::make_pair(ent.GetId(), component));
	// TODO: We need to more careful about this - if an entity is constructed by a component
	// while we're iterating over all components, this will invalidate the iterators and everything
	// will break.
	// We probably need some kind of delayed addition, so they get pushed onto a queue and then
	// inserted into the world later on. (Be careful about immediation deletion in that case, too.)

	SEntityComponentCache* cache = ent.GetComponentCache();
	ENSURE(cache != NULL && ct.iid < (int)cache->numInterfaces && cache->interfaces[ct.iid] == NULL);
	cache->interfaces[ct.iid] = component;

	return component;
}

void CComponentManager::AddMockComponent(CEntityHandle ent, InterfaceId iid, IComponent& component)
{
	// Just add it into the by-interface map, not the by-component-type map,
	// so it won't be considered for messages or deletion etc

	std::unordered_map<entity_id_t, IComponent*>& emap1 = m_ComponentsByInterface.at(iid);
	if (emap1.find(ent.GetId()) != emap1.end())
		debug_warn(L"Multiple components for interface");
	emap1.insert(std::make_pair(ent.GetId(), &component));

	SEntityComponentCache* cache = ent.GetComponentCache();
	ENSURE(cache != NULL && iid < (int)cache->numInterfaces && cache->interfaces[iid] == NULL);
	cache->interfaces[iid] = &component;
}

CEntityHandle CComponentManager::AllocateEntityHandle(entity_id_t ent)
{
	ENSURE(!EntityExists(ent));

	// Interface IDs start at 1, and SEntityComponentCache is defined with a 1-sized array,
	// so we need space for an extra m_InterfaceIdsByName.size() items
	SEntityComponentCache* cache = (SEntityComponentCache*)calloc(1,
		sizeof(SEntityComponentCache) + sizeof(IComponent*) * m_InterfaceIdsByName.size());
	ENSURE(cache != NULL);
	cache->numInterfaces = m_InterfaceIdsByName.size() + 1;

	m_ComponentCaches[ent] = cache;

	return CEntityHandle(ent, cache);
}

CEntityHandle CComponentManager::LookupEntityHandle(entity_id_t ent, bool allowCreate)
{
	std::unordered_map<entity_id_t, SEntityComponentCache*>::iterator it;
	it = m_ComponentCaches.find(ent);
	if (it == m_ComponentCaches.end())
	{
		if (allowCreate)
			return AllocateEntityHandle(ent);
		else
			return CEntityHandle(ent, NULL);
	}
	else
		return CEntityHandle(ent, it->second);
}

void CComponentManager::InitSystemEntity()
{
	ENSURE(m_SystemEntity.GetId() == INVALID_ENTITY);
	m_SystemEntity = AllocateEntityHandle(SYSTEM_ENTITY);
	m_SimContext.SetSystemEntity(m_SystemEntity);
}

entity_id_t CComponentManager::AddEntity(const std::wstring& templateName, entity_id_t ent)
{
	ICmpTemplateManager *cmpTemplateManager = static_cast<ICmpTemplateManager*> (QueryInterface(SYSTEM_ENTITY, IID_TemplateManager));
	if (!cmpTemplateManager)
	{
		debug_warn(L"No ICmpTemplateManager loaded");
		return INVALID_ENTITY;
	}

	const CParamNode* tmpl = cmpTemplateManager->LoadTemplate(ent, utf8_from_wstring(templateName));
	if (!tmpl)
		return INVALID_ENTITY; // LoadTemplate will have reported the error

	// This also ensures that ent does not exist
	CEntityHandle handle = AllocateEntityHandle(ent);

	// Construct a component for each child of the root element
	const CParamNode::ChildrenMap& tmplChilds = tmpl->GetChildren();
	for (CParamNode::ChildrenMap::const_iterator it = tmplChilds.begin(); it != tmplChilds.end(); ++it)
	{
		// Ignore attributes on the root element
		if (it->first.length() && it->first[0] == '@')
			continue;

		CComponentManager::ComponentTypeId cid = LookupCID(it->first);
		if (cid == CID__Invalid)
		{
			LOGERROR("Unrecognized component type name '%s' in entity template '%s'", it->first, utf8_from_wstring(templateName));
			return INVALID_ENTITY;
		}

		if (!AddComponent(handle, cid, it->second))
		{
			LOGERROR("Failed to construct component type name '%s' in entity template '%s'", it->first, utf8_from_wstring(templateName));
			return INVALID_ENTITY;
		}
		// TODO: maybe we should delete already-constructed components if one of them fails?
	}

	CMessageCreate msg(ent);
	PostMessage(ent, msg);

	return ent;
}

bool CComponentManager::EntityExists(entity_id_t ent) const
{
	return m_ComponentCaches.find(ent) != m_ComponentCaches.end();
}


void CComponentManager::DestroyComponentsSoon(entity_id_t ent)
{
	m_DestructionQueue.push_back(ent);
}

void CComponentManager::FlushDestroyedComponents()
{
	PROFILE2("Flush Destroyed Components");
	while (!m_DestructionQueue.empty())
	{
		// Make a copy of the destruction queue, so that the iterators won't be invalidated if the
		// CMessageDestroy handlers try to destroy more entities themselves
		std::vector<entity_id_t> queue;
		queue.swap(m_DestructionQueue);

		for (std::vector<entity_id_t>::iterator it = queue.begin(); it != queue.end(); ++it)
		{
			entity_id_t ent = *it;

			// Do nothing if invalid, destroyed, etc.
			if (!EntityExists(ent))
				continue;

			CEntityHandle handle = LookupEntityHandle(ent);

			CMessageDestroy msg(ent);
			PostMessage(ent, msg);

			// Flatten all the dynamic subscriptions to ensure there are no dangling
			// references in the 'removed' lists to components we're going to delete
			// Some components may have dynamically unsubscribed following the Destroy message
			FlattenDynamicSubscriptions();

			// Destroy the components, and remove from m_ComponentsByTypeId:
			std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::iterator iit = m_ComponentsByTypeId.begin();
			for (; iit != m_ComponentsByTypeId.end(); ++iit)
			{
				std::map<entity_id_t, IComponent*>::iterator eit = iit->second.find(ent);
				if (eit != iit->second.end())
				{
					eit->second->Deinit();
					RemoveComponentDynamicSubscriptions(eit->second);
					m_ComponentTypesById[iit->first].dealloc(eit->second);
					iit->second.erase(ent);
					handle.GetComponentCache()->interfaces[m_ComponentTypesById[iit->first].iid] = NULL;
				}
			}

			free(handle.GetComponentCache());
			m_ComponentCaches.erase(ent);

			// Remove from m_ComponentsByInterface
			std::vector<std::unordered_map<entity_id_t, IComponent*> >::iterator ifcit = m_ComponentsByInterface.begin();
			for (; ifcit != m_ComponentsByInterface.end(); ++ifcit)
			{
				ifcit->erase(ent);
			}
		}
	}
}

IComponent* CComponentManager::QueryInterface(entity_id_t ent, InterfaceId iid) const
{
	if ((size_t)iid >= m_ComponentsByInterface.size())
	{
		// Invalid iid
		return NULL;
	}

	std::unordered_map<entity_id_t, IComponent*>::const_iterator eit = m_ComponentsByInterface[iid].find(ent);
	if (eit == m_ComponentsByInterface[iid].end())
	{
		// This entity doesn't implement this interface
		return NULL;
	}

	return eit->second;
}

CComponentManager::InterfaceList CComponentManager::GetEntitiesWithInterface(InterfaceId iid) const
{
	std::vector<std::pair<entity_id_t, IComponent*> > ret;

	if ((size_t)iid >= m_ComponentsByInterface.size())
	{
		// Invalid iid
		return ret;
	}

	ret.reserve(m_ComponentsByInterface[iid].size());

	std::unordered_map<entity_id_t, IComponent*>::const_iterator it = m_ComponentsByInterface[iid].begin();
	for (; it != m_ComponentsByInterface[iid].end(); ++it)
		ret.push_back(*it);

	std::sort(ret.begin(), ret.end()); // lexicographic pair comparison means this'll sort by entity ID

	return ret;
}

static CComponentManager::InterfaceListUnordered g_EmptyEntityMap;
const CComponentManager::InterfaceListUnordered& CComponentManager::GetEntitiesWithInterfaceUnordered(InterfaceId iid) const
{
	if ((size_t)iid >= m_ComponentsByInterface.size())
	{
		// Invalid iid
		return g_EmptyEntityMap;
	}

	return m_ComponentsByInterface[iid];
}

void CComponentManager::PostMessage(entity_id_t ent, const CMessage& msg)
{
	PROFILE2_IFSPIKE("Post Message", 0.0005);
	PROFILE2_ATTR("%s", msg.GetScriptHandlerName());
	// Send the message to components of ent, that subscribed locally to this message
	std::map<MessageTypeId, std::vector<ComponentTypeId> >::const_iterator it;
	it = m_LocalMessageSubscriptions.find(msg.GetType());
	if (it != m_LocalMessageSubscriptions.end())
	{
		std::vector<ComponentTypeId>::const_iterator ctit = it->second.begin();
		for (; ctit != it->second.end(); ++ctit)
		{
			// Find the component instances of this type (if any)
			std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::const_iterator emap = m_ComponentsByTypeId.find(*ctit);
			if (emap == m_ComponentsByTypeId.end())
				continue;

			// Send the message to all of them
			std::map<entity_id_t, IComponent*>::const_iterator eit = emap->second.find(ent);
			if (eit != emap->second.end())
				eit->second->HandleMessage(msg, false);
		}
	}

	SendGlobalMessage(ent, msg);
}

void CComponentManager::BroadcastMessage(const CMessage& msg)
{
	// Send the message to components of all entities that subscribed locally to this message
	std::map<MessageTypeId, std::vector<ComponentTypeId> >::const_iterator it;
	it = m_LocalMessageSubscriptions.find(msg.GetType());
	if (it != m_LocalMessageSubscriptions.end())
	{
		std::vector<ComponentTypeId>::const_iterator ctit = it->second.begin();
		for (; ctit != it->second.end(); ++ctit)
		{
			// Find the component instances of this type (if any)
			std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::const_iterator emap = m_ComponentsByTypeId.find(*ctit);
			if (emap == m_ComponentsByTypeId.end())
				continue;

			// Send the message to all of them
			std::map<entity_id_t, IComponent*>::const_iterator eit = emap->second.begin();
			for (; eit != emap->second.end(); ++eit)
				eit->second->HandleMessage(msg, false);
		}
	}

	SendGlobalMessage(INVALID_ENTITY, msg);
}

void CComponentManager::SendGlobalMessage(entity_id_t ent, const CMessage& msg)
{
	PROFILE2_IFSPIKE("SendGlobalMessage", 0.001);
	PROFILE2_ATTR("%s", msg.GetScriptHandlerName());
	// (Common functionality for PostMessage and BroadcastMessage)

	// Send the message to components of all entities that subscribed globally to this message
	std::map<MessageTypeId, std::vector<ComponentTypeId> >::const_iterator it;
	it = m_GlobalMessageSubscriptions.find(msg.GetType());
	if (it != m_GlobalMessageSubscriptions.end())
	{
		std::vector<ComponentTypeId>::const_iterator ctit = it->second.begin();
		for (; ctit != it->second.end(); ++ctit)
		{
			// Special case: Messages for local entities shouldn't be sent to script
			// components that subscribed globally, so that we don't have to worry about
			// them accidentally picking up non-network-synchronised data.
			if (ENTITY_IS_LOCAL(ent))
			{
				std::map<ComponentTypeId, ComponentType>::const_iterator cit = m_ComponentTypesById.find(*ctit);
				if (cit != m_ComponentTypesById.end() && cit->second.type == CT_Script)
					continue;
			}

			// Find the component instances of this type (if any)
			std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::const_iterator emap = m_ComponentsByTypeId.find(*ctit);
			if (emap == m_ComponentsByTypeId.end())
				continue;

			// Send the message to all of them
			std::map<entity_id_t, IComponent*>::const_iterator eit = emap->second.begin();
			for (; eit != emap->second.end(); ++eit)
				eit->second->HandleMessage(msg, true);
		}
	}

	// Send the message to component instances that dynamically subscribed to this message
	std::map<MessageTypeId, CDynamicSubscription>::iterator dit = m_DynamicMessageSubscriptionsNonsync.find(msg.GetType());
	if (dit != m_DynamicMessageSubscriptionsNonsync.end())
	{
		dit->second.Flatten();
		const std::vector<IComponent*>& dynamic = dit->second.GetComponents();
		for (size_t i = 0; i < dynamic.size(); i++)
			dynamic[i]->HandleMessage(msg, false);
	}
}

std::string CComponentManager::GenerateSchema() const
{
	std::string schema =
		"<grammar xmlns='http://relaxng.org/ns/structure/1.0' xmlns:a='http://ns.wildfiregames.com/entity' datatypeLibrary='http://www.w3.org/2001/XMLSchema-datatypes'>"
			"<define name='decimal'>"
				"<data type='decimal'/>"
			"</define>"
			"<define name='nonNegativeDecimal'>"
				"<data type='decimal'><param name='minInclusive'>0</param></data>"
			"</define>"
			"<define name='positiveDecimal'>"
				"<data type='decimal'><param name='minExclusive'>0</param></data>"
			"</define>"
			"<define name='anything'>"
				"<zeroOrMore>"
					  "<choice>"
							"<attribute><anyName/></attribute>"
							"<text/>"
							"<element>"
								"<anyName/>"
								"<ref name='anything'/>"
							"</element>"
					  "</choice>"
				"</zeroOrMore>"
			"</define>";

	std::map<InterfaceId, std::vector<std::string> > interfaceComponentTypes;

	std::vector<std::string> componentTypes;

	for (std::map<ComponentTypeId, ComponentType>::const_iterator it = m_ComponentTypesById.begin(); it != m_ComponentTypesById.end(); ++it)
	{
		schema +=
			"<define name='component." + it->second.name + "'>"
				"<element name='" + it->second.name + "'>"
					"<interleave>" + it->second.schema + "</interleave>"
				"</element>"
			"</define>";

		interfaceComponentTypes[it->second.iid].push_back(it->second.name);
		componentTypes.push_back(it->second.name);
	}

	// Declare the implementation of each interface, for documentation
	for (std::map<std::string, InterfaceId>::const_iterator it = m_InterfaceIdsByName.begin(); it != m_InterfaceIdsByName.end(); ++it)
	{
		schema += "<define name='interface." + it->first + "'><choice>";
		std::vector<std::string>& cts = interfaceComponentTypes[it->second];
		for (size_t i = 0; i < cts.size(); ++i)
			schema += "<ref name='component." + cts[i] + "'/>";
		schema += "</choice></define>";
	}

	// List all the component types, in alphabetical order (to match the reordering performed by CParamNode).
	// (We do it this way, rather than <interleave>ing all the interface definitions (which would additionally perform
	// a check that we don't use multiple component types of the same interface in one file), because libxml2 gives
	// useless error messages in the latter case; this way lets it report the real error.)
	std::sort(componentTypes.begin(), componentTypes.end());
	schema +=
		"<start>"
			"<element>"
				"<anyName/>"
				"<optional><attribute name='parent'/></optional>";
	for (std::vector<std::string>::const_iterator it = componentTypes.begin(); it != componentTypes.end(); ++it)
		schema += "<optional><ref name='component." + *it + "'/></optional>";
	schema +=
		"</element>"
	"</start>";

	schema += "</grammar>";

	return schema;
}
