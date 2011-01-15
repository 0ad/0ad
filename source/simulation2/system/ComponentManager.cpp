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

#include "precompiled.h"

#include "ComponentManager.h"

#include "IComponent.h"
#include "ParamNode.h"
#include "SimContext.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpTemplateManager.h"

#include "ps/CLogger.h"
#include "ps/Filesystem.h"

/**
 * Used for script-only message types.
 */
class CMessageScripted : public CMessage
{
public:
	virtual int GetType() const { return mtid; }
	virtual const char* GetScriptHandlerName() const { return handlerName.c_str(); }
	virtual const char* GetScriptGlobalHandlerName() const { return globalHandlerName.c_str(); }
	virtual jsval ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const { return msg.get(); }

	CMessageScripted(int mtid, const std::string& name, const CScriptValRooted& msg) :
		mtid(mtid), handlerName("On" + name), globalHandlerName("OnGlobal" + name), msg(msg)
	{
	}

	int mtid;
	std::string handlerName;
	std::string globalHandlerName;
	CScriptValRooted msg;
};

CComponentManager::CComponentManager(CSimContext& context, bool skipScriptFunctions) :
	m_NextScriptComponentTypeId(CID__LastNative),
	m_ScriptInterface("Engine", "Simulation", ScriptInterface::CreateRuntime()),
	m_SimContext(context), m_CurrentlyHotloading(false)
{
	context.SetComponentManager(this);

	m_ScriptInterface.SetCallbackData(static_cast<void*> (this));

	// TODO: ought to seed the RNG (in a network-synchronised way) before we use it
	m_ScriptInterface.ReplaceNondeterministicFunctions(m_RNG);

	// For component script tests, the test system sets up its own scripted implementation of
	// these functions, so we skip registering them here in those cases
	if (!skipScriptFunctions)
	{
		m_ScriptInterface.RegisterFunction<void, int, std::string, CScriptVal, CComponentManager::Script_RegisterComponentType> ("RegisterComponentType");
		m_ScriptInterface.RegisterFunction<void, std::string, CComponentManager::Script_RegisterInterface> ("RegisterInterface");
		m_ScriptInterface.RegisterFunction<void, std::string, CComponentManager::Script_RegisterMessageType> ("RegisterMessageType");
		m_ScriptInterface.RegisterFunction<void, std::string, CScriptVal, CComponentManager::Script_RegisterGlobal> ("RegisterGlobal");
		m_ScriptInterface.RegisterFunction<IComponent*, int, int, CComponentManager::Script_QueryInterface> ("QueryInterface");
		m_ScriptInterface.RegisterFunction<std::vector<int>, int, CComponentManager::Script_GetEntitiesWithInterface> ("GetEntitiesWithInterface");
		m_ScriptInterface.RegisterFunction<std::vector<IComponent*>, int, CComponentManager::Script_GetComponentsWithInterface> ("GetComponentsWithInterface");
		m_ScriptInterface.RegisterFunction<void, int, int, CScriptVal, CComponentManager::Script_PostMessage> ("PostMessage");
		m_ScriptInterface.RegisterFunction<void, int, CScriptVal, CComponentManager::Script_BroadcastMessage> ("BroadcastMessage");
		m_ScriptInterface.RegisterFunction<int, std::string, CComponentManager::Script_AddEntity> ("AddEntity");
		m_ScriptInterface.RegisterFunction<int, std::string, CComponentManager::Script_AddLocalEntity> ("AddLocalEntity");
		m_ScriptInterface.RegisterFunction<void, int, CComponentManager::Script_DestroyEntity> ("DestroyEntity");
		m_ScriptInterface.RegisterFunction<CScriptVal, std::string, CComponentManager::Script_ReadJSONFile> ("ReadJSONFile");
	}

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
	m_ScriptInterface.SetGlobal("SYSTEM_ENTITY", (int)SYSTEM_ENTITY);

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


bool CComponentManager::LoadScript(const std::wstring& filename, bool hotload)
{
	m_CurrentlyHotloading = hotload;
	CVFSFile file;
	PSRETURN loadOk = file.Load(g_VFS, filename);
	debug_assert(loadOk == PSRETURN_OK); // TODO
	std::wstring content(file.GetBuffer(), file.GetBuffer() + file.GetBufferSize()); // TODO: encodings etc
	bool ok = m_ScriptInterface.LoadScript(filename, content);
	return ok;
}

void CComponentManager::Script_RegisterComponentType(void* cbdata, int iid, std::string cname, CScriptVal ctor)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	// Find the C++ component that wraps the interface
	int cidWrapper = componentManager->GetScriptWrapper(iid);
	if (cidWrapper == CID__Invalid)
	{
		componentManager->m_ScriptInterface.ReportError("Invalid interface id");
		return;
	}
	const ComponentType& ctWrapper = componentManager->m_ComponentTypesById[cidWrapper];

	bool mustReloadComponents = false; // for hotloading

	ComponentTypeId cid = componentManager->LookupCID(cname);
	if (cid == CID__Invalid)
	{
		// Allocate a new cid number
		cid = componentManager->m_NextScriptComponentTypeId++;
		componentManager->m_ComponentTypeIdsByName[cname] = cid;
	}
	else
	{
		// Component type is already loaded, so do hotloading:

		if (!componentManager->m_CurrentlyHotloading)
		{
			componentManager->m_ScriptInterface.ReportError("Registering component type with already-registered name"); // TODO: report the actual name
			return;
		}

		const ComponentType& ctPrevious = componentManager->m_ComponentTypesById[cid];

		// We can only replace scripted component types, not native ones
		if (ctPrevious.type != CT_Script)
		{
			componentManager->m_ScriptInterface.ReportError("Hotloading script component type with same name as native component");
			return;
		}

		// We don't support changing the IID of a component type (it would require fiddling
		// around with m_ComponentsByInterface and being careful to guarantee uniqueness per entity)
		if (ctPrevious.iid != iid)
		{
			// ...though it only matters if any components exist with this type
			if (!componentManager->m_ComponentsByTypeId[cid].empty())
			{
				componentManager->m_ScriptInterface.ReportError("Hotloading script component type mustn't change interface ID");
				return;
			}
		}

		// Remove the old component type's message subscriptions
		std::map<MessageTypeId, std::vector<ComponentTypeId> >::iterator it;
		for (it = componentManager->m_LocalMessageSubscriptions.begin(); it != componentManager->m_LocalMessageSubscriptions.end(); ++it)
		{
			std::vector<ComponentTypeId>& types = it->second;
			std::vector<ComponentTypeId>::iterator ctit = find(types.begin(), types.end(), cid);
			if (ctit != types.end())
				types.erase(ctit);
		}
		for (it = componentManager->m_GlobalMessageSubscriptions.begin(); it != componentManager->m_GlobalMessageSubscriptions.end(); ++it)
		{
			std::vector<ComponentTypeId>& types = it->second;
			std::vector<ComponentTypeId>::iterator ctit = find(types.begin(), types.end(), cid);
			if (ctit != types.end())
				types.erase(ctit);
		}

		mustReloadComponents = true;
	}

	std::string schema = "<empty/>";
	{
		CScriptValRooted prototype;
		if (componentManager->m_ScriptInterface.GetProperty(ctor.get(), "prototype", prototype) &&
			componentManager->m_ScriptInterface.HasProperty(prototype.get(), "Schema"))
		{
			componentManager->m_ScriptInterface.GetProperty(prototype.get(), "Schema", schema);
		}
	}

	// Construct a new ComponentType, using the wrapper's alloc functions
	ComponentType ct = {
		CT_Script,
		iid,
		ctWrapper.alloc,
		ctWrapper.dealloc,
		cname,
		schema,
		CScriptValRooted(componentManager->m_ScriptInterface.GetContext(), ctor)
	};
	componentManager->m_ComponentTypesById[cid] = ct;

	componentManager->m_CurrentComponent = cid; // needed by Subscribe


	// Find all the ctor prototype's On* methods, and subscribe to the appropriate messages:

	CScriptVal proto;
	if (!componentManager->m_ScriptInterface.GetProperty(ctor.get(), "prototype", proto))
		return; // error

	std::vector<std::string> methods;
	if (!componentManager->m_ScriptInterface.EnumeratePropertyNamesWithPrefix(proto.get(), "On", methods))
		return; // error

	for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it)
	{
		std::string name = (*it).substr(2); // strip the "On" prefix

		// Handle "OnGlobalFoo" functions specially
		bool isGlobal = false;
		if (name.substr(0, 6) == "Global")
		{
			isGlobal = true;
			name = name.substr(6);
		}

		std::map<std::string, MessageTypeId>::const_iterator mit = componentManager->m_MessageTypeIdsByName.find(name);
		if (mit == componentManager->m_MessageTypeIdsByName.end())
		{
			std::string msg = "Registered component has unrecognised '" + *it + "' message handler method";
			componentManager->m_ScriptInterface.ReportError(msg.c_str());
			return;
		}

		if (isGlobal)
			componentManager->SubscribeGloballyToMessageType(mit->second);
		else
			componentManager->SubscribeToMessageType(mit->second);
	}

	componentManager->m_CurrentComponent = CID__Invalid;

	if (mustReloadComponents)
	{
		// For every script component with this cid, we need to switch its
		// prototype from the old constructor's prototype property to the new one's
		const std::map<entity_id_t, IComponent*>& comps = componentManager->m_ComponentsByTypeId[cid];
		std::map<entity_id_t, IComponent*>::const_iterator eit = comps.begin();
		for (; eit != comps.end(); ++eit)
		{
			jsval instance = eit->second->GetJSInstance();
			if (!JSVAL_IS_NULL(instance))
				componentManager->m_ScriptInterface.SetPrototype(instance, proto.get());
		}
	}
}

void CComponentManager::Script_RegisterInterface(void* cbdata, std::string name)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	std::map<std::string, InterfaceId>::iterator it = componentManager->m_InterfaceIdsByName.find(name);
	if (it != componentManager->m_InterfaceIdsByName.end())
	{
		// Redefinitions are fine (and just get ignored) when hotloading; otherwise
		// they're probably unintentional and should be reported
		if (!componentManager->m_CurrentlyHotloading)
			componentManager->m_ScriptInterface.ReportError("Registering interface with already-registered name"); // TODO: report the actual name
		return;
	}

	// IIDs start at 1, so size+1 is the next unused one
	size_t id = componentManager->m_InterfaceIdsByName.size() + 1;
	componentManager->m_InterfaceIdsByName[name] = (InterfaceId)id;
	componentManager->m_ScriptInterface.SetGlobal(("IID_" + name).c_str(), (int)id);
}

void CComponentManager::Script_RegisterMessageType(void* cbdata, std::string name)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	std::map<std::string, MessageTypeId>::iterator it = componentManager->m_MessageTypeIdsByName.find(name);
	if (it != componentManager->m_MessageTypeIdsByName.end())
	{
		// Redefinitions are fine (and just get ignored) when hotloading; otherwise
		// they're probably unintentional and should be reported
		if (!componentManager->m_CurrentlyHotloading)
			componentManager->m_ScriptInterface.ReportError("Registering message type with already-registered name"); // TODO: report the actual name
		return;
	}

	// MTIDs start at 1, so size+1 is the next unused one
	size_t id = componentManager->m_MessageTypeIdsByName.size() + 1;
	componentManager->RegisterMessageType((MessageTypeId)id, name.c_str());
	componentManager->m_ScriptInterface.SetGlobal(("MT_" + name).c_str(), (int)id);
}

void CComponentManager::Script_RegisterGlobal(void* cbdata, std::string name, CScriptVal value)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	// Set the value, and accept duplicates only if hotloading (otherwise it's an error,
	// in order to detect accidental duplicate definitions of globals)
	componentManager->m_ScriptInterface.SetGlobal(name.c_str(), value, componentManager->m_CurrentlyHotloading);
}

IComponent* CComponentManager::Script_QueryInterface(void* cbdata, int ent, int iid)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);
	IComponent* component = componentManager->QueryInterface((entity_id_t)ent, iid);
	return component;
}

std::vector<int> CComponentManager::Script_GetEntitiesWithInterface(void* cbdata, int iid)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	std::vector<int> ret;
	const std::map<entity_id_t, IComponent*>& ents = componentManager->GetEntitiesWithInterface(iid);
	for (std::map<entity_id_t, IComponent*>::const_iterator it = ents.begin(); it != ents.end(); ++it)
		ret.push_back(it->first); // TODO: maybe we should exclude local entities
	return ret;
}

std::vector<IComponent*> CComponentManager::Script_GetComponentsWithInterface(void* cbdata, int iid)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	std::vector<IComponent*> ret;
	const std::map<entity_id_t, IComponent*>& ents = componentManager->GetEntitiesWithInterface(iid);
	for (std::map<entity_id_t, IComponent*>::const_iterator it = ents.begin(); it != ents.end(); ++it)
		ret.push_back(it->second); // TODO: maybe we should exclude local entities
	return ret;
}

CMessage* CComponentManager::ConstructMessage(int mtid, CScriptVal data)
{
	if (mtid == MT__Invalid || mtid > (int)m_MessageTypeIdsByName.size()) // (IDs start at 1 so use '>' here)
		LOGERROR(L"PostMessage with invalid message type ID '%d'", mtid);

	if (mtid < MT__LastNative)
	{
		return CMessageFromJSVal(mtid, m_ScriptInterface, data.get());
	}
	else
	{
		return new CMessageScripted(mtid, m_MessageTypeNamesById[mtid],
				CScriptValRooted(m_ScriptInterface.GetContext(), data));
	}
}

void CComponentManager::Script_PostMessage(void* cbdata, int ent, int mtid, CScriptVal data)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	CMessage* msg = componentManager->ConstructMessage(mtid, data);
	if (!msg)
		return; // error

	componentManager->PostMessage(ent, *msg);

	delete msg;
}

void CComponentManager::Script_BroadcastMessage(void* cbdata, int mtid, CScriptVal data)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	CMessage* msg = componentManager->ConstructMessage(mtid, data);
	if (!msg)
		return; // error

	componentManager->BroadcastMessage(*msg);

	delete msg;
}

int CComponentManager::Script_AddEntity(void* cbdata, std::string templateName)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	std::wstring name(templateName.begin(), templateName.end());
	// TODO: should validate the string to make sure it doesn't contain scary characters
	// that will let it access non-component-template files

	entity_id_t ent = componentManager->AddEntity(name, componentManager->AllocateNewEntity());
	return (int)ent;
}

int CComponentManager::Script_AddLocalEntity(void* cbdata, std::string templateName)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	std::wstring name(templateName.begin(), templateName.end());
	// TODO: should validate the string to make sure it doesn't contain scary characters
	// that will let it access non-component-template files

	entity_id_t ent = componentManager->AddEntity(name, componentManager->AllocateNewLocalEntity());
	return (int)ent;
}

void CComponentManager::Script_DestroyEntity(void* cbdata, int ent)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	componentManager->DestroyComponentsSoon(ent);
}

void CComponentManager::ResetState()
{
	// Delete all IComponents
	std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::iterator iit = m_ComponentsByTypeId.begin();
	for (; iit != m_ComponentsByTypeId.end(); ++iit)
	{
		std::map<entity_id_t, IComponent*>::iterator eit = iit->second.begin();
		for (; eit != iit->second.end(); ++eit)
		{
			eit->second->Deinit(m_SimContext);
			m_ComponentTypesById[iit->first].dealloc(eit->second);
		}
	}

	m_ComponentsByInterface.clear();
	m_ComponentsByTypeId.clear();

	m_DestructionQueue.clear();

	// Reset IDs
	m_NextEntityId = SYSTEM_ENTITY + 1;
	m_NextLocalEntityId = FIRST_LOCAL_ENTITY;
}

void CComponentManager::RegisterComponentType(InterfaceId iid, ComponentTypeId cid, AllocFunc alloc, DeallocFunc dealloc,
		const char* name, const std::string& schema)
{
	ComponentType c = { CT_Native, iid, alloc, dealloc, name, schema, CScriptValRooted() };
	m_ComponentTypesById.insert(std::make_pair(cid, c));
	m_ComponentTypeIdsByName[name] = cid;
}

void CComponentManager::RegisterComponentTypeScriptWrapper(InterfaceId iid, ComponentTypeId cid, AllocFunc alloc,
		DeallocFunc dealloc, const char* name, const std::string& schema)
{
	ComponentType c = { CT_ScriptWrapper, iid, alloc, dealloc, name, schema, CScriptValRooted() };
	m_ComponentTypesById.insert(std::make_pair(cid, c));
	m_ComponentTypeIdsByName[name] = cid;
	// TODO: merge with RegisterComponentType
}

void CComponentManager::RegisterMessageType(MessageTypeId mtid, const char* name)
{
	m_MessageTypeIdsByName[name] = mtid;
	m_MessageTypeNamesById[mtid] = name;
}

void CComponentManager::SubscribeToMessageType(MessageTypeId mtid)
{
	// TODO: verify mtid
	debug_assert(m_CurrentComponent != CID__Invalid);
	std::vector<ComponentTypeId>& types = m_LocalMessageSubscriptions[mtid];
	types.push_back(m_CurrentComponent);
	std::sort(types.begin(), types.end()); // TODO: just sort once at the end of LoadComponents
}

void CComponentManager::SubscribeGloballyToMessageType(MessageTypeId mtid)
{
	// TODO: verify mtid
	debug_assert(m_CurrentComponent != CID__Invalid);
	std::vector<ComponentTypeId>& types = m_GlobalMessageSubscriptions[mtid];
	types.push_back(m_CurrentComponent);
	std::sort(types.begin(), types.end()); // TODO: just sort once at the end of LoadComponents
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
	LOGERROR(L"No script wrapper found for interface id %d", iid); // TODO: report name (if iid is valid at all)
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
	entity_id_t id = preferredId;

	// Ensure this ID won't be allocated again
	if (id >= m_NextEntityId)
		m_NextEntityId = id+1;
	// TODO: check for overflow

	return id;
}

bool CComponentManager::AddComponent(entity_id_t ent, ComponentTypeId cid, const CParamNode& paramNode)
{
	IComponent* component = ConstructComponent(ent, cid);
	if (!component)
		return false;

	component->Init(m_SimContext, paramNode);
	return true;
}

IComponent* CComponentManager::ConstructComponent(entity_id_t ent, ComponentTypeId cid)
{
	std::map<ComponentTypeId, ComponentType>::const_iterator it = m_ComponentTypesById.find(cid);
	if (it == m_ComponentTypesById.end())
	{
		LOGERROR(L"Invalid component id %d", cid);
		return NULL;
	}

	const ComponentType& ct = it->second;

	std::map<entity_id_t, IComponent*>& emap1 = m_ComponentsByInterface[ct.iid];
	if (emap1.find(ent) != emap1.end())
	{
		LOGERROR(L"Multiple components for interface %d", ct.iid);
		return NULL;
	}

	std::map<entity_id_t, IComponent*>& emap2 = m_ComponentsByTypeId[cid];

	// If this is a scripted component, construct the appropriate JS object first
	jsval obj = JSVAL_NULL;
	if (ct.type == CT_Script)
	{
		obj = m_ScriptInterface.CallConstructor(ct.ctor.get(), JSVAL_VOID);
		if (JSVAL_IS_VOID(obj))
		{
			LOGERROR(L"Script component constructor failed");
			return NULL;
		}
	}

	// Construct the new component
	IComponent* component = ct.alloc(m_ScriptInterface, obj);
	debug_assert(component);

	component->SetEntityId(ent);
	component->SetSimContext(m_SimContext);

	// Store a reference to the new component
	emap1.insert(std::make_pair(ent, component));
	emap2.insert(std::make_pair(ent, component));
	// TODO: We need to more careful about this - if an entity is constructed by a component
	// while we're iterating over all components, this will invalidate the iterators and everything
	// will break.
	// We probably need some kind of delayed addition, so they get pushed onto a queue and then
	// inserted into the world later on. (Be careful about immediation deletion in that case, too.)

	return component;
}

void CComponentManager::AddMockComponent(entity_id_t ent, InterfaceId iid, IComponent& component)
{
	// Just add it into the by-interface map, not the by-component-type map,
	// so it won't be considered for messages or deletion etc

	std::map<entity_id_t, IComponent*>& emap1 = m_ComponentsByInterface[iid];
	if (emap1.find(ent) != emap1.end())
		debug_warn(L"Multiple components for interface");
	emap1.insert(std::make_pair(ent, &component));
}

entity_id_t CComponentManager::AddEntity(const std::wstring& templateName, entity_id_t ent)
{
	ICmpTemplateManager *tempMan = static_cast<ICmpTemplateManager*> (QueryInterface(SYSTEM_ENTITY, IID_TemplateManager));
	if (!tempMan)
	{
		debug_warn(L"No ICmpTemplateManager loaded");
		return INVALID_ENTITY;
	}

	// TODO: should assert that ent doesn't exist

	const CParamNode* tmpl = tempMan->LoadTemplate(ent, CStr8(templateName), -1);
	if (!tmpl)
		return INVALID_ENTITY; // LoadTemplate will have reported the error

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
			LOGERROR(L"Unrecognised component type name '%hs' in entity template '%ls'", it->first.c_str(), templateName.c_str());
			return INVALID_ENTITY;
		}

		if (!AddComponent(ent, cid, it->second))
		{
			LOGERROR(L"Failed to construct component type name '%hs' in entity template '%ls'", it->first.c_str(), templateName.c_str());
			return INVALID_ENTITY;
		}

		// TODO: maybe we should delete already-constructed components if one of them fails?
	}

	CMessageCreate msg(ent);
	PostMessage(ent, msg);

	return ent;
}

void CComponentManager::DestroyComponentsSoon(entity_id_t ent)
{
	m_DestructionQueue.push_back(ent);
}

void CComponentManager::FlushDestroyedComponents()
{
	// Make a copy of the destruction queue, so that the iterators won't be invalidated if the
	// CMessageDestroy handlers try to destroy more entities themselves
	std::vector<entity_id_t> queue;
	queue.swap(m_DestructionQueue);

	for (std::vector<entity_id_t>::iterator it = queue.begin(); it != queue.end(); ++it)
	{
		entity_id_t ent = *it;

		CMessageDestroy msg(ent);
		PostMessage(ent, msg);

		// Destroy the components, and remove from m_ComponentsByTypeId:
		std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::iterator iit = m_ComponentsByTypeId.begin();
		for (; iit != m_ComponentsByTypeId.end(); ++iit)
		{
			std::map<entity_id_t, IComponent*>::iterator eit = iit->second.find(ent);
			if (eit != iit->second.end())
			{
				eit->second->Deinit(m_SimContext);
				m_ComponentTypesById[iit->first].dealloc(eit->second);
				iit->second.erase(ent);
			}
		}

		// Remove from m_ComponentsByInterface
		std::map<InterfaceId, std::map<entity_id_t, IComponent*> >::iterator ifcit = m_ComponentsByInterface.begin();
		for (; ifcit != m_ComponentsByInterface.end(); ++ifcit)
		{
			ifcit->second.erase(ent);
		}
	}
}

IComponent* CComponentManager::QueryInterface(entity_id_t ent, InterfaceId iid) const
{
	std::map<InterfaceId, std::map<entity_id_t, IComponent*> >::const_iterator iit = m_ComponentsByInterface.find(iid);
	if (iit == m_ComponentsByInterface.end())
	{
		// Invalid iid, or no entities implement this interface
		return NULL;
	}

	std::map<entity_id_t, IComponent*>::const_iterator eit = iit->second.find(ent);
	if (eit == iit->second.end())
	{
		// This entity doesn't implement this interface
		return NULL;
	}

	return eit->second;
}

static std::map<entity_id_t, IComponent*> g_EmptyEntityMap;
const std::map<entity_id_t, IComponent*>& CComponentManager::GetEntitiesWithInterface(InterfaceId iid) const
{
	std::map<InterfaceId, std::map<entity_id_t, IComponent*> >::const_iterator iit = m_ComponentsByInterface.find(iid);
	if (iit == m_ComponentsByInterface.end())
	{
		// Invalid iid, or no entities implement this interface
		return g_EmptyEntityMap;
	}

	return iit->second;
}

void CComponentManager::PostMessage(entity_id_t ent, const CMessage& msg) const
{
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
				eit->second->HandleMessage(m_SimContext, msg, false);
		}
	}

	SendGlobalMessage(ent, msg);
}

void CComponentManager::BroadcastMessage(const CMessage& msg) const
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
				eit->second->HandleMessage(m_SimContext, msg, false);
		}
	}

	SendGlobalMessage(INVALID_ENTITY, msg);
}

void CComponentManager::SendGlobalMessage(entity_id_t ent, const CMessage& msg) const
{
	// (Common functionality for PostMessage and BroadcastMessage)

	// Send the message to components of all entities that subscribed globally to this message
	std::map<MessageTypeId, std::vector<ComponentTypeId> >::const_iterator it;
	it = m_GlobalMessageSubscriptions.find(msg.GetType());
	if (it != m_GlobalMessageSubscriptions.end())
	{
		std::vector<ComponentTypeId>::const_iterator ctit = it->second.begin();
		for (; ctit != it->second.end(); ++ctit)
		{
			// Special case: Messages for non-local entities shouldn't be sent to script
			// components that subscribed globally, so that we don't have to worry about
			// them accidentally picking up non-network-synchronised data.
			if (ENTITY_IS_LOCAL(ent))
			{
				std::map<ComponentTypeId, ComponentType>::const_iterator it = m_ComponentTypesById.find(*ctit);
				if (it != m_ComponentTypesById.end() && it->second.type == CT_Script)
					continue;
			}

			// Find the component instances of this type (if any)
			std::map<ComponentTypeId, std::map<entity_id_t, IComponent*> >::const_iterator emap = m_ComponentsByTypeId.find(*ctit);
			if (emap == m_ComponentsByTypeId.end())
				continue;

			// Send the message to all of them
			std::map<entity_id_t, IComponent*>::const_iterator eit = emap->second.begin();
			for (; eit != emap->second.end(); ++eit)
				eit->second->HandleMessage(m_SimContext, msg, true);
		}
	}
}


std::string CComponentManager::GenerateSchema()
{
	std::string schema =
		"<grammar xmlns='http://relaxng.org/ns/structure/1.0' xmlns:a='http://ns.wildfiregames.com/entity' datatypeLibrary='http://www.w3.org/2001/XMLSchema-datatypes'>"
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
			"<element name='Entity'>"
				"<optional><attribute name='parent'/></optional>";
	for (std::vector<std::string>::const_iterator it = componentTypes.begin(); it != componentTypes.end(); ++it)
		schema += "<optional><ref name='component." + *it + "'/></optional>";
	schema +=
		"</element>"
	"</start>";

	schema += "</grammar>";

	return schema;
}

CScriptVal CComponentManager::Script_ReadJSONFile(void* cbdata, std::string fileName)
{
	CComponentManager* componentManager = static_cast<CComponentManager*> (cbdata);

	std::wstring path = L"simulation/data/" + CStrW(fileName);

	return componentManager->GetScriptInterface().ReadJSONFile(path).get();
}
