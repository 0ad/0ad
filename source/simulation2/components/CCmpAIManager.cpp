/* Copyright (C) 2014 Wildfire Games.
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

#include "simulation2/MessageTypes.h"

#include "graphics/Terrain.h"
#include "lib/timer.h"
#include "lib/tex/tex.h"
#include "lib/allocators/shared_ptr.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Util.h"
#include "simulation2/components/ICmpAIInterface.h"
#include "simulation2/components/ICmpCommandQueue.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/components/ICmpTechnologyTemplateManager.h"
#include "simulation2/components/ICmpTerritoryManager.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/serialization/DebugSerializer.h"
#include "simulation2/serialization/StdDeserializer.h"
#include "simulation2/serialization/StdSerializer.h"
#include "simulation2/serialization/SerializeTemplates.h"

/**
 * @file
 * Player AI interface.
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

/**
 * Implements worker thread for CCmpAIManager.
 */
class CAIWorker
{
private:
	class CAIPlayer
	{
		NONCOPYABLE(CAIPlayer);
	public:
		CAIPlayer(CAIWorker& worker, const std::wstring& aiName, player_id_t player, uint8_t difficulty,
				shared_ptr<ScriptInterface> scriptInterface) :
			m_Worker(worker), m_AIName(aiName), m_Player(player), m_Difficulty(difficulty), 
			m_ScriptInterface(scriptInterface), m_Obj(scriptInterface->GetJSRuntime())
		{
		}

		bool Initialise()
		{
			// LoadScripts will only load each script once even though we call it for each player
			if (!m_Worker.LoadScripts(m_AIName))
				return false;
				
			JSContext* cx = m_ScriptInterface->GetContext();
			JSAutoRequest rq(cx);

			OsPath path = L"simulation/ai/" + m_AIName + L"/data.json";
			JS::RootedValue metadata(cx);
			m_Worker.LoadMetadata(path, &metadata);
			if (metadata.isUndefined())
			{
				LOGERROR("Failed to create AI player: can't find %s", path.string8());
				return false;
			}

			// Get the constructor name from the metadata
			std::string moduleName;
			std::string constructor;
			JS::RootedValue objectWithConstructor(cx); // object that should contain the constructor function
			JS::RootedValue global(cx, m_ScriptInterface->GetGlobalObject());
			JS::RootedValue ctor(cx);
			if (!m_ScriptInterface->HasProperty(metadata, "moduleName"))
			{
				LOGERROR("Failed to create AI player: %s: missing 'moduleName'", path.string8());
				return false;
			}

			m_ScriptInterface->GetProperty(metadata, "moduleName", moduleName);
			if (!m_ScriptInterface->GetProperty(global, moduleName.c_str(), &objectWithConstructor)
			    || objectWithConstructor.isUndefined())
			{
				LOGERROR("Failed to create AI player: %s: can't find the module that should contain the constructor: '%s'", path.string8(), moduleName);
				return false;
			}

			if (!m_ScriptInterface->GetProperty(metadata, "constructor", constructor))
			{
				LOGERROR("Failed to create AI player: %s: missing 'constructor'", path.string8());
				return false;
			}

			// Get the constructor function from the loaded scripts
			if (!m_ScriptInterface->GetProperty(objectWithConstructor, constructor.c_str(), &ctor)
			    || ctor.isNull())
			{
				LOGERROR("Failed to create AI player: %s: can't find constructor '%s'", path.string8(), constructor);
				return false;
			}

			m_ScriptInterface->GetProperty(metadata, "useShared", m_UseSharedComponent);

			// Set up the data to pass as the constructor argument
			JS::RootedValue settings(cx);
			m_ScriptInterface->Eval(L"({})", &settings);
			m_ScriptInterface->SetProperty(settings, "player", m_Player, false);
			m_ScriptInterface->SetProperty(settings, "difficulty", m_Difficulty, false);
			ENSURE(m_Worker.m_HasLoadedEntityTemplates);
			m_ScriptInterface->SetProperty(settings, "templates", m_Worker.m_EntityTemplates, false);

			JS::AutoValueVector argv(cx);
			argv.append(settings.get());
			m_ScriptInterface->CallConstructor(ctor, argv, &m_Obj);

			if (m_Obj.get().isNull())
			{
				LOGERROR("Failed to create AI player: %s: error calling constructor '%s'", path.string8(), constructor);
				return false;
			}
			return true;
		}

		void Run(JS::HandleValue state, int playerID)
		{
			m_Commands.clear();
			m_ScriptInterface->CallFunctionVoid(m_Obj, "HandleMessage", state, playerID);
		}
		// overloaded with a sharedAI part.
		// javascript can handle both natively on the same function.
		void Run(JS::HandleValue state, int playerID, JS::HandleValue SharedAI)
		{
			m_Commands.clear();
			m_ScriptInterface->CallFunctionVoid(m_Obj, "HandleMessage", state, playerID, SharedAI);
		}
		void InitAI(JS::HandleValue state, JS::HandleValue SharedAI)
		{
			m_Commands.clear();
			m_ScriptInterface->CallFunctionVoid(m_Obj, "Init", state, m_Player, SharedAI);
		}

		CAIWorker& m_Worker;
		std::wstring m_AIName;
		player_id_t m_Player;
		uint8_t m_Difficulty;
		bool m_UseSharedComponent;
		
		// Take care to keep this declaration before heap rooted members. Destructors of heap rooted
		// members have to be called before the runtime destructor.
		shared_ptr<ScriptInterface> m_ScriptInterface;
		
		JS::PersistentRootedValue m_Obj;
		std::vector<shared_ptr<ScriptInterface::StructuredClone> > m_Commands;
	};

public:
	struct SCommandSets
	{
		player_id_t player;
		std::vector<shared_ptr<ScriptInterface::StructuredClone> > commands;
	};

	CAIWorker() :
		m_ScriptInterface(new ScriptInterface("Engine", "AI", g_ScriptRuntime)),
		m_TurnNum(0),
		m_CommandsComputed(true),
		m_HasLoadedEntityTemplates(false),
		m_HasSharedComponent(false),
		m_SerializablePrototypes(new ObjectIdCache<std::wstring>(g_ScriptRuntime)),
		m_EntityTemplates(g_ScriptRuntime->m_rt),
		m_TechTemplates(g_ScriptRuntime->m_rt),
		m_SharedAIObj(g_ScriptRuntime->m_rt),
		m_PassabilityMapVal(g_ScriptRuntime->m_rt),
		m_TerritoryMapVal(g_ScriptRuntime->m_rt)
	{

		m_ScriptInterface->ReplaceNondeterministicRNG(m_RNG);
		m_ScriptInterface->LoadGlobalScripts();

		m_ScriptInterface->SetCallbackData(static_cast<void*> (this));

		m_SerializablePrototypes->init();
		JS_AddExtraGCRootsTracer(m_ScriptInterface->GetJSRuntime(), Trace, this);

		m_ScriptInterface->RegisterFunction<void, int, JS::HandleValue, CAIWorker::PostCommand>("PostCommand");
		m_ScriptInterface->RegisterFunction<void, std::wstring, CAIWorker::IncludeModule>("IncludeModule");
		m_ScriptInterface->RegisterFunction<void, CAIWorker::DumpHeap>("DumpHeap");
		m_ScriptInterface->RegisterFunction<void, CAIWorker::ForceGC>("ForceGC");
		
		m_ScriptInterface->RegisterFunction<void, std::wstring, std::vector<u32>, u32, u32, u32, CAIWorker::DumpImage>("DumpImage");
	}

	~CAIWorker()
	{
		JS_RemoveExtraGCRootsTracer(m_ScriptInterface->GetJSRuntime(), Trace, this);
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
		if (vfs::GetPathnames(g_VFS, L"simulation/ai/" + moduleName + L"/", L"*.js", pathnames) < 0)
		{
			LOGERROR("Failed to load AI scripts for module %s", utf8_from_wstring(moduleName));
			return false;
		}

		for (VfsPaths::iterator it = pathnames.begin(); it != pathnames.end(); ++it)
		{
			if (!m_ScriptInterface->LoadGlobalScriptFile(*it))
			{
				LOGERROR("Failed to load script %s", it->string8());
				return false;
			}
		}

		return true;
	}
	
	static void IncludeModule(ScriptInterface::CxPrivate* pCxPrivate, std::wstring name)
	{
		ENSURE(pCxPrivate->pCBData);
		CAIWorker* self = static_cast<CAIWorker*> (pCxPrivate->pCBData);
		self->LoadScripts(name);
	}

	static void PostCommand(ScriptInterface::CxPrivate* pCxPrivate, int playerid, JS::HandleValue cmd)
	{
		ENSURE(pCxPrivate->pCBData);
		CAIWorker* self = static_cast<CAIWorker*> (pCxPrivate->pCBData);
		self->PostCommand(playerid, cmd);
	}
	
	void PostCommand(int playerid, JS::HandleValue cmd)
	{
		for (size_t i=0; i<m_Players.size(); i++)
		{
			if (m_Players[i]->m_Player == playerid)	
			{
				m_Players[i]->m_Commands.push_back(m_ScriptInterface->WriteStructuredClone(cmd));
				return;
			}
		}
		
		LOGERROR("Invalid playerid in PostCommand!");	
	}
	// The next two ought to be implmeneted someday but for now as it returns "null" it can't
	static void DumpHeap(ScriptInterface::CxPrivate* pCxPrivate)
	{
		pCxPrivate->pScriptInterface->DumpHeap();
	}
	static void ForceGC(ScriptInterface::CxPrivate* pCxPrivate)
	{
		PROFILE3("AI compute GC");
		JS_GC(pCxPrivate->pScriptInterface->GetJSRuntime());
	}
	
	/**
	 * Debug function for AI scripts to dump 2D array data (e.g. terrain tile weights).
	 */
	static void DumpImage(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring name, std::vector<u32> data, u32 w, u32 h, u32 max)
	{
		// TODO: this is totally not threadsafe.
		VfsPath filename = L"screenshots/aidump/" + name;
		
		if (data.size() != w*h)
		{
			debug_warn(L"DumpImage: data size doesn't match w*h");
			return;
		}
		
		if (max == 0)
		{
			debug_warn(L"DumpImage: max must not be 0");
			return;
		}
		
		const size_t bpp = 8;
		int flags = TEX_BOTTOM_UP|TEX_GREY;
		
		const size_t img_size = w * h * bpp/8;
		const size_t hdr_size = tex_hdr_size(filename);
		shared_ptr<u8> buf;
		AllocateAligned(buf, hdr_size+img_size, maxSectorSize);
		Tex t;
		if (t.wrap(w, h, bpp, flags, buf, hdr_size) < 0)
			return;
		
		u8* img = buf.get() + hdr_size;
		for (size_t i = 0; i < data.size(); ++i)
			img[i] = (u8)((data[i] * 255) / max);
		
		tex_write(&t, filename);
	}

	void SetRNGSeed(uint32_t seed)
	{
		m_RNG.seed(seed);
	}

	bool TryLoadSharedComponent(bool hasTechs)
	{
		JSContext* cx = m_ScriptInterface->GetContext();
		JSAutoRequest rq(cx);

		// we don't need to load it.
		if (!m_HasSharedComponent)
			return false;
		
		// reset the value so it can be used to determine if we actually initialized it.
		m_HasSharedComponent = false;
		
		if (LoadScripts(L"common-api"))
			m_HasSharedComponent = true;
		else
			return false;

		// mainly here for the error messages
		OsPath path = L"simulation/ai/common-api/";
		
		// Constructor name is SharedScript, it's in the module API3
		// TODO: Hardcoding this is bad, we need a smarter way. 
		JS::RootedValue AIModule(cx);
		JS::RootedValue global(cx, m_ScriptInterface->GetGlobalObject());
		JS::RootedValue ctor(cx);
		if (!m_ScriptInterface->GetProperty(global, "API3", &AIModule) || AIModule.isUndefined())
		{
			LOGERROR("Failed to create shared AI component: %s: can't find module '%s'", path.string8(), "API3");
			return false;
		}
		
		if (!m_ScriptInterface->GetProperty(AIModule, "SharedScript", &ctor)
		    || ctor.isUndefined())
		{
			LOGERROR("Failed to create shared AI component: %s: can't find constructor '%s'", path.string8(), "SharedScript");
			return false;
		}
		
		// Set up the data to pass as the constructor argument
		JS::RootedValue settings(cx);
		m_ScriptInterface->Eval(L"({})", &settings);
		JS::RootedValue playersID(cx);
		m_ScriptInterface->Eval(L"({})", &playersID);
		
		for (size_t i = 0; i < m_Players.size(); ++i)
		{
			JS::RootedValue val(cx);
			m_ScriptInterface->ToJSVal(cx, &val, m_Players[i]->m_Player);
			m_ScriptInterface->SetPropertyInt(playersID, i, val, true);
		}
		
		m_ScriptInterface->SetProperty(settings, "players", playersID);
		ENSURE(m_HasLoadedEntityTemplates);
		m_ScriptInterface->SetProperty(settings, "templates", m_EntityTemplates, false);
		
		if (hasTechs)
		{
			m_ScriptInterface->SetProperty(settings, "techTemplates", m_TechTemplates, false);
		}
		else
		{
			// won't get the tech templates directly.
			JS::RootedValue fakeTech(cx);
			m_ScriptInterface->Eval("({})", &fakeTech);
			m_ScriptInterface->SetProperty(settings, "techTemplates", fakeTech, false);
		}
		
		JS::AutoValueVector argv(cx);
		argv.append(settings);
		m_ScriptInterface->CallConstructor(ctor, argv, &m_SharedAIObj);
		
		if (m_SharedAIObj.get().isNull())
		{
			LOGERROR("Failed to create shared AI component: %s: error calling constructor '%s'", path.string8(), "SharedScript");
			return false;
		}
		
		return true;
	}

	bool AddPlayer(const std::wstring& aiName, player_id_t player, uint8_t difficulty)
	{
		shared_ptr<CAIPlayer> ai(new CAIPlayer(*this, aiName, player, difficulty, m_ScriptInterface));
		if (!ai->Initialise())
			return false;
		
		// this will be set to true if we need to load the shared Component.
		if (!m_HasSharedComponent)
			m_HasSharedComponent = ai->m_UseSharedComponent;

		m_Players.push_back(ai);

		return true;
	}

	bool RunGamestateInit(const shared_ptr<ScriptInterface::StructuredClone>& gameState, const Grid<u16>& passabilityMap, const Grid<u8>& territoryMap)
	{
		// this will be run last by InitGame.Js, passing the full game representation.
		// For now it will run for the shared Component.
		// This is NOT run during deserialization.
		JSContext* cx = m_ScriptInterface->GetContext();
		JSAutoRequest rq(cx);
		
		JS::RootedValue state(cx);
		m_ScriptInterface->ReadStructuredClone(gameState, &state);
		ScriptInterface::ToJSVal(cx, &m_PassabilityMapVal, passabilityMap);
		ScriptInterface::ToJSVal(cx, &m_TerritoryMapVal, territoryMap);
		
		if (m_HasSharedComponent)
		{
			m_ScriptInterface->SetProperty(state, "passabilityMap", m_PassabilityMapVal, true);
			m_ScriptInterface->SetProperty(state, "territoryMap", m_TerritoryMapVal, true);
			m_ScriptInterface->CallFunctionVoid(m_SharedAIObj, "init", state);
			
			for (size_t i = 0; i < m_Players.size(); ++i)
			{
				if (m_HasSharedComponent && m_Players[i]->m_UseSharedComponent)
					m_Players[i]->InitAI(state, m_SharedAIObj);
			}
		}
		
		return true;
	}
	void StartComputation(const shared_ptr<ScriptInterface::StructuredClone>& gameState, const Grid<u16>& passabilityMap, const Grid<u8>& territoryMap, bool territoryMapDirty)
	{
		ENSURE(m_CommandsComputed);

		m_GameState = gameState;
		JSContext* cx = m_ScriptInterface->GetContext();

		if (passabilityMap.m_DirtyID != m_PassabilityMap.m_DirtyID)
		{
			m_PassabilityMap = passabilityMap;
			ScriptInterface::ToJSVal(cx, &m_PassabilityMapVal, m_PassabilityMap);
		}

		if (territoryMapDirty)
		{
			m_TerritoryMap = territoryMap;
			ScriptInterface::ToJSVal(cx, &m_TerritoryMapVal, m_TerritoryMap);
		}

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

	void RegisterTechTemplates(const shared_ptr<ScriptInterface::StructuredClone>& techTemplates)
	{
		m_ScriptInterface->ReadStructuredClone(techTemplates, &m_TechTemplates);
	}
	
	void LoadEntityTemplates(const std::vector<std::pair<std::string, const CParamNode*> >& templates)
	{
		JSContext* cx = m_ScriptInterface->GetContext();
		JSAutoRequest rq(cx);

		m_HasLoadedEntityTemplates = true;

		m_ScriptInterface->Eval("({})", &m_EntityTemplates);

		JS::RootedValue val(cx);
		for (size_t i = 0; i < templates.size(); ++i)
		{
			templates[i].second->ToJSVal(cx, false, &val);
			m_ScriptInterface->SetProperty(m_EntityTemplates, templates[i].first.c_str(), val, true);
		}

		// Since the template data is shared between AI players, freeze it
		// to stop any of them changing it and confusing the other players
		m_ScriptInterface->FreezeObject(m_EntityTemplates, true);
	}

	void Serialize(std::ostream& stream, bool isDebug)
	{
		WaitToFinishComputation();

		if (isDebug)
		{
			CDebugSerializer serializer(*m_ScriptInterface, stream);
			serializer.Indent(4);
			SerializeState(serializer);
		}
		else
		{
			CStdSerializer serializer(*m_ScriptInterface, stream);
			// TODO: see comment in Deserialize()
			serializer.SetSerializablePrototypes(m_SerializablePrototypes);
			SerializeState(serializer);
		}
	}

	void SerializeState(ISerializer& serializer)
	{
		JSContext* cx = m_ScriptInterface->GetContext();
		JSAutoRequest rq(cx);
		
		std::stringstream rngStream;
		rngStream << m_RNG;
		serializer.StringASCII("rng", rngStream.str(), 0, 32);

		serializer.NumberU32_Unbounded("turn", m_TurnNum);

		serializer.NumberU32_Unbounded("num ais", (u32)m_Players.size());

		serializer.Bool("useSharedScript", m_HasSharedComponent);
		if (m_HasSharedComponent)
		{
			JS::RootedValue sharedData(cx);
			JS::RootedValue tmpSharedAIObj(cx, m_SharedAIObj.get()); // TODO: Check if this temporary root can be removed after SpiderMonkey 31 upgrade
			if (!m_ScriptInterface->CallFunction(tmpSharedAIObj, "Serialize", &sharedData))
				LOGERROR("AI shared script Serialize call failed");
			serializer.ScriptVal("sharedData", &sharedData);
		}
		for (size_t i = 0; i < m_Players.size(); ++i)
		{
			serializer.String("name", m_Players[i]->m_AIName, 1, 256);
			serializer.NumberI32_Unbounded("player", m_Players[i]->m_Player);
			serializer.NumberU8_Unbounded("difficulty", m_Players[i]->m_Difficulty);
			
			serializer.NumberU32_Unbounded("num commands", (u32)m_Players[i]->m_Commands.size());
			for (size_t j = 0; j < m_Players[i]->m_Commands.size(); ++j)
			{
				JS::RootedValue val(cx);
				m_ScriptInterface->ReadStructuredClone(m_Players[i]->m_Commands[j], &val);
				serializer.ScriptVal("command", &val);
			}

			JS::RootedValue tmpPlayerObj(cx, m_Players[i]->m_Obj.get()); // TODO: Check if this temporary root can be removed after SpiderMonkey 31 upgrade 
			bool hasCustomSerialize = m_ScriptInterface->HasProperty(tmpPlayerObj, "Serialize");
			if (hasCustomSerialize)
			{
				JS::RootedValue scriptData(cx);
				if (!m_ScriptInterface->CallFunction(tmpPlayerObj, "Serialize", &scriptData))
					LOGERROR("AI script Serialize call failed");
				serializer.ScriptVal("data", &scriptData);
			}
			else
			{
				serializer.ScriptVal("data", &tmpPlayerObj);
			}
		}
	}

	void Deserialize(std::istream& stream)
	{
		JSContext* cx = m_ScriptInterface->GetContext();
		JSAutoRequest rq(cx);
		
		ENSURE(m_CommandsComputed); // deserializing while we're still actively computing would be bad

		CStdDeserializer deserializer(*m_ScriptInterface, stream);

		m_PlayerMetadata.clear();
		m_Players.clear();

		std::string rngString;
		std::stringstream rngStream;
		deserializer.StringASCII("rng", rngString, 0, 32);
		rngStream << rngString;
		rngStream >> m_RNG;

		deserializer.NumberU32_Unbounded("turn", m_TurnNum);

		uint32_t numAis;
		deserializer.NumberU32_Unbounded("num ais", numAis);

		deserializer.Bool("useSharedScript", m_HasSharedComponent);
		TryLoadSharedComponent(false);
		if (m_HasSharedComponent)
		{
			JS::RootedValue sharedData(cx);
			JS::RootedValue tmpSharedAIObj(cx, m_SharedAIObj.get()); // TODO: Check if this temporary root can be removed after SpiderMonkey 31
			deserializer.ScriptVal("sharedData", &sharedData);
			if (!m_ScriptInterface->CallFunctionVoid(tmpSharedAIObj, "Deserialize", sharedData))
				LOGERROR("AI shared script Deserialize call failed");
		}

		for (size_t i = 0; i < numAis; ++i)
		{
			std::wstring name;
			player_id_t player;
			uint8_t difficulty;
			deserializer.String("name", name, 1, 256);
			deserializer.NumberI32_Unbounded("player", player);
			deserializer.NumberU8_Unbounded("difficulty",difficulty);
			if (!AddPlayer(name, player, difficulty))
				throw PSERROR_Deserialize_ScriptError();

			uint32_t numCommands;
			deserializer.NumberU32_Unbounded("num commands", numCommands);
			m_Players.back()->m_Commands.reserve(numCommands);
			for (size_t j = 0; j < numCommands; ++j)
			{
				JS::RootedValue val(cx);
				deserializer.ScriptVal("command", &val);
				m_Players.back()->m_Commands.push_back(m_ScriptInterface->WriteStructuredClone(val));
			}
			
			// TODO: this is yucky but necessary while the AIs are sharing data between contexts;
			//	ideally a new (de)serializer instance would be created for each player
			//	so they would have a single, consistent script context to use and serializable
			//	prototypes could be stored in their ScriptInterface
			deserializer.SetSerializablePrototypes(m_DeserializablePrototypes);

			bool hasCustomDeserialize = m_ScriptInterface->HasProperty(m_Players.back()->m_Obj, "Deserialize");
			if (hasCustomDeserialize)
			{
				JS::RootedValue scriptData(cx);
				deserializer.ScriptVal("data", &scriptData);
				if (m_Players[i]->m_UseSharedComponent)
				{
					if (!m_ScriptInterface->CallFunctionVoid(m_Players.back()->m_Obj, "Deserialize", scriptData, m_SharedAIObj))
						LOGERROR("AI script Deserialize call failed");
				}
				else if (!m_ScriptInterface->CallFunctionVoid(m_Players.back()->m_Obj, "Deserialize", scriptData))
				{
					LOGERROR("AI script deserialize() call failed");
				}
			}
			else
			{
				deserializer.ScriptVal("data", &m_Players.back()->m_Obj);
			}
		}
	}
	
	int getPlayerSize()
	{
		return m_Players.size();
	}

	void RegisterSerializablePrototype(std::wstring name, JS::HandleValue proto)
	{
		// Require unique prototype and name (for reverse lookup)
		// TODO: this is yucky - see comment in Deserialize()
		ENSURE(proto.isObject() && "A serializable prototype has to be an object!");
		JS::RootedObject obj(m_ScriptInterface->GetContext(), &proto.toObject());
		if (m_SerializablePrototypes->has(obj) || m_DeserializablePrototypes.find(name) != m_DeserializablePrototypes.end())
		{
			LOGERROR("RegisterSerializablePrototype called with same prototype multiple times: p=%p n='%s'", (void *)obj.get(), utf8_from_wstring(name));
			return;
		}
		m_SerializablePrototypes->add(m_ScriptInterface->GetContext(), obj, name);
		m_DeserializablePrototypes[name] = JS::Heap<JSObject*>(obj);
	}

private:
	static void Trace(JSTracer *trc, void *data)
	{
		reinterpret_cast<CAIWorker*>(data)->TraceMember(trc);
	}

	void TraceMember(JSTracer *trc)
	{
		for (auto& prototypes : m_DeserializablePrototypes)
			JS_CallHeapObjectTracer(trc, &prototypes.second, "CAIWorker::m_DeserializablePrototypes");
		for (auto& metadata : m_PlayerMetadata)
			JS_CallHeapValueTracer(trc, &metadata.second, "CAIWorker::m_PlayerMetadata");
	}

	void LoadMetadata(const VfsPath& path, JS::MutableHandleValue out)
	{
		if (m_PlayerMetadata.find(path) == m_PlayerMetadata.end())
		{
			// Load and cache the AI player metadata
			m_ScriptInterface->ReadJSONFile(path, out);
			m_PlayerMetadata[path] = JS::Heap<JS::Value>(out);
			return;
		}
		out.set(m_PlayerMetadata[path].get());
	}

	void PerformComputation()
	{			
		// Deserialize the game state, to pass to the AI's HandleMessage
		JSContext* cx = m_ScriptInterface->GetContext();
		JSAutoRequest rq(cx);
		JS::RootedValue state(cx);
		{
			PROFILE3("AI compute read state");
			m_ScriptInterface->ReadStructuredClone(m_GameState, &state);
			m_ScriptInterface->SetProperty(state, "passabilityMap", m_PassabilityMapVal, true);
			m_ScriptInterface->SetProperty(state, "territoryMap", m_TerritoryMapVal, true);
		}

		// It would be nice to do
		//   m_ScriptInterface->FreezeObject(state.get(), true);
		// to prevent AI scripts accidentally modifying the state and
		// affecting other AI scripts they share it with. But the performance
		// cost is far too high, so we won't do that.
		// If there is a shared component, run it

		if (m_HasSharedComponent)
		{
			PROFILE3("AI run shared component");
			JS::RootedValue tmpSharedAIObj(cx, m_SharedAIObj.get()); // TODO: Check if this temporary root can be removed after SpiderMonkey 31
			m_ScriptInterface->CallFunctionVoid(tmpSharedAIObj, "onUpdate", state);
		}
		
		for (size_t i = 0; i < m_Players.size(); ++i)
		{
			PROFILE3("AI script");
			PROFILE2_ATTR("player: %d", m_Players[i]->m_Player);
			PROFILE2_ATTR("script: %ls", m_Players[i]->m_AIName.c_str());

			if (m_HasSharedComponent && m_Players[i]->m_UseSharedComponent)
				m_Players[i]->Run(state, m_Players[i]->m_Player, m_SharedAIObj);
			else
				m_Players[i]->Run(state, m_Players[i]->m_Player);
		}
	}

	// Take care to keep this declaration before heap rooted members. Destructors of heap rooted
	// members have to be called before the runtime destructor.
	shared_ptr<ScriptRuntime> m_ScriptRuntime;
	
	shared_ptr<ScriptInterface> m_ScriptInterface;
	boost::rand48 m_RNG;
	u32 m_TurnNum;

	JS::PersistentRootedValue m_EntityTemplates;
	bool m_HasLoadedEntityTemplates;
	JS::PersistentRootedValue m_TechTemplates;

	std::map<VfsPath, JS::Heap<JS::Value> > m_PlayerMetadata;
	std::vector<shared_ptr<CAIPlayer> > m_Players; // use shared_ptr just to avoid copying

	bool m_HasSharedComponent;
	JS::PersistentRootedValue m_SharedAIObj;
	std::vector<SCommandSets> m_Commands;
	
	std::set<std::wstring> m_LoadedModules;
	
	shared_ptr<ScriptInterface::StructuredClone> m_GameState;
	Grid<u16> m_PassabilityMap;
	JS::PersistentRootedValue m_PassabilityMapVal;
	Grid<u8> m_TerritoryMap;
	JS::PersistentRootedValue m_TerritoryMapVal;

	bool m_CommandsComputed;

	shared_ptr<ObjectIdCache<std::wstring> > m_SerializablePrototypes;
	std::map<std::wstring, JS::Heap<JSObject*> > m_DeserializablePrototypes;
};


/**
 * Implementation of ICmpAIManager.
 */
class CCmpAIManager : public ICmpAIManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_ProgressiveLoad);
	}

	DEFAULT_COMPONENT_ALLOCATOR(AIManager)

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_TerritoriesDirtyID = 0;
		m_JustDeserialized = false;

		StartLoadEntityTemplates();
	}

	virtual void Deinit()
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

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);

		ForceLoadEntityTemplates();

		m_Worker.Deserialize(deserialize.GetStream());
		
		m_JustDeserialized = true;
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_ProgressiveLoad:
		{
			const CMessageProgressiveLoad& msgData = static_cast<const CMessageProgressiveLoad&> (msg);

			*msgData.total += (int)m_TemplateNames.size();

			if (*msgData.progressed)
				break;

			if (ContinueLoadEntityTemplates())
				*msgData.progressed = true;

			*msgData.progress += (int)m_TemplateLoadedIdx;

			break;
		}
		}
	}

	virtual void AddPlayer(std::wstring id, player_id_t player, uint8_t difficulty)
	{
		m_Worker.AddPlayer(id, player, difficulty);

		// AI players can cheat and see through FoW/SoD, since that greatly simplifies
		// their implementation.
		// (TODO: maybe cleverer AIs should be able to optionally retain FoW/SoD)
		CmpPtr<ICmpRangeManager> cmpRangeManager(GetSystemEntity());
		if (cmpRangeManager)
			cmpRangeManager->SetLosRevealAll(player, true);
	}
	
	virtual void SetRNGSeed(uint32_t seed)
	{
		m_Worker.SetRNGSeed(seed);
	}

	virtual void TryLoadSharedComponent()
	{
		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();
		JSContext* cx = scriptInterface.GetContext();
		JSAutoRequest rq(cx);
		
		// load the technology templates
		CmpPtr<ICmpTechnologyTemplateManager> cmpTechTemplateManager(GetSystemEntity());
		ENSURE(cmpTechTemplateManager);
		
		// Get the game state from AIInterface
		JS::RootedValue techTemplates(cx);
		cmpTechTemplateManager->GetAllTechs(&techTemplates);
		
		m_Worker.RegisterTechTemplates(scriptInterface.WriteStructuredClone(techTemplates));
		m_Worker.TryLoadSharedComponent(true);
	}

	virtual void RunGamestateInit()
	{
		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();
		JSContext* cx = scriptInterface.GetContext();
		JSAutoRequest rq(cx);
		
		CmpPtr<ICmpAIInterface> cmpAIInterface(GetSystemEntity());
		ENSURE(cmpAIInterface);
		
		// Get the game state from AIInterface
		// We flush events from the initialization so we get a clean state now.
		JS::RootedValue state(cx);
		cmpAIInterface->GetFullRepresentation(&state, true);

		// Get the passability data
		Grid<u16> dummyGrid;
		const Grid<u16>* passabilityMap = &dummyGrid;
		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (cmpPathfinder)
			passabilityMap = &cmpPathfinder->GetPassabilityGrid();
		
		// Get the territory data
		//	Since getting the territory grid can trigger a recalculation, we check NeedUpdate first
		Grid<u8> dummyGrid2;
		const Grid<u8>* territoryMap = &dummyGrid2;
		CmpPtr<ICmpTerritoryManager> cmpTerritoryManager(GetSystemEntity());
		if (cmpTerritoryManager && cmpTerritoryManager->NeedUpdate(&m_TerritoriesDirtyID))
		{
			territoryMap = &cmpTerritoryManager->GetTerritoryGrid();
		}
		
		LoadPathfinderClasses(state);

		m_Worker.RunGamestateInit(scriptInterface.WriteStructuredClone(state), *passabilityMap, *territoryMap);
	}

	virtual void StartComputation()
	{
		PROFILE("AI setup");

		ForceLoadEntityTemplates();

		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();
		JSContext* cx = scriptInterface.GetContext();
		JSAutoRequest rq(cx);

		if (m_Worker.getPlayerSize() == 0)
			return;
		
		CmpPtr<ICmpAIInterface> cmpAIInterface(GetSystemEntity());
		ENSURE(cmpAIInterface);

		// Get the game state from AIInterface
		JS::RootedValue state(cx);
		if (m_JustDeserialized)
			cmpAIInterface->GetFullRepresentation(&state, false);
		else
			cmpAIInterface->GetRepresentation(&state);

		// Get the passability data
		Grid<u16> dummyGrid;
		const Grid<u16>* passabilityMap = &dummyGrid;
		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (cmpPathfinder)
			passabilityMap = &cmpPathfinder->GetPassabilityGrid();

		// Get the territory data
		//	Since getting the territory grid can trigger a recalculation, we check NeedUpdate first
		bool territoryMapDirty = false;
		Grid<u8> dummyGrid2;
		const Grid<u8>* territoryMap = &dummyGrid2;
		CmpPtr<ICmpTerritoryManager> cmpTerritoryManager(GetSystemEntity());
		if (cmpTerritoryManager && cmpTerritoryManager->NeedUpdate(&m_TerritoriesDirtyID))
		{
			territoryMap = &cmpTerritoryManager->GetTerritoryGrid();
			territoryMapDirty = true;
		}

		LoadPathfinderClasses(state);

		m_Worker.StartComputation(scriptInterface.WriteStructuredClone(state), *passabilityMap, *territoryMap, territoryMapDirty);
		
		m_JustDeserialized = false;
	}

	virtual void PushCommands()
	{
		std::vector<CAIWorker::SCommandSets> commands;
		m_Worker.GetCommands(commands);

		CmpPtr<ICmpCommandQueue> cmpCommandQueue(GetSystemEntity());
		if (!cmpCommandQueue)
			return;
		
		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();
		JSContext* cx = scriptInterface.GetContext();
		JSAutoRequest rq(cx);
		JS::RootedValue clonedCommandVal(cx);
		
		for (size_t i = 0; i < commands.size(); ++i)
		{
			for (size_t j = 0; j < commands[i].commands.size(); ++j)
			{
				scriptInterface.ReadStructuredClone(commands[i].commands[j], &clonedCommandVal);
				cmpCommandQueue->PushLocalCommand(commands[i].player, clonedCommandVal);
			}
		}
	}

private:
	std::vector<std::string> m_TemplateNames;
	size_t m_TemplateLoadedIdx;
	std::vector<std::pair<std::string, const CParamNode*> > m_Templates;
	size_t m_TerritoriesDirtyID;

	bool m_JustDeserialized;

	void StartLoadEntityTemplates()
	{
		CmpPtr<ICmpTemplateManager> cmpTemplateManager(GetSystemEntity());
		ENSURE(cmpTemplateManager);

		m_TemplateNames = cmpTemplateManager->FindAllTemplates(false);
		m_TemplateLoadedIdx = 0;
		m_Templates.reserve(m_TemplateNames.size());
	}

	// Tries to load the next entity template. Returns true if we did some work.
	bool ContinueLoadEntityTemplates()
	{
		if (m_TemplateLoadedIdx >= m_TemplateNames.size())
			return false;

		CmpPtr<ICmpTemplateManager> cmpTemplateManager(GetSystemEntity());
		ENSURE(cmpTemplateManager);

		const CParamNode* node = cmpTemplateManager->GetTemplateWithoutValidation(m_TemplateNames[m_TemplateLoadedIdx]);
		if (node)
			m_Templates.push_back(std::make_pair(m_TemplateNames[m_TemplateLoadedIdx], node));

		m_TemplateLoadedIdx++;

		// If this was the last template, send the data to the worker
		if (m_TemplateLoadedIdx == m_TemplateNames.size())
			m_Worker.LoadEntityTemplates(m_Templates);

		return true;
	}

	void ForceLoadEntityTemplates()
	{
		while (ContinueLoadEntityTemplates())
		{
		}
	}

	void LoadPathfinderClasses(JS::HandleValue state)
	{
		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (!cmpPathfinder)
			return;

		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();
		JSContext* cx = scriptInterface.GetContext();
		JSAutoRequest rq(cx);

		JS::RootedValue classesVal(cx);
		scriptInterface.Eval("({ pathfinderObstruction: 1, foundationObstruction: 2 })", &classesVal);

		std::map<std::string, ICmpPathfinder::pass_class_t> classes = cmpPathfinder->GetPassabilityClasses();
		for (std::map<std::string, ICmpPathfinder::pass_class_t>::iterator it = classes.begin(); it != classes.end(); ++it)
			scriptInterface.SetProperty(classesVal, it->first.c_str(), it->second, true);

		scriptInterface.SetProperty(state, "passabilityClasses", classesVal, true);
	}

	CAIWorker m_Worker;
};

REGISTER_COMPONENT_TYPE(AIManager)
