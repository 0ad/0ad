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

#include "simulation2/system/Component.h"
#include "ICmpAIManager.h"

#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "simulation2/components/ICmpAIInterface.h"
#include "simulation2/components/ICmpAIProxy.h"
#include "simulation2/components/ICmpCommandQueue.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/serialization/DebugSerializer.h"
#include "simulation2/serialization/StdDeserializer.h"
#include "simulation2/serialization/StdSerializer.h"
#include "simulation2/serialization/SerializeTemplates.h"

/**
 * @file Player AI interface.
 *
 * AI is primarily scripted, and the CCmpAIManager component defined here
 * takes care of managing all the scripts.
 *
 * To avoid slow AI scripts causing jerky rendering, they are run in a background
 * thread (maintained by CAIWorker) so that it's okay if they take a whole simulation
 * turn before returning their results (though preferably they shouldn't use nearly
 * that much CPU).
 *
 * CCmpAIManager grabs the world state after each turn (making use of AIInterface.js
 * and AIProxy.js to decide what data to include) then passes it to CAIWorker.
 * The AI scripts will then run asynchronously and return a list of commands to execute.
 * Any attempts to read the command list (including indirectly via serialization)
 * will block until it's actually completed, so the rest of the engine should avoid
 * reading it for as long as possible.
 *
 * TODO: actually the thread isn't implemented yet, because performance hasn't been
 * sufficiently problematic to justify the complexity yet, but the CAIWorker interface
 * is designed to hopefully support threading when we want it.
 */

class CAIWorker
{
private:
	struct SAIPlayer
	{
		std::wstring aiName;
		player_id_t player;
		CScriptValRooted obj;
	};

	struct SCommands
	{
		player_id_t player;
		std::vector<CScriptValRooted> commands;
	};

public:
	struct SReturnedCommands
	{
		player_id_t player;
		std::vector<std::string> commands;
	};

	CAIWorker() :
		m_ScriptInterface("Engine", "AI"),
		m_CommandsComputed(true),
		m_CurrentlyComputingPlayer(-1)
	{
		m_ScriptInterface.SetCallbackData(static_cast<void*> (this));

		// TODO: ought to seed the RNG (in a network-synchronised way) before we use it
		m_ScriptInterface.ReplaceNondeterministicFunctions(m_RNG);

		m_ScriptInterface.RegisterFunction<void, CScriptValRooted, CAIWorker::PostCommand>("PostCommand");
	}

	~CAIWorker()
	{
		// Clear rooted script values before destructing the script interface
		m_EntityTemplates = CScriptValRooted();
		m_PlayerMetadata.clear();
		m_Players.clear();
		m_Commands.clear();
	}

	bool AddPlayer(const std::wstring& aiName, player_id_t player, bool callConstructor)
	{
		std::wstring path = L"simulation/ai/" + aiName + L"/data.json";
		CScriptValRooted metadata = LoadPlayerFiles(aiName, path);
		if (metadata.uninitialised())
		{
			LOGERROR(L"Failed to create AI player: can't find %ls", path.c_str());
			return false;
		}

		// Get the constructor name from the metadata
		std::string constructor;
		if (!m_ScriptInterface.GetProperty(metadata.get(), "constructor", constructor))
		{
			LOGERROR(L"Failed to create AI player: %ls: missing 'constructor'", path.c_str());
			return false;
		}

		// Get the constructor function from the loaded scripts
		CScriptVal ctor;
		if (!m_ScriptInterface.GetProperty(m_ScriptInterface.GetGlobalObject(), constructor.c_str(), ctor)
			|| ctor.undefined())
		{
			LOGERROR(L"Failed to create AI player: %ls: can't find constructor '%hs'", path.c_str(), constructor.c_str());
			return false;
		}

		CScriptVal obj;

		if (callConstructor)
		{
			// Set up the data to pass as the constructor argument
			CScriptVal settings;
			m_ScriptInterface.Eval(L"({})", settings);
			m_ScriptInterface.SetProperty(settings.get(), "player", player, false);
			m_ScriptInterface.SetProperty(settings.get(), "templates", m_EntityTemplates, false);

			obj = m_ScriptInterface.CallConstructor(ctor.get(), settings.get());
		}
		else
		{
			// For deserialization, we want to create the object with the correct prototype
			// but don't want to actually run the constructor again
			obj = m_ScriptInterface.NewObjectFromConstructor(ctor.get());
		}

		if (obj.undefined())
		{
			LOGERROR(L"Failed to create AI player: %ls: error calling constructor '%hs'", path.c_str(), constructor.c_str());
			return false;
		}

		SAIPlayer ai;
		ai.aiName = aiName;
		ai.player = player;
		ai.obj = CScriptValRooted(m_ScriptInterface.GetContext(), obj);
		m_Players.push_back(ai);
		return true;
	}

	void StartComputation(const std::string& gameState)
	{
		debug_assert(m_CommandsComputed);

		m_GameState = gameState;

		m_CommandsComputed = false;
	}

	void WaitToFinishComputation()
	{
		if (!m_CommandsComputed)
		{
			PerformComputation();
			m_CommandsComputed = true;
		}
	}

	void GetCommands(std::vector<SReturnedCommands>& commands)
	{
		WaitToFinishComputation();

		commands.clear();
		commands.resize(m_Commands.size());
		for (size_t i = 0; i < m_Commands.size(); ++i)
		{
			commands[i].player = m_Commands[i].player;
			commands[i].commands.resize(m_Commands[i].commands.size());
			for (size_t j = 0; j < m_Commands[i].commands.size(); ++j)
			{
				// Serialize the returned command, so that it's safe to transfer
				// across threads (in the future when we actually run AI in a
				// background thread)
				std::stringstream stream;
				CStdSerializer serializer(m_ScriptInterface, stream);
				serializer.ScriptVal("command", m_Commands[i].commands[j]);
				commands[i].commands[j] = stream.str();
			}
		}
	}

	void LoadEntityTemplates(const std::vector<std::pair<std::string, const CParamNode*> >& templates)
	{
		m_ScriptInterface.Eval("({})", m_EntityTemplates);

		for (size_t i = 0; i < templates.size(); ++i)
		{
			jsval val = templates[i].second->ToJSVal(m_ScriptInterface.GetContext(), false);
			m_ScriptInterface.SetProperty(m_EntityTemplates.get(), templates[i].first.c_str(), CScriptVal(val), true);
		}

		// Since the template data is shared between AI players, freeze it
		// to stop any of them changing it and confusing the other players
		m_ScriptInterface.FreezeObject(m_EntityTemplates.get(), true);
	}

	void Serialize(std::ostream& stream, bool isDebug)
	{
		WaitToFinishComputation();

		if (isDebug)
		{
			CDebugSerializer serializer(m_ScriptInterface, stream);
			serializer.Indent(4);
			SerializeState(serializer);
		}
		else
		{
			CStdSerializer serializer(m_ScriptInterface, stream);
			SerializeState(serializer);
		}
	}

	void SerializeState(ISerializer& serializer)
	{
		serializer.NumberU32_Unbounded("num ais", m_Players.size());

		for (size_t i = 0; i < m_Players.size(); ++i)
		{
			serializer.String("name", m_Players[i].aiName, 0, 256);
			serializer.NumberI32_Unbounded("player", m_Players[i].player);
			serializer.ScriptVal("data", m_Players[i].obj);
		}

		serializer.NumberU32_Unbounded("num ai commands", m_Commands.size());

		for (size_t i = 0; i < m_Commands.size(); ++i)
		{
			serializer.NumberI32_Unbounded("player", m_Commands[i].player);
			SerializeVector<SerializeScriptVal>()(serializer, "commands", m_Commands[i].commands);
		}
	}

	void Deserialize(std::istream& stream)
	{
		debug_assert(m_CommandsComputed); // deserializing while we're still actively computing would be bad

		CStdDeserializer deserializer(m_ScriptInterface, stream);

		m_PlayerMetadata.clear();
		m_Players.clear();
		m_Commands.clear();

		uint32_t numAis;
		deserializer.NumberU32_Unbounded("num ais", numAis);

		for (size_t i = 0; i < numAis; ++i)
		{
			std::wstring name;
			player_id_t player;
			deserializer.String("name", name, 0, 256);
			deserializer.NumberI32_Unbounded("player", player);
			if (!AddPlayer(name, player, false))
				throw PSERROR_Deserialize_ScriptError();

			// Use ScriptObjectAppend so we don't lose the carefully-constructed
			// prototype/parent of this object
			deserializer.ScriptObjectAppend("data", m_Players.back().obj.getRef());
		}

		uint32_t numCommands;
		deserializer.NumberU32_Unbounded("num ai commands", numCommands);

		m_Commands.resize(numCommands);
		for (size_t i = 0; i < numCommands; ++i)
		{
			deserializer.NumberI32_Unbounded("player", m_Commands[i].player);
			SerializeVector<SerializeScriptVal>()(deserializer, "commands", m_Commands[i].commands);
		}
	}

private:
	CScriptValRooted LoadPlayerFiles(const std::wstring& aiName, const std::wstring& path)
	{
		if (m_PlayerMetadata.find(path) == m_PlayerMetadata.end())
		{
			// Load and cache the AI player metadata
			m_PlayerMetadata[path] = m_ScriptInterface.ReadJSONFile(path);

			// TODO: includes

			// Load and execute *.js
			VfsPaths pathnames;
			fs_util::GetPathnames(g_VFS, L"simulation/ai/" + aiName + L"/", L"*.js", pathnames);
			for (VfsPaths::iterator it = pathnames.begin(); it != pathnames.end(); ++it)
			{
				m_ScriptInterface.LoadGlobalScriptFile(*it);
			}
		}

		return m_PlayerMetadata[path];
	}

	void PerformComputation()
	{
		PROFILE("AI compute");

		m_Commands.clear();

		// Deserialize the game state, to pass to the AI's HandleMessage
		CScriptVal state;

		{
//			TIMER(L"deserialize AI game state");
			std::stringstream stream(m_GameState);
			CStdDeserializer deserializer(m_ScriptInterface, stream);
			deserializer.ScriptVal("state", state);
		}

		m_ScriptInterface.FreezeObject(state.get(), true);

		m_Commands.resize(m_Players.size());

		for (size_t i = 0; i < m_Players.size(); ++i)
		{
			m_Commands[i].player = m_Players[i].player;

			m_CurrentlyComputingPlayer = i;
			m_ScriptInterface.CallFunctionVoid(m_Players[i].obj.get(), "HandleMessage", state);
			// (This script will probably call PostCommand)
		}

		m_CurrentlyComputingPlayer = -1;
	}

	static void PostCommand(void* cbdata, CScriptValRooted cmd)
	{
		CAIWorker* self = static_cast<CAIWorker*> (cbdata);

		debug_assert(self->m_CurrentlyComputingPlayer >= 0); // called outside of PerformComputation somehow

		self->m_Commands[self->m_CurrentlyComputingPlayer].commands.push_back(cmd);
	}

	ScriptInterface m_ScriptInterface;
	boost::rand48 m_RNG;

	CScriptValRooted m_EntityTemplates;
	std::map<std::wstring, CScriptValRooted> m_PlayerMetadata;
	std::vector<SAIPlayer> m_Players;

	std::string m_GameState;

	bool m_CommandsComputed;
	std::vector<SCommands> m_Commands;
	int m_CurrentlyComputingPlayer; // used so PostCommand knows what player the command is for
};



class CCmpAIManager : public ICmpAIManager
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(AIManager)

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& UNUSED(paramNode))
	{
		LoadEntityTemplates();
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// Because the AI worker uses its own ScriptInterface, we can't use the
		// ISerializer (which was initialised with the simulation ScriptInterface)
		// directly. So we'll just grab the ISerializer's stream and write to it
		// with an independent serializer.

		m_Worker.Serialize(serialize.GetStream(), serialize.IsDebug());
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(context, paramNode);

		m_Worker.Deserialize(deserialize.GetStream());
	}

	virtual void AddPlayer(std::wstring id, player_id_t player)
	{
		m_Worker.AddPlayer(id, player, true);
	}

	virtual void StartComputation()
	{
		PROFILE("AI setup");

		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();

		CmpPtr<ICmpAIInterface> cmpAIInterface(GetSimContext(), SYSTEM_ENTITY);
		debug_assert(!cmpAIInterface.null());

		// Get most of the game state from AIInterface
		CScriptVal state = cmpAIInterface->GetRepresentation();

		// Get entity state from each entity:

		CScriptVal entities;
		scriptInterface.Eval(L"({})", entities);

		const std::map<entity_id_t, IComponent*>& ents = GetSimContext().GetComponentManager().GetEntitiesWithInterface(IID_AIProxy);

		for (std::map<entity_id_t, IComponent*>::const_iterator it = ents.begin(); it != ents.end(); ++it)
		{
			// Skip local entities
			if (ENTITY_IS_LOCAL(it->first))
				continue;

			CScriptVal rep = static_cast<ICmpAIProxy*>(it->second)->GetRepresentation();

			scriptInterface.SetPropertyInt(entities.get(), it->first, rep, true);
		}

		// Add the entities state into the object returned by AIInterface
		scriptInterface.SetProperty(state.get(), "entities", entities, true);

		// Serialize the state representation, so that it's safe to transfer
		// across threads (in the future when we actually run AI in a
		// background thread)
		std::stringstream stream;
		CStdSerializer serializer(scriptInterface, stream);
		serializer.ScriptVal("state", state);

		m_Worker.StartComputation(stream.str());
	}

	virtual void PushCommands()
	{
		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();

		std::vector<CAIWorker::SReturnedCommands> commands;
		m_Worker.GetCommands(commands);

		CmpPtr<ICmpCommandQueue> cmpCommandQueue(GetSimContext(), SYSTEM_ENTITY);
		if (cmpCommandQueue.null())
			return;

		for (size_t i = 0; i < commands.size(); ++i)
		{
			for (size_t j = 0; j < commands[i].commands.size(); ++j)
			{
				std::stringstream stream(commands[i].commands[j]);
				CStdDeserializer deserializer(scriptInterface, stream);
				CScriptVal cmd;
				deserializer.ScriptVal("command", cmd);
				cmpCommandQueue->PushLocalCommand(commands[i].player, cmd);
			}
		}
	}

private:
	void LoadEntityTemplates()
	{
		TIMER(L"LoadEntityTemplates");

		CmpPtr<ICmpTemplateManager> cmpTemplateManager(GetSimContext(), SYSTEM_ENTITY);
		debug_assert(!cmpTemplateManager.null());

		std::vector<std::string> templateNames = cmpTemplateManager->FindAllTemplates(false);

		std::vector<std::pair<std::string, const CParamNode*> > templates;
		templates.reserve(templateNames.size());

		for (size_t i = 0; i < templateNames.size(); ++i)
		{
			const CParamNode* node = cmpTemplateManager->GetTemplate(templateNames[i]);
			if (node)
				templates.push_back(std::make_pair(templateNames[i], node));
		}

		m_Worker.LoadEntityTemplates(templates);
	}

	CAIWorker m_Worker;
};

REGISTER_COMPONENT_TYPE(AIManager)
