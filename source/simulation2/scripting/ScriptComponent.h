/* Copyright (C) 2017 Wildfire Games.
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

class CSimContext;
class CParamNode;
class ISerializer;
class IDeserializer;

class CComponentTypeScript
{
	NONCOPYABLE(CComponentTypeScript);
public:
	CComponentTypeScript(ScriptInterface& scriptInterface, JS::HandleValue instance);

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
	ScriptInterface& m_ScriptInterface;
	JS::PersistentRootedValue m_Instance;
	bool m_HasCustomSerialize;
	bool m_HasCustomDeserialize;
	bool m_HasNullSerialize;
};

#endif // INCLUDED_SCRIPTCOMPONENT
