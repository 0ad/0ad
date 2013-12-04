/* Copyright (C) 2013 Wildfire Games.
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
				const shared_ptr<ScriptRuntime>& runtime, boost::rand48& rng) :
			m_Worker(worker), m_AIName(aiName), m_Player(player), m_Difficulty(difficulty), m_ScriptInterface("Engine", "AI", runtime)
		{
			m_ScriptInterface.SetCallbackData(static_cast<void*> (this));

			m_ScriptInterface.ReplaceNondeterministicRNG(rng);
			m_ScriptInterface.LoadGlobalScripts();

			m_ScriptInterface.RegisterFunction<void, std::wstring, CAIPlayer::IncludeModule>("IncludeModule");
			m_ScriptInterface.RegisterFunction<void, CAIPlayer::DumpHeap>("DumpHeap");
			m_ScriptInterface.RegisterFunction<void, CAIPlayer::ForceGC>("ForceGC");
			m_ScriptInterface.RegisterFunction<void, CScriptValRooted, CAIPlayer::PostCommand>("PostCommand");

			m_ScriptInterface.RegisterFunction<void, std::wstring, std::vector<u32>, u32, u32, u32, CAIPlayer::DumpImage>("DumpImage");

			m_ScriptInterface.RegisterFunction<void, std::wstring, CScriptVal, CAIPlayer::RegisterSerializablePrototype>("RegisterSerializablePrototype");
		}

		~CAIPlayer()
		{
			// Clean up rooted objects before destroying their script context
			m_Obj = CScriptValRooted();
			m_Commands.clear();
		}

		static void IncludeModule(void* cbdata, std::wstring name)
		{
			CAIPlayer* self = static_cast<CAIPlayer*> (cbdata);

			self->LoadScripts(name);
		}
		static void DumpHeap(void* cbdata)
		{
			CAIPlayer* self = static_cast<CAIPlayer*> (cbdata);
			
			//std::cout << JS_GetGCParameter(self->m_ScriptInterface.GetRuntime(), JSGC_BYTES) << std::endl;
			self->m_ScriptInterface.DumpHeap();
		}
		static void ForceGC(void* cbdata)
		{
			CAIPlayer* self = static_cast<CAIPlayer*> (cbdata);
			
			JS_GC(self->m_ScriptInterface.GetContext());
		}		
		static void PostCommand(void* cbdata, CScriptValRooted cmd)
		{
			CAIPlayer* self = static_cast<CAIPlayer*> (cbdata);

			self->m_Commands.push_back(self->m_ScriptInterface.WriteStructuredClone(cmd.get()));
		}

		/**
		 * Debug function for AI scripts to dump 2D array data (e.g. terrain tile weights).
		 * TODO: check if this needs to be here too.
		 */
		static void DumpImage(void* UNUSED(cbdata), std::wstring name, std::vector<u32> data, u32 w, u32 h, u32 max)
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
			if (tex_wrap(w, h, bpp, flags, buf, hdr_size, &t) < 0)
				return;

			u8* img = buf.get() + hdr_size;
			for (size_t i = 0; i < data.size(); ++i)
				img[i] = (u8)((data[i] * 255) / max);

			tex_write(&t, filename);
			tex_free(&t);
		}

		static void RegisterSerializablePrototype(void* cbdata, std::wstring name, CScriptVal proto)
		{
			CAIPlayer* self = static_cast<CAIPlayer*> (cbdata);
			// Add our player number to avoid name conflicts with other prototypes
			// TODO: it would be better if serializable prototypes were stored in ScriptInterfaces
			//	and then each serializer would access those matching its own context, but that's
			//	not possible with AIs sharing data across contexts
			std::wstringstream protoID;
			protoID << self->m_Player << L"." << name.c_str();
			self->m_Worker.RegisterSerializablePrototype(protoID.str(), proto);
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
			vfs::GetPathnames(g_VFS, L"simulation/ai/" + moduleName + L"/", L"*.js", pathnames);
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

			OsPath path = L"simulation/ai/" + m_AIName + L"/data.json";
			CScriptValRooted metadata = m_Worker.LoadMetadata(path);
			if (metadata.uninitialised())
			{
				LOGERROR(L"Failed to create AI player: can't find %ls", path.string().c_str());
				return false;
			}

			// Get the constructor name from the metadata
			std::string constructor;
			if (!m_ScriptInterface.GetProperty(metadata.get(), "constructor", constructor))
			{
				LOGERROR(L"Failed to create AI player: %ls: missing 'constructor'", path.string().c_str());
				return false;
			}

			// Get the constructor function from the loaded scripts
			CScriptVal ctor;
			if (!m_ScriptInterface.GetProperty(m_ScriptInterface.GetGlobalObject(), constructor.c_str(), ctor)
				|| ctor.undefined())
			{
				LOGERROR(L"Failed to create AI player: %ls: can't find constructor '%hs'", path.string().c_str(), constructor.c_str());
				return false;
			}

			m_ScriptInterface.GetProperty(metadata.get(), "useShared", m_UseSharedComponent);
			
			CScriptVal obj;

			if (callConstructor)
			{
				// Set up the data to pass as the constructor argument
				CScriptVal settings;
				m_ScriptInterface.Eval(L"({})", settings);
				m_ScriptInterface.SetProperty(settings.get(), "player", m_Player, false);
				m_ScriptInterface.SetProperty(settings.get(), "difficulty", m_Difficulty, false);
				ENSURE(m_Worker.m_HasLoadedEntityTemplates);
				m_ScriptInterface.SetProperty(settings.get(), "templates", m_Worker.m_EntityTemplates, false);

				obj = m_ScriptInterface.CallConstructor(ctor.get(), settings.get());
			}
			else
			{
				// For deserialization, we want to create the object with the correct prototype
				// but don't want to actually run the constructor again
				// XXX: actually we don't currently use this path for deserialization - maybe delete it?
				obj = m_ScriptInterface.NewObjectFromConstructor(ctor.get());
			}

			if (obj.undefined())
			{
				LOGERROR(L"Failed to create AI player: %ls: error calling constructor '%hs'", path.string().c_str(), constructor.c_str());
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
		// overloaded with a sharedAI part.
		// javascript can handle both natively on the same function.
		void Run(CScriptVal state, CScriptValRooted SharedAI)
		{
			m_Commands.clear();
			m_ScriptInterface.CallFunctionVoid(m_Obj.get(), "HandleMessage", state, SharedAI);
		}
		void InitWithSharedScript(CScriptVal state, CScriptValRooted SharedAI)
		{
			m_Commands.clear();
			m_ScriptInterface.CallFunctionVoid(m_Obj.get(), "InitWithSharedScript", state, SharedAI);
		}

		CAIWorker& m_Worker;
		std::wstring m_AIName;
		player_id_t m_Player;
		uint8_t m_Difficulty;
		bool m_UseSharedComponent;
		
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
		// TODO: Passing a 32 MB argument to CreateRuntime() is a temporary fix
		// to prevent frequent AI out-of-memory crashes. The argument should be
		// removed as soon whenever the new pathfinder is committed
		// And the AIs can stop relying on their own little hands.
		m_ScriptRuntime(ScriptInterface::CreateRuntime(33554432)),
		m_ScriptInterface("Engine", "AI", m_ScriptRuntime),
		m_TurnNum(0),
		m_CommandsComputed(true),
		m_HasLoadedEntityTemplates(false),
		m_HasSharedComponent(false)
	{

		// TODO: ought to seed the RNG (in a network-synchronised way) before we use it
		m_ScriptInterface.ReplaceNondeterministicRNG(m_RNG);
		m_ScriptInterface.LoadGlobalScripts();

		m_ScriptInterface.SetCallbackData(NULL);

		m_ScriptInterface.RegisterFunction<void, CScriptValRooted, CAIWorker::PostCommand>("PostCommand");
		m_ScriptInterface.RegisterFunction<void, CAIWorker::DumpHeap>("DumpHeap");
		m_ScriptInterface.RegisterFunction<void, CAIWorker::ForceGC>("ForceGC");
		
		m_ScriptInterface.RegisterFunction<void, std::wstring, std::vector<u32>, u32, u32, u32, CAIWorker::DumpImage>("DumpImage");
	}

	~CAIWorker()
	{
		// Clear rooted script values before destructing the script interface
		m_EntityTemplates = CScriptValRooted();
		m_PlayerMetadata.clear();
		m_Players.clear();
		m_GameState.reset();
		m_PassabilityMapVal = CScriptValRooted();
		m_TerritoryMapVal = CScriptValRooted();
	}

	// This is called by AIs if they use the v3 API.
	// If the AIs originate the call, cbdata is not NULL.
	// If the shared component does, it is, so it must not be taken into account.
	static void PostCommand(void* cbdata, CScriptValRooted cmd)
	{
		if (cbdata == NULL) {
			debug_warn(L"Warning: the shared component has tried to push an engine command. Ignoring.");
			return;
		}
		CAIPlayer* self = static_cast<CAIPlayer*> (cbdata);
		self->m_Commands.push_back(self->m_ScriptInterface.WriteStructuredClone(cmd.get()));
	}
	// The next two ought to be implmeneted someday but for now as it returns "null" it can't
	static void DumpHeap(void* cbdata)
	{
		if (cbdata == NULL) {
			debug_warn(L"Warning: the shared component has asked for DumpHeap. Ignoring.");
			return;
		}
		CAIWorker* self = static_cast<CAIWorker*> (cbdata);
		self->m_ScriptInterface.DumpHeap();
	}
	static void ForceGC(void* cbdata)
	{
		if (cbdata == NULL) {
			debug_warn(L"Warning: the shared component has asked for ForceGC. Ignoring.");
			return;
		}
		CAIWorker* self = static_cast<CAIWorker*> (cbdata);
		PROFILE3("AI compute GC");
		JS_GC(self->m_ScriptInterface.GetContext());
	}
	
	/**
	 * Debug function for AI scripts to dump 2D array data (e.g. terrain tile weights).
	 */
	static void DumpImage(void* UNUSED(cbdata), std::wstring name, std::vector<u32> data, u32 w, u32 h, u32 max)
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
		if (tex_wrap(w, h, bpp, flags, buf, hdr_size, &t) < 0)
			return;
		
		u8* img = buf.get() + hdr_size;
		for (size_t i = 0; i < data.size(); ++i)
			img[i] = (u8)((data[i] * 255) / max);
		
		tex_write(&t, filename);
		tex_free(&t);
	}

	bool TryLoadSharedComponent(bool hasTechs)
	{
		// we don't need to load it.
		if (!m_HasSharedComponent)
			return false;
		
		// reset the value so it can be used to determine if we actually initialized it.
		m_HasSharedComponent = false;
				
		VfsPaths sharedPathnames;
		// Check for "shared" module.
		vfs::GetPathnames(g_VFS, L"simulation/ai/common-api-v3/", L"*.js", sharedPathnames);
		for (VfsPaths::iterator it = sharedPathnames.begin(); it != sharedPathnames.end(); ++it)
		{
			if (!m_ScriptInterface.LoadGlobalScriptFile(*it))
			{
				LOGERROR(L"Failed to load shared script %ls", it->string().c_str());
				return false;
			}
			m_HasSharedComponent = true;
		}
		if (!m_HasSharedComponent)
			return false;
		
		// mainly here for the error messages
		OsPath path = L"simulation/ai/common-api-v2/";
		
		// Constructor name is SharedScript
		CScriptVal ctor;
		if (!m_ScriptInterface.GetProperty(m_ScriptInterface.GetGlobalObject(), "SharedScript", ctor)
			|| ctor.undefined())
		{
			LOGERROR(L"Failed to create shared AI component: %ls: can't find constructor '%hs'", path.string().c_str(), "SharedScript");
			return false;
		}
		
		if (hasTechs)
		{
			// Set up the data to pass as the constructor argument
			CScriptVal settings;
			m_ScriptInterface.Eval(L"({})", settings);
			CScriptVal playersID;
			m_ScriptInterface.Eval(L"({})", playersID);
			
			for (size_t i = 0; i < m_Players.size(); ++i)
			{
				jsval val = m_ScriptInterface.ToJSVal(m_ScriptInterface.GetContext(), m_Players[i]->m_Player);
				m_ScriptInterface.SetPropertyInt(playersID.get(), i, CScriptVal(val), true);
			}
			
			m_ScriptInterface.SetProperty(settings.get(), "players", playersID);
			
			ENSURE(m_HasLoadedEntityTemplates);
			m_ScriptInterface.SetProperty(settings.get(), "templates", m_EntityTemplates, false);
		
			m_ScriptInterface.SetProperty(settings.get(), "techTemplates", m_TechTemplates, false);

			m_SharedAIObj = CScriptValRooted(m_ScriptInterface.GetContext(),m_ScriptInterface.CallConstructor(ctor.get(), settings.get()));
		}
		else
		{
			// won't get the tech templates directly.
			// Set up the data to pass as the constructor argument
			CScriptVal settings;
			m_ScriptInterface.Eval(L"({})", settings);
			CScriptVal playersID;
			m_ScriptInterface.Eval(L"({})", playersID);
			for (size_t i = 0; i < m_Players.size(); ++i)
			{
				jsval val = m_ScriptInterface.ToJSVal(m_ScriptInterface.GetContext(), m_Players[i]->m_Player);
				m_ScriptInterface.SetPropertyInt(playersID.get(), i, CScriptVal(val), true);
			}
			
			m_ScriptInterface.SetProperty(settings.get(), "players", playersID);
			
			CScriptVal m_fakeTech;
			m_ScriptInterface.Eval("({})", m_fakeTech);
			m_ScriptInterface.SetProperty(settings.get(), "techTemplates", m_fakeTech, false);

			ENSURE(m_HasLoadedEntityTemplates);
			m_ScriptInterface.SetProperty(settings.get(), "templates", m_EntityTemplates, false);
			m_SharedAIObj = CScriptValRooted(m_ScriptInterface.GetContext(),m_ScriptInterface.CallConstructor(ctor.get(), settings.get()));
		}
		
		if (m_SharedAIObj.undefined())
		{
			LOGERROR(L"Failed to create shared AI component: %ls: error calling constructor '%hs'", path.string().c_str(), "SharedScript");
			return false;
		}
		
		return true;
	}

	bool AddPlayer(const std::wstring& aiName, player_id_t player, uint8_t difficulty, bool callConstructor)
	{
		shared_ptr<CAIPlayer> ai(new CAIPlayer(*this, aiName, player, difficulty, m_ScriptRuntime, m_RNG));
		if (!ai->Initialise(callConstructor))
			return false;
		
		// this will be set to true if we need to load the shared Component.
		if (!m_HasSharedComponent)
			m_HasSharedComponent = ai->m_UseSharedComponent;

		m_ScriptInterface.MaybeGC();

		m_Players.push_back(ai);

		return true;
	}

	bool RunGamestateInit(const shared_ptr<ScriptInterface::StructuredClone>& gameState, const Grid<u16>& passabilityMap, const Grid<u8>& territoryMap)
	{
		// this will be run last by InitGame.Js, passing the full game representation.
		// For now it will run for the shared Component.
		// This is NOT run during deserialization.
		CScriptVal state = m_ScriptInterface.ReadStructuredClone(gameState);
		JSContext* cx = m_ScriptInterface.GetContext();

		m_PassabilityMapVal = CScriptValRooted(cx, ScriptInterface::ToJSVal(cx, passabilityMap));
		m_TerritoryMapVal = CScriptValRooted(cx, ScriptInterface::ToJSVal(cx, territoryMap));
		if (m_HasSharedComponent)
		{
			m_ScriptInterface.SetProperty(state.get(), "passabilityMap", m_PassabilityMapVal, true);
			m_ScriptInterface.SetProperty(state.get(), "territoryMap", m_TerritoryMapVal, true);

			m_ScriptInterface.CallFunctionVoid(m_SharedAIObj.get(), "initWithState", state);
			m_ScriptInterface.MaybeGC();
			
			for (size_t i = 0; i < m_Players.size(); ++i)
			{
				if (m_HasSharedComponent && m_Players[i]->m_UseSharedComponent)
					m_Players[i]->InitWithSharedScript(state,m_SharedAIObj);
			}
		}
		
		return true;
	}
	void StartComputation(const shared_ptr<ScriptInterface::StructuredClone>& gameState, const Grid<u16>& passabilityMap, const Grid<u8>& territoryMap, bool territoryMapDirty)
	{
		ENSURE(m_CommandsComputed);

		m_GameState = gameState;

		if (passabilityMap.m_DirtyID != m_PassabilityMap.m_DirtyID)
		{
			m_PassabilityMap = passabilityMap;

			JSContext* cx = m_ScriptInterface.GetContext();
			m_PassabilityMapVal = CScriptValRooted(cx, ScriptInterface::ToJSVal(cx, m_PassabilityMap));
		}

		if (territoryMapDirty)
		{
			m_TerritoryMap = territoryMap;

			JSContext* cx = m_ScriptInterface.GetContext();
			m_TerritoryMapVal = CScriptValRooted(cx, ScriptInterface::ToJSVal(cx, m_TerritoryMap));
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

	void RegisterTechTemplates(const shared_ptr<ScriptInterface::StructuredClone>& techTemplates) {
		JSContext* cx = m_ScriptInterface.GetContext();
		m_TechTemplates = CScriptValRooted(cx, m_ScriptInterface.ReadStructuredClone(techTemplates));
	}
	
	void LoadEntityTemplates(const std::vector<std::pair<std::string, const CParamNode*> >& templates)
	{
		m_HasLoadedEntityTemplates = true;

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
			// TODO: see comment in Deserialize()
			serializer.SetSerializablePrototypes(m_SerializablePrototypes);
			SerializeState(serializer);
		}
	}

	void SerializeState(ISerializer& serializer)
	{
		std::stringstream rngStream;
		rngStream << m_RNG;
		serializer.StringASCII("rng", rngStream.str(), 0, 32);

		serializer.NumberU32_Unbounded("turn", m_TurnNum);

		serializer.NumberU32_Unbounded("num ais", (u32)m_Players.size());

		serializer.Bool("useSharedScript", m_HasSharedComponent);
		if (m_HasSharedComponent)
		{
			CScriptVal sharedData;
			if (!m_ScriptInterface.CallFunction(m_SharedAIObj.get(), "Serialize", sharedData))
				LOGERROR(L"AI shared script Serialize call failed");
			serializer.ScriptVal("sharedData", sharedData);
		}
		for (size_t i = 0; i < m_Players.size(); ++i)
		{
			serializer.String("name", m_Players[i]->m_AIName, 1, 256);
			serializer.NumberI32_Unbounded("player", m_Players[i]->m_Player);
			serializer.NumberU8_Unbounded("difficulty", m_Players[i]->m_Difficulty);
			
			serializer.NumberU32_Unbounded("num commands", (u32)m_Players[i]->m_Commands.size());
			for (size_t j = 0; j < m_Players[i]->m_Commands.size(); ++j)
			{
				CScriptVal val = m_ScriptInterface.ReadStructuredClone(m_Players[i]->m_Commands[j]);
				serializer.ScriptVal("command", val);
			}

			bool hasCustomSerialize = m_ScriptInterface.HasProperty(m_Players[i]->m_Obj.get(), "Serialize");
			if (hasCustomSerialize)
			{
				CScriptVal scriptData;
				if (!m_ScriptInterface.CallFunction(m_Players[i]->m_Obj.get(), "Serialize", scriptData))
					LOGERROR(L"AI script Serialize call failed");
				serializer.ScriptVal("data", scriptData);
			}
			else
			{
				serializer.ScriptVal("data", m_Players[i]->m_Obj.get());
			}
		}
	}

	void Deserialize(std::istream& stream)
	{
		ENSURE(m_CommandsComputed); // deserializing while we're still actively computing would be bad

		CStdDeserializer deserializer(m_ScriptInterface, stream);

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
			CScriptVal sharedData;
			deserializer.ScriptVal("sharedData", sharedData);
			if (!m_ScriptInterface.CallFunctionVoid(m_SharedAIObj.get(), "Deserialize", sharedData))
				LOGERROR(L"AI shared script Deserialize call failed");
		}

		for (size_t i = 0; i < numAis; ++i)
		{
			std::wstring name;
			player_id_t player;
			uint8_t difficulty;
			deserializer.String("name", name, 1, 256);
			deserializer.NumberI32_Unbounded("player", player);
			deserializer.NumberU8_Unbounded("difficulty",difficulty);
			if (!AddPlayer(name, player, difficulty, true))
				throw PSERROR_Deserialize_ScriptError();

			uint32_t numCommands;
			deserializer.NumberU32_Unbounded("num commands", numCommands);
			m_Players.back()->m_Commands.reserve(numCommands);
			for (size_t j = 0; j < numCommands; ++j)
			{
				CScriptVal val;
				deserializer.ScriptVal("command", val);
				m_Players.back()->m_Commands.push_back(m_ScriptInterface.WriteStructuredClone(val.get()));
			}
			
			// TODO: this is yucky but necessary while the AIs are sharing data between contexts;
			//	ideally a new (de)serializer instance would be created for each player
			//	so they would have a single, consistent script context to use and serializable
			//	prototypes could be stored in their ScriptInterface
			deserializer.SetSerializablePrototypes(m_DeserializablePrototypes);

			bool hasCustomDeserialize = m_ScriptInterface.HasProperty(m_Players.back()->m_Obj.get(), "Deserialize");
			if (hasCustomDeserialize)
			{
				CScriptVal scriptData;
				deserializer.ScriptVal("data", scriptData);
				if (m_Players[i]->m_UseSharedComponent)
				{
					if (!m_ScriptInterface.CallFunctionVoid(m_Players.back()->m_Obj.get(), "Deserialize", scriptData, m_SharedAIObj))
						LOGERROR(L"AI script Deserialize call failed");
				}
				else if (!m_ScriptInterface.CallFunctionVoid(m_Players.back()->m_Obj.get(), "Deserialize", scriptData))
				{
					LOGERROR(L"AI script deserialize() call failed");
				}
			}
			else
			{
				deserializer.ScriptVal("data", m_Players.back()->m_Obj);
			}
		}
	}
	
	int getPlayerSize()
	{
		return m_Players.size();
	}

	void RegisterSerializablePrototype(std::wstring name, CScriptVal proto)
	{
		// Require unique prototype and name (for reverse lookup)
		// TODO: this is yucky - see comment in Deserialize()
		JSObject* obj = JSVAL_TO_OBJECT(proto.get());
		std::pair<std::map<JSObject*, std::wstring>::iterator, bool> ret1 = m_SerializablePrototypes.insert(std::make_pair(obj, name));
		std::pair<std::map<std::wstring, JSObject*>::iterator, bool> ret2 = m_DeserializablePrototypes.insert(std::make_pair(name, obj));
		if (!ret1.second || !ret2.second)
			LOGERROR(L"RegisterSerializablePrototype called with same prototype multiple times: p=%p n='%ls'", obj, name.c_str());
	}

private:
	CScriptValRooted LoadMetadata(const VfsPath& path)
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
		if (m_Players.size() == 0)
		{
			// Run the GC every so often.
			// (This isn't particularly necessary, but it makes profiling clearer
			// since it avoids random GC delays while running other scripts)
			if (m_TurnNum++ % 50 == 0)
			{
				PROFILE3("AI compute GC");
				m_ScriptInterface.MaybeGC();
			}
			return;
		}
				
		// Deserialize the game state, to pass to the AI's HandleMessage
		CScriptVal state;
		{
			PROFILE3("AI compute read state");
			state = m_ScriptInterface.ReadStructuredClone(m_GameState);
			m_ScriptInterface.SetProperty(state.get(), "passabilityMap", m_PassabilityMapVal, true);
			m_ScriptInterface.SetProperty(state.get(), "territoryMap", m_TerritoryMapVal, true);
		}

		// It would be nice to do
		//   m_ScriptInterface.FreezeObject(state.get(), true);
		// to prevent AI scripts accidentally modifying the state and
		// affecting other AI scripts they share it with. But the performance
		// cost is far too high, so we won't do that.
		// If there is a shared component, run it

		if (m_HasSharedComponent)
		{
			PROFILE3("AI run shared component");
			m_ScriptInterface.CallFunctionVoid(m_SharedAIObj.get(), "onUpdate", state);
		}
		
		for (size_t i = 0; i < m_Players.size(); ++i)
		{
			PROFILE3("AI script");
			PROFILE2_ATTR("player: %d", m_Players[i]->m_Player);
			PROFILE2_ATTR("script: %ls", m_Players[i]->m_AIName.c_str());
			if (m_HasSharedComponent && m_Players[i]->m_UseSharedComponent)
				m_Players[i]->Run(state,m_SharedAIObj);
			else
				m_Players[i]->Run(state);
		}

		// Run GC if we are about to overflow
		if (JS_GetGCParameter(m_ScriptInterface.GetRuntime(), JSGC_BYTES) > 33000000)
		{
			PROFILE3("AI compute GC");

			JS_GC(m_ScriptInterface.GetContext());
		}
		
		// Run the GC every so often.
		// (This isn't particularly necessary, but it makes profiling clearer
		// since it avoids random GC delays while running other scripts)
		/*if (m_TurnNum++ % 20 == 0)
		{
			PROFILE3("AI compute GC");
			m_ScriptInterface.MaybeGC();
		}*/
	}

	shared_ptr<ScriptRuntime> m_ScriptRuntime;
	ScriptInterface m_ScriptInterface;
	boost::rand48 m_RNG;
	u32 m_TurnNum;

	CScriptValRooted m_EntityTemplates;
	bool m_HasLoadedEntityTemplates;
	CScriptValRooted m_TechTemplates;

	std::map<VfsPath, CScriptValRooted> m_PlayerMetadata;
	std::vector<shared_ptr<CAIPlayer> > m_Players; // use shared_ptr just to avoid copying

	bool m_HasSharedComponent;
	CScriptValRooted m_SharedAIObj;
	std::vector<SCommandSets> m_Commands;
	
	shared_ptr<ScriptInterface::StructuredClone> m_GameState;
	Grid<u16> m_PassabilityMap;
	CScriptValRooted m_PassabilityMapVal;
	Grid<u8> m_TerritoryMap;
	CScriptValRooted m_TerritoryMapVal;

	bool m_CommandsComputed;

	std::map<JSObject*, std::wstring> m_SerializablePrototypes;
	std::map<std::wstring, JSObject*> m_DeserializablePrototypes;
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
		m_Worker.AddPlayer(id, player, difficulty, true);

		// AI players can cheat and see through FoW/SoD, since that greatly simplifies
		// their implementation.
		// (TODO: maybe cleverer AIs should be able to optionally retain FoW/SoD)
		CmpPtr<ICmpRangeManager> cmpRangeManager(GetSystemEntity());
		if (cmpRangeManager)
			cmpRangeManager->SetLosRevealAll(player, true);
	}
	
	virtual void TryLoadSharedComponent()
	{
		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();
		// load the technology templates
		CmpPtr<ICmpTechnologyTemplateManager> cmpTechTemplateManager(GetSystemEntity());
		ENSURE(cmpTechTemplateManager);
		
		// Get the game state from AIInterface
		CScriptVal techTemplates = cmpTechTemplateManager->GetAllTechs();
		
		m_Worker.RegisterTechTemplates(scriptInterface.WriteStructuredClone(techTemplates.get()));
		m_Worker.TryLoadSharedComponent(true);
	}

	virtual void RunGamestateInit()
	{
		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();
		
		CmpPtr<ICmpAIInterface> cmpAIInterface(GetSystemEntity());
		ENSURE(cmpAIInterface);
		
		// Get the game state from AIInterface
		// We flush events from the initialization so we get a clean state now.
		CScriptVal state = cmpAIInterface->GetFullRepresentation(true);

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

		m_Worker.RunGamestateInit(scriptInterface.WriteStructuredClone(state.get()), *passabilityMap, *territoryMap);
	}

	virtual void StartComputation()
	{
		PROFILE("AI setup");

		ForceLoadEntityTemplates();

		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();

		if (m_Worker.getPlayerSize() == 0)
			return;
		
		CmpPtr<ICmpAIInterface> cmpAIInterface(GetSystemEntity());
		ENSURE(cmpAIInterface);

		// Get the game state from AIInterface
		CScriptVal state = cmpAIInterface->GetRepresentation();

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

		m_Worker.StartComputation(scriptInterface.WriteStructuredClone(state.get()), *passabilityMap, *territoryMap, territoryMapDirty);
	}

	virtual void PushCommands()
	{
		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();

		std::vector<CAIWorker::SCommandSets> commands;
		m_Worker.GetCommands(commands);

		CmpPtr<ICmpCommandQueue> cmpCommandQueue(GetSystemEntity());
		if (!cmpCommandQueue)
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
	std::vector<std::string> m_TemplateNames;
	size_t m_TemplateLoadedIdx;
	std::vector<std::pair<std::string, const CParamNode*> > m_Templates;
	size_t m_TerritoriesDirtyID;

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

	void LoadPathfinderClasses(CScriptVal state)
	{
		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (!cmpPathfinder)
			return;

		ScriptInterface& scriptInterface = GetSimContext().GetScriptInterface();

		CScriptVal classesVal;
		scriptInterface.Eval("({ pathfinderObstruction: 1, foundationObstruction: 2 })", classesVal);

		std::map<std::string, ICmpPathfinder::pass_class_t> classes = cmpPathfinder->GetPassabilityClasses();
		for (std::map<std::string, ICmpPathfinder::pass_class_t>::iterator it = classes.begin(); it != classes.end(); ++it)
			scriptInterface.SetProperty(classesVal.get(), it->first.c_str(), it->second, true);

		scriptInterface.SetProperty(state.get(), "passabilityClasses", classesVal, true);
	}

	CAIWorker m_Worker;
};

REGISTER_COMPONENT_TYPE(AIManager)
