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

#include "simulation2/system/Component.h"
#include "ICmpCommandQueue.h"

#include "ps/CLogger.h"

class CCmpCommandQueue : public ICmpCommandQueue
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(CommandQueue)

	struct Command
	{
		int player;
		CScriptValRooted data;
	};

	std::vector<Command> m_CmdQueue;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& UNUSED(paramNode))
	{
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		serialize.NumberU32_Unbounded("num commands", (u32)m_CmdQueue.size());
		for (size_t i = 0; i < m_CmdQueue.size(); ++i)
		{
			serialize.NumberI32_Unbounded("player", m_CmdQueue[i].player);
			serialize.ScriptVal("data", m_CmdQueue[i].data.get());
		}
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& UNUSED(paramNode), IDeserializer& deserialize)
	{
		JSContext* cx = context.GetScriptInterface().GetContext();

		u32 numCmds;
		deserialize.NumberU32_Unbounded(numCmds);
		for (size_t i = 0; i < numCmds; ++i)
		{
			i32 player;
			jsval data;
			deserialize.NumberI32_Unbounded(player);
			deserialize.ScriptVal(data);
			Command c = { player, CScriptValRooted(cx, data) };
			m_CmdQueue.push_back(c);
		}
	}

	virtual void PushClientCommand(int player, CScriptVal cmd)
	{
		JSContext* cx = GetSimContext().GetScriptInterface().GetContext();

		Command c = { player, CScriptValRooted(cx, cmd) };
		m_CmdQueue.push_back(c);
	}

	virtual void ProcessCommands()
	{
		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();

		for (size_t i = 0; i < m_CmdQueue.size(); ++i)
		{
			bool ok = scriptInterface.CallFunctionVoid(scriptInterface.GetGlobalObject(), "ProcessCommand", m_CmdQueue[i].player, m_CmdQueue[i].data);
			if (!ok)
				LOGERROR(L"Failed to call ProcessCommand() global script function");
		}

		m_CmdQueue.clear();
	}
};

REGISTER_COMPONENT_TYPE(CommandQueue)
