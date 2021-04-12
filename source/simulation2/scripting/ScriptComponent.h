/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_SCRIPTCOMPONENT
#define INCLUDED_SCRIPTCOMPONENT

#include "simulation2/system/Component.h"

#include "ps/CLogger.h"

class CComponentTypeScript
{
	NONCOPYABLE(CComponentTypeScript);
public:
	CComponentTypeScript(const ScriptInterface& scriptInterface, JS::HandleValue instance);

	JS::Value GetInstance() const { return m_Instance.get(); }

	void Init(const CParamNode& paramNode, entity_id_t ent);
	void Deinit();
	void HandleMessage(const CMessage& msg, bool global);

	void Serialize(ISerializer& serialize);
	void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize, entity_id_t ent);

	template<typename R, typename... Ts>
	R Call(const char* funcname, const Ts&... params) const
	{
		R ret;
		if (m_ScriptInterface.CallFunction(m_Instance, funcname, ret, params...))
			return ret;
		LOGERROR("Error calling component script function %s", funcname);
		return R();
	}

	// CallRef is mainly used for returning script values with correct stack rooting.
	template<typename R, typename... Ts>
	void CallRef(const char* funcname, R ret, const Ts&... params) const
	{
		if (!m_ScriptInterface.CallFunction(m_Instance, funcname, ret, params...))
			LOGERROR("Error calling component script function %s", funcname);
	}

	template<typename... Ts>
	void CallVoid(const char* funcname, const Ts&... params) const
	{
		if (!m_ScriptInterface.CallFunctionVoid(m_Instance, funcname, params...))
			LOGERROR("Error calling component script function %s", funcname);
	}

private:
	const ScriptInterface& m_ScriptInterface;
	JS::PersistentRootedValue m_Instance;
};

#define REGISTER_COMPONENT_SCRIPT_WRAPPER(cname) \
	void RegisterComponentType_##cname(CComponentManager& mgr) \
	{ \
		mgr.RegisterComponentTypeScriptWrapper(CCmp##cname::GetInterfaceId(), CID_##cname, CCmp##cname::Allocate, CCmp##cname::Deallocate, #cname, CCmp##cname::GetSchema()); \
		CCmp##cname::ClassInit(mgr); \
	}


#define DEFAULT_SCRIPT_WRAPPER(cname) \
	static void ClassInit(CComponentManager& UNUSED(componentManager)) { } \
	static IComponent* Allocate(const ScriptInterface& scriptInterface, JS::HandleValue instance) \
	{ \
		return new CCmp##cname(scriptInterface, instance); \
	} \
	static void Deallocate(IComponent* cmp) \
	{ \
		delete static_cast<CCmp##cname*> (cmp); \
	} \
	CCmp##cname(const ScriptInterface& scriptInterface, JS::HandleValue instance) : m_Script(scriptInterface, instance) { } \
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
	virtual JS::Value GetJSInstance() const \
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

#endif // INCLUDED_SCRIPTCOMPONENT
