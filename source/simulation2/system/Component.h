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

#ifndef INCLUDED_COMPONENT
#define INCLUDED_COMPONENT

#include "simulation2/system/CmpPtr.h"
#include "simulation2/system/Components.h"
#include "simulation2/system/ComponentManager.h"
#include "simulation2/system/IComponent.h"
#include "simulation2/system/ParamNode.h"
#include "simulation2/system/SimContext.h"
#include "simulation2/serialization/ISerializer.h"
#include "simulation2/serialization/IDeserializer.h"

#define REGISTER_COMPONENT_TYPE(cname) \
	void RegisterComponentType_##cname(CComponentManager& mgr) \
	{ \
		mgr.RegisterComponentType(CCmp##cname::GetInterfaceId(), CID_##cname, CCmp##cname::Allocate, CCmp##cname::Deallocate, #cname, CCmp##cname::GetSchema()); \
		CCmp##cname::ClassInit(mgr); \
	}

#define REGISTER_COMPONENT_SCRIPT_WRAPPER(cname) \
	void RegisterComponentType_##cname(CComponentManager& mgr) \
	{ \
		mgr.RegisterComponentTypeScriptWrapper(CCmp##cname::GetInterfaceId(), CID_##cname, CCmp##cname::Allocate, CCmp##cname::Deallocate, #cname, CCmp##cname::GetSchema()); \
		CCmp##cname::ClassInit(mgr); \
	}

#define DEFAULT_COMPONENT_ALLOCATOR(cname) \
	static IComponent* Allocate(ScriptInterface&, jsval) { return new CCmp##cname(); } \
	static void Deallocate(IComponent* cmp) { delete static_cast<CCmp##cname*> (cmp); } \
	virtual int GetComponentTypeId() const \
	{ \
		return CID_##cname; \
	}

#define DEFAULT_SCRIPT_WRAPPER(cname) \
	static void ClassInit(CComponentManager& UNUSED(componentManager)) { } \
	static IComponent* Allocate(ScriptInterface& scriptInterface, jsval instance) \
	{ \
		return new CCmp##cname(scriptInterface, instance); \
	} \
	static void Deallocate(IComponent* cmp) \
	{ \
		delete static_cast<CCmp##cname*> (cmp); \
	} \
	CCmp##cname(ScriptInterface& scriptInterface, jsval instance) : m_Script(scriptInterface, instance) { } \
	static std::string GetSchema() \
	{ \
		return "<a:component type='script-wrapper'/><empty/>"; \
	} \
	virtual void Init(const CParamNode& paramNode) \
	{ \
		m_Script.Init(paramNode, GetEntityId()); \
	} \
	virtual void Deinit() \
	{ \
		m_Script.Deinit(); \
	} \
	virtual void HandleMessage(const CMessage& msg, bool global) \
	{ \
		m_Script.HandleMessage(msg, global); \
	} \
	virtual void Serialize(ISerializer& serialize) \
	{ \
		m_Script.Serialize(serialize); \
	} \
	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize) \
	{ \
		m_Script.Deserialize(paramNode, deserialize, GetEntityId()); \
	} \
	virtual jsval GetJSInstance() const \
	{ \
		return m_Script.GetInstance(); \
	} \
	virtual int GetComponentTypeId() const \
	{ \
		return CID_##cname; \
	} \
	private: \
		CComponentTypeScript m_Script; \
	public:

#define DEFAULT_MOCK_COMPONENT() \
	virtual int GetComponentTypeId() const \
	{ \
		return -1; \
	} \
	virtual void Init(const CParamNode& UNUSED(paramNode)) \
	{ \
	} \
	virtual void Deinit() \
	{ \
	} \
	virtual void Serialize(ISerializer& UNUSED(serialize)) \
	{ \
	} \
	virtual void Deserialize(const CParamNode& UNUSED(paramNode), IDeserializer& UNUSED(deserialize)) \
	{ \
	} \

#endif // INCLUDED_COMPONENT
