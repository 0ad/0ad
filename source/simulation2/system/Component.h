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
		mgr.RegisterComponentType(CCmp##cname::GetInterfaceId(), CID_##cname, CCmp##cname::Allocate, CCmp##cname::Deallocate, #cname); \
		CCmp##cname::ClassInit(mgr); \
	}

#define REGISTER_COMPONENT_SCRIPT_WRAPPER(cname) \
	void RegisterComponentType_##cname(CComponentManager& mgr) \
	{ \
		mgr.RegisterComponentTypeScriptWrapper(CCmp##cname::GetInterfaceId(), CID_##cname, CCmp##cname::Allocate, CCmp##cname::Deallocate, #cname); \
		CCmp##cname::ClassInit(mgr); \
	}

#define DEFAULT_COMPONENT_ALLOCATOR(cname) \
	static IComponent* Allocate(ScriptInterface&, jsval) { return new CCmp##cname(); } \
	static void Deallocate(IComponent* cmp) { delete static_cast<CCmp##cname*> (cmp); } \

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
	virtual void Init(const CSimContext& context, const CParamNode& paramNode) \
	{ \
		m_Script.Init(context, paramNode, GetEntityId()); \
	} \
	virtual void Deinit(const CSimContext& context) \
	{ \
		m_Script.Deinit(context); \
	} \
	virtual void HandleMessage(const CSimContext& context, const CMessage& msg, bool global) \
	{ \
		m_Script.HandleMessage(context, msg, global); \
	} \
	virtual void Serialize(ISerializer& serialize) \
	{ \
		m_Script.Serialize(serialize); \
	} \
	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& deserialize) \
	{ \
		m_Script.Deserialize(context, paramNode, deserialize, GetEntityId()); \
	} \
	virtual jsval GetJSInstance() const \
	{ \
		return m_Script.GetInstance(); \
	} \
	private: \
	CComponentTypeScript m_Script; \
	public:

#define DEFAULT_MOCK_COMPONENT() \
	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& UNUSED(paramNode)) \
	{ \
	} \
	virtual void Deinit(const CSimContext& UNUSED(context)) \
	{ \
	} \
	virtual void Serialize(ISerializer& UNUSED(serialize)) \
	{ \
	} \
	virtual void Deserialize(const CSimContext& UNUSED(context), const CParamNode& UNUSED(paramNode), IDeserializer& UNUSED(deserialize)) \
	{ \
	} \

#endif // INCLUDED_COMPONENT
