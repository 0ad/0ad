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
 * JS values are passed between the game and AI threads using ScriptInterface::StructuredClone.
 *
 * TODO: actually the thread isn't implemented yet, because performance hasn't been
 * sufficiently problematic to justify the complexity yet, but the CAIWorker interface
 * is designed to hopefully support threading when we want it.
 */

class CAIWorker
{
private:
	class CAIPlayer
	{
		NONCOPYABLE(CAIPlayer);
	public:
		CAIPlayer(CAIWorker& worker, const std::wstring& aiName, player_id_t player,
				const shared_ptr<ScriptRuntime>& runtime, boost::rand48& rng) :
			m_Worker(worker), m_AIName(aiName), m_Player(player), m_ScriptInterface("Engine", "AI", runtime)
		{
			m_ScriptInterface.SetCallbackData(static_cast<void*> (this));

			m_ScriptInterface.ReplaceNondeterministicFunctions(rng);

			m_ScriptInterface.RegisterFunction<void, std::wstring, CAIPlayer::IncludeModule>("IncludeModule");
			m_ScriptInterface.RegisterFunction<void, CScriptValRooted, CAIPlayer::PostCommand>("PostCommand");
		}

		~CAIPlayer()
		{
			// Clean up rooted objects before destroying their script context
			m_Obj = CScriptValRooted();
		}

		static void IncludeModule(void* cbdata, std::wstring name)
		{
			CAIPlayer* self = static_cast<CAIPlayer*> (cbdata);

			self->LoadScripts(name);
		}

		static void PostCommand(void* cbdata, CScriptValRooted cmd)
		{
			CAIPlayer* self = static_cast<CAIPlayer*> (cbdata);

			self->m_Commands.push_back(self->m_ScriptInterface.WriteStructuredClone(cmd.get()));
		}

		bool LoadScripts(const std::wstring& moduleName)
		{
			// Ignore modules that are already loaded
			if (m_LoadedModules.find(moduleName) != m_LoadedModules.end())
				return true;

			// Mark this as loaded, to prevent it recursively loading itself
			m_LoadedModules.insert(moduleName);

			// Load and execute *.js
			VfsPaths pathnames;
			fs_util::GetPathnames(g_VFS, L"simulation/ai/" + moduleName + L"/", L"*.js", pathnames);
			for (VfsPaths::iterator it = pathnames.begin(); it != pathnames.end(); ++it)
			{
				if (!m_ScriptInterface.LoadGlobalScriptFile(*it))
				{
					LOGERROR(L"Failed to load script %ls", it->string().c_str());
					return false;
				}
			}

			return true;
		}

		bool Initialise(bool callConstructor)
		{
			if (!LoadScripts(m_AIName))
				return false;

			std::wstring path = L"simulation/ai/" + m_AIName + L"/data.json";
			CScriptValRooted metadata = m_Worker.LoadMetadata(path);
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
				m_ScriptInterface.SetProperty(settings.get(), "player", m_Player, false);
				m_ScriptInterface.SetProperty(settings.get(), "templates", m_Worker.m_EntityTemplates, false);

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

			m_Obj = CScriptValRooted(m_ScriptInterface.GetContext(), obj);
			return true;
		}

		void Run(CScriptVal state)
		{
			m_Commands.clear();
			m_ScriptInterface.CallFunctionVoid(m_Obj.get(), "HandleMessage", state);
		}

		CAIWorker& m_Worker;
		std::wstring m_AIName;
		player_id_t m_Player;

		ScriptInterface m_ScriptInterface;
		CScriptValRooted m_Obj;
		std::vector<shared_ptr<ScriptInterface::StructuredClone> > m_Commands;
		std::set<std::wstring> m_LoadedModules;
	};

public:
	struct SCommandSets
	{
		player_id_t player;
		std::vector<shared_ptr<ScriptInterface::StructuredClone> > commands;
	};

	CAIWorker() :
		m_ScriptRuntime(ScriptInterface::CreateRuntime()),
		m_ScriptInterface("Engine", "AI", m_ScriptRuntime),
		m_CommandsComputed(true)
	{
		m_ScriptInterface.SetCallbackData(static_cast<void*> (this));

		// TODO: ought to seed the RNG (in a network-synchronised way) before we use it
		m_ScriptInterface.ReplaceNondeterministicFunctions(m_RNG);
	}

	~CAIWorker()
	{
		// Clear rooted script values before destructing the script interface
		m_EntityTemplates = CScriptValRooted();
		m_PlayerMetadata.clear();
		m_Players.clear();
	}

	bool AddPlayer(const std::wstring& aiName, player_id_t player, bool callConstructor)
	{
		shared_ptr<CAIPlayer> ai(new CAIPlayer(*this, aiName, player, m_ScriptRuntime, m_RNG));
		if (!ai->Initialise(callConstructor))
			return false;

		m_Players.push_back(ai);

		return true;
	}

	void StartComputation(const shared_ptr<ScriptInterface::StructuredClone>& gameState)
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

	void GetCommands(std::vector<SCommandSets>& commands)
	{
		WaitToFinishComputation();

		commands.clear();
		commands.resize(m_Players.size());
		for (size_t i = 0; i < m_Players.size(); ++i)
		{
			commands[i].player = m_Players[i]->m_Player;
			commands[i].commands = m_Players[i]->m_Commands;
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
			serializer.String("name", m_Players[i]->m_AIName, 0, 256);
			serializer.NumberI32_Unbounded("player", m_Players[i]->m_Player);
			serializer.ScriptVal("data", m_Players[i]->m_Obj);

			serializer.NumberU32_Unbounded("num commands", m_Players[i]->m_Commands.size());
			for (size_t j = 0; j < m_Players[i]->m_Commands.size(); ++j)
			{
				CScriptVal val = m_ScriptInterface.ReadStructuredClone(m_Players[i]->m_Commands[j]);
				serializer.ScriptVal("command", val);
			}
		}
	}

	void Deserialize(std::istream& stream)
	{
		debug_assert(m_CommandsComputed); // deserializing while we're still actively computing would be bad

		CStdDeserializer deserializer(m_ScriptInterface, stream);

		m_PlayerMetadata.clear();
		m_Players.clear();

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
			deserializer.ScriptObjectAppend("data", m_Players.back()->m_Obj.getRef());

			uint32_t numCommands;
			deserializer.NumberU32_Unbounded("num commands", numCommands);
			m_Players.back()->m_Commands.reserve(numCommands);
			for (size_t j = 0; j < numCommands; ++j)
			{
				CScriptVal val;
				deserializer.ScriptVal("command", val);
				m_Players.back()->m_Commands.push_back(m_ScriptInterface.WriteStructuredClone(val.get()));
			}
		}
	}

private:
	CScriptValRooted LoadMetadata(const std::wstring& path)
	{
		if (m_PlayerMetadata.find(path) == m_PlayerMetadata.end())
		{
			// Load and cache the AI player metadata
			m_PlayerMetadata[path] = m_ScriptInterface.ReadJSONFile(path);
		}

		return m_PlayerMetadata[path];
	}

	void PerformComputation()
	{
		PROFILE("AI compute");

		// Deserialize the game state, to pass to the AI's HandleMessage
		CScriptVal state = m_ScriptInterface.ReadStructuredClone(m_GameState);

		// It would be nice to do
		//   m_ScriptInterface.FreezeObject(state.get(), true);
		// to prevent AI scripts accidentally modifying the state and
		// affecting other AI scripts they share it with. But the performance
		// cost is far too high, so we won't do that.

		for (size_t i = 0; i < m_Players.size(); ++i)
			m_Players[i]->Run(state);
	}

	shared_ptr<ScriptRuntime> m_ScriptRuntime;
	ScriptInterface m_ScriptInterface;
	boost::rand48 m_RNG;

	CScriptValRooted m_EntityTemplates;
	std::map<std::wstring, CScriptValRooted> m_PlayerMetadata;
	std::vector<shared_ptr<CAIPlayer> > m_Players; // use shared_ptr just to avoid copying

	shared_ptr<ScriptInterface::StructuredClone> m_GameState;

	bool m_CommandsComputed;
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

		// Get the game state from AIInterface
		CScriptVal state = cmpAIInterface->GetRepresentation();

		m_Worker.StartComputation(scriptInterface.WriteStructuredClone(state.get()));
	}

	virtual void PushCommands()
	{
		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();

		std::vector<CAIWorker::SCommandSets> commands;
		m_Worker.GetCommands(commands);

		CmpPtr<ICmpCommandQueue> cmpCommandQueue(GetSimContext(), SYSTEM_ENTITY);
		if (cmpCommandQueue.null())
			return;

		for (size_t i = 0; i < commands.size(); ++i)
		{
			for (size_t j = 0; j < commands[i].commands.size(); ++j)
			{
				cmpCommandQueue->PushLocalCommand(commands[i].player,
					scriptInterface.ReadStructuredClone(commands[i].commands[j]));
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
