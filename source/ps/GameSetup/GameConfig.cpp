#include "ps/GameSetup/GameConfig.h"
#include "ps/Errors.h"
#include "ps/XML/Xeromyces.h"
#include "ps/CLogger.h"
#include "scriptinterface/ScriptConversions.h"
#include "ps/Filesystem.h"

#include "ps/World.h"

GameConfig GameConfig::from (const CmdLineArgs& args)
{
    CStr autoStartName = args.Get("autostart");
    Path mapPath = Path(autoStartName);
    std::wstring mapName = mapPath.Filename().string();

    std::wstring mapDirectory = mapPath.Parent().Filename().string();
    std::wstring mapType;
    if (mapDirectory == L"scenarios")
    {
        mapType = L"scenario";
    }
    else if (mapDirectory == L"skirmishes")
    {
        mapType = L"skirmish";
    }

    struct GameConfig config(mapType, mapName);

    config.nonVisual = args.Has("autostart-nonvisual");
    if (args.Has("autostart-size"))
    {
        config.size = args.Get("autostart-size").ToUInt();
    }

    if (args.Has("autostart-players"))
    {
        config.numPlayers = args.Get("autostart-players").ToUInt();
    }

    // map type
    if (args.Has("autostart-seed"))
    {
        CStr seedArg = args.Get("autostart-seed");
        if (seedArg == "-1")
            config.seed = rand();
        else
            config.seed = seedArg.ToULong();
    }

    // Set seed for AIs
    if (args.Has("autostart-aiseed"))
    {
        CStr seedArg = args.Get("autostart-aiseed");
        if (seedArg == "-1")
            config.aiseed = rand();
        else
            config.aiseed = seedArg.ToULong();
    }

    {
        std::vector<CStr> civArgs = args.GetMultiple("autostart-team");
        for (size_t i = 0; i < civArgs.size(); ++i)
        {
            int playerID = civArgs[i].BeforeFirst(":").ToInt();
            int teamID = civArgs[i].AfterFirst(":").ToInt() - 1;
            config.teams.push_back(std::make_tuple(playerID, teamID));
        }
    }

    if (args.Has("autostart-ceasefire"))
        config.ceasefire = args.Get("autostart-ceasefire").ToInt();

    if (args.Has("autostart-ai"))
    {
        std::vector<CStr> aiArgs = args.GetMultiple("autostart-ai");
        for (size_t i = 0; i < aiArgs.size(); ++i)
        {
            int playerID = aiArgs[i].BeforeFirst(":").ToInt();
            CStr name = aiArgs[i].AfterFirst(":");
            config.ai.push_back(std::make_tuple(playerID, name));
        }
    }

    // Set AI difficulty
    if (args.Has("autostart-aidiff"))
    {
        std::vector<CStr> civArgs = args.GetMultiple("autostart-aidiff");
        for (size_t i = 0; i < civArgs.size(); ++i)
        {
            int playerID = civArgs[i].BeforeFirst(":").ToInt();
            int difficulty = civArgs[i].AfterFirst(":").ToInt();
            config.difficulties.push_back(std::make_tuple(playerID, difficulty));
        }
    }

    // Set player data for Civs
    if (args.Has("autostart-civ"))
    {
        std::vector<CStr> civArgs = args.GetMultiple("autostart-civ");
        for (size_t i = 0; i < civArgs.size(); ++i)
        {
            int playerID = civArgs[i].BeforeFirst(":").ToInt();
            CStr name = civArgs[i].AfterFirst(":");
            config.civs.push_back(std::make_tuple(playerID, name));
        }
    }

    if (args.Has("autostart-playername"))
        config.username = args.Get("autostart-playername").FromUTF8();

    // Add additional scripts to the TriggerScripts property
    if (args.Has("autostart-victory"))
    {
        config.victoryConditions = std::vector<std::string>();
        std::vector<CStr> vicArgs = args.GetMultiple("autostart-victory");
        for (size_t i = 0; i < vicArgs.size(); ++i)
        {
            config.victoryConditions.push_back(vicArgs[i]);
        }
    }

    if (args.Has("autostart-wonderduration"))
        config.wonderDuration = args.Get("autostart-wonderduration").ToInt();

    if (args.Has("autostart-relicduration"))
        config.relicDuration = args.Get("autostart-relicduration").ToInt();

    if (args.Has("autostart-reliccount"))
        config.relicCount = args.Get("autostart-reliccount").ToInt();

    
    if (args.Has("autostart-host"))
    {
        config.setNetworkHost();
        if (args.Has("autostart-host-players"))
            config.maxPlayersToHost = args.Get("autostart-host-players").ToUInt();
    }
    else if (args.Has("autostart-client"))
    {
        CStr ip = args.Get("autostart-client");
        if (ip.empty())
            ip = "127.0.0.1";

        config.setNetworkClient();
    }
    else if (args.Has("autostart-player"))
    {
        config.playerID = args.Get("autostart-player").ToInt();
    }
    return config;
}

GameConfig GameConfig::from (const ScenarioConfig& msg)
{
    const std::wstring mapType = wstring_from_utf8(msg.type());
    const std::wstring mapName = wstring_from_utf8(msg.name());
    GameConfig config(mapType, mapName);

    config.seed = msg.seed() || rand();
    config.aiseed = msg.aiseed() || rand();

    if (msg.gamespeed())
        config.gameSpeed = msg.gamespeed();
    
    for (int i = 0; i < msg.players_size(); i++)
    {
        int playerID = msg.players(i).id();
        CStr name = msg.players(i).type();
        int difficulty = msg.players(i).difficulty() || 3;

        config.ai.push_back(std::make_tuple(playerID, name));
        config.difficulties.push_back(std::make_tuple(playerID, difficulty));
    }

    return config;
}

/**
 * Temporarily loads a scenario map and retrieves the "ScriptSettings" JSON
 * data from it.
 * The scenario map format is used for scenario and skirmish map types (random
 * games do not use a "map" (format) but a small JavaScript program which
 * creates a map on the fly). It contains a section to initialize the game
 * setup screen.
 * @param mapPath Absolute path (from VFS root) to the map file to peek in.
 * @return ScriptSettings in JSON format extracted from the map.
 */
CStr8 LoadSettingsOfScenarioMap(const VfsPath &mapPath)
{
	CXeromyces mapFile;
	const char *pathToSettings[] =
	{
		"Scenario", "ScriptSettings", ""	// Path to JSON data in map
	};

	Status loadResult = mapFile.Load(g_VFS, mapPath);

	if (INFO::OK != loadResult)
	{
        // TODO: Throw error
		LOGERROR("LoadSettingsOfScenarioMap: Unable to load map file '%s'", mapPath.string8());
		throw PSERROR_Game_World_MapLoadFailed("Unable to load map file, check the path for typos.");
	}
	XMBElement mapElement = mapFile.GetRoot();

	// Select the ScriptSettings node in the map file...
	for (int i = 0; pathToSettings[i][0]; ++i)
	{
		int childId = mapFile.GetElementID(pathToSettings[i]);

		XMBElementList nodes = mapElement.GetChildNodes();
		auto it = std::find_if(nodes.begin(), nodes.end(), [&childId](const XMBElement& child) {
			return child.GetNodeName() == childId;
		});

		if (it != nodes.end())
			mapElement = *it;
	}
	// ... they contain a JSON document to initialize the game setup
	// screen
	return mapElement.GetText();
}

JS::MutableHandleValue GameConfig::toJSValue (const ScriptInterface& scriptInterface) const
{
	JSContext* cx = scriptInterface.GetContext();

    JS::RootedValue attrs(cx);
	scriptInterface.Eval("({})", &attrs);
	JS::RootedValue settings(cx);
	scriptInterface.Eval("({})", &settings);
	JS::RootedValue playerData(cx);
	scriptInterface.Eval("([])", &playerData);

	// The directory in front of the actual map name indicates which type
	// of map is being loaded. Drawback of this approach is the association
	// of map types and folders is hard-coded, but benefits are:
	// - No need to pass the map type via command line separately
	// - Prevents mixing up of scenarios and skirmish maps to some degree
    CStr fullName = utf8_from_wstring(this->getFullName());
	std::wstring mapDirectory = this->getMapDirectory();

	if (this->type == L"random")
	{
		// Random map definition will be loaded from JSON file, so we need to parse it
		std::wstring scriptPath = L"maps/" + fullName.FromUTF8() + L".json";
		JS::RootedValue scriptData(cx);
		scriptInterface.ReadJSONFile(scriptPath, &scriptData);
		if (!scriptData.isUndefined() && scriptInterface.GetProperty(scriptData, "settings", &settings))
		{
			// JSON loaded ok - copy script name over to game attributes
			std::wstring scriptFile;
			scriptInterface.GetProperty(settings, "Script", scriptFile);
			scriptInterface.SetProperty(attrs, "script", scriptFile);				// RMS filename
		}
		else
		{
            // TODO: Throw an exception
			// Problem with JSON file
            //throw std::runtime_error("Error reading random map script '%s'", utf8_from_wstring(scriptPath));
			LOGERROR("Autostart: Error reading random map script '%s'", utf8_from_wstring(scriptPath));
			throw PSERROR_Game_World_MapLoadFailed("Error reading random map script.\nCheck application log for details.");
		}

		scriptInterface.SetProperty(settings, "Size", this->size);		// Random map size (in patches)

		// Set up player data
		for (size_t i = 0; i < this->numPlayers; ++i)
		{
			JS::RootedValue player(cx);
			scriptInterface.Eval("({})", &player);

			// We could load player_defaults.json here, but that would complicate the logic
			// even more and autostart is only intended for developers anyway
            // FIXME: enable setting the civilizations
			scriptInterface.SetProperty(player, "Civ", std::string("athen"));
			scriptInterface.SetPropertyInt(playerData, i, player);
		}
	}
	else if (this->type == L"scenario" || this->type == L"skirmish")
	{
		// Initialize general settings from the map data so some values
		// (e.g. name of map) are always present, even when autostart is
		// partially configured
		CStr8 mapSettingsJSON = LoadSettingsOfScenarioMap("maps/" + fullName + ".xml");
		scriptInterface.ParseJSON(mapSettingsJSON, &settings);

		// Initialize the playerData array being modified by autostart
		// with the real map data, so sensible values are present:
		scriptInterface.GetProperty(settings, "PlayerData", &playerData);
	}
	else
	{
        // TODO: Throw an exception
		LOGERROR("Autostart: Unrecognized map type '%s'", utf8_from_wstring(mapDirectory));
		throw PSERROR_Game_World_MapLoadFailed("Unrecognized map type.\nConsult readme.txt for the currently supported types.");
	}

	scriptInterface.SetProperty(attrs, "mapType", this->type);
	scriptInterface.SetProperty(attrs, "map", std::string("maps/" + fullName));
	scriptInterface.SetProperty(settings, "mapType", this->type);
	scriptInterface.SetProperty(settings, "gameSpeed", this->gameSpeed);
	scriptInterface.SetProperty(settings, "CheatsEnabled", true);

	// The seed is used for both random map generation and simulation
	scriptInterface.SetProperty(settings, "Seed", this->seed);

	// Set seed for AIs
	scriptInterface.SetProperty(settings, "AISeed", this->aiseed);

	// Set player data for AIs
	//		attrs.settings = { PlayerData: [ { AI: ... }, ... ] }
	//		            or = { PlayerData: [ null, { AI: ... }, ... ] } when gaia set
	int offset = 1;
	JS::RootedValue player(cx);
	if (scriptInterface.GetPropertyInt(playerData, 0, &player) && player.isNull())
		offset = 0;

	// Set teams
    std::vector<std::tuple<int, int>> civArgs = this->teams;
    for (size_t i = 0; i < this->teams.size(); ++i)
    {
        int playerID = std::get<0>(this->teams[i]);
        int teamID = std::get<1>(this->teams[i]);

        // Instead of overwriting existing player data, modify the array
        JS::RootedValue player(cx);
        if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
        {
            if (mapDirectory == L"skirmishes")
            {
                // playerID is certainly bigger than this map player number
                LOGWARNING("Autostart: Invalid player %d in autostart-team option", playerID);
                continue;
            }
            scriptInterface.Eval("({})", &player);
        }

        scriptInterface.SetProperty(player, "Team", teamID);
        scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
	}

	scriptInterface.SetProperty(settings, "Ceasefire", this->ceasefire);

    for (size_t i = 0; i < this->ai.size(); ++i)
    {
        int playerID = std::get<0>(this->ai[i]);
        CStr name = std::get<1>(this->ai[i]);

        // Instead of overwriting existing player data, modify the array
        JS::RootedValue player(cx);
        if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
        {
            if (mapDirectory == L"scenarios" || mapDirectory == L"skirmishes")
            {
                // playerID is certainly bigger than this map player number
                LOGWARNING("Autostart: Invalid player %d in autostart-ai option", playerID);
                continue;
            }
            scriptInterface.Eval("({})", &player);
        }

        scriptInterface.SetProperty(player, "AI", std::string(name));
        scriptInterface.SetProperty(player, "AIDiff", 3);
        scriptInterface.SetProperty(player, "AIBehavior", std::string("balanced"));
        scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
    }
	// Set AI difficulty
    for (size_t i = 0; i < this->difficulties.size(); ++i)
    {
        int playerID = std::get<0>(this->difficulties[i]);
        int difficulty = std::get<1>(this->difficulties[i]);

        // Instead of overwriting existing player data, modify the array
        JS::RootedValue player(cx);
        if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
        {
            if (mapDirectory == L"scenarios" || mapDirectory == L"skirmishes")
            {
                // playerID is certainly bigger than this map player number
                LOGWARNING("Autostart: Invalid player %d in autostart-aidiff option", playerID);
                continue;
            }
            scriptInterface.Eval("({})", &player);
        }

        scriptInterface.SetProperty(player, "AIDiff", difficulty);
        scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
    }
	// Set player data for Civs
    if (this->type != L"scenario")
    {
        for (size_t i = 0; i < this->civs.size(); ++i)
        {
            int playerID = std::get<0>(this->civs[i]);
            CStr name = std::get<1>(this->civs[i]);

            // Instead of overwriting existing player data, modify the array
            JS::RootedValue player(cx);
            if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
            {
                if (mapDirectory == L"skirmishes")
                {
                    // playerID is certainly bigger than this map player number
                    LOGWARNING("Autostart: Invalid player %d in autostart-civ option", playerID);
                    continue;
                }
                scriptInterface.Eval("({})", &player);
            }

            scriptInterface.SetProperty(player, "Civ", std::string(name));
            scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
        }
    }
    else if (this->civs.size() > 0)
        LOGWARNING("Autostart: Option 'autostart-civ' is invalid for scenarios");

	// Add player data to map settings
	scriptInterface.SetProperty(settings, "PlayerData", playerData);

	// Add map settings to game attributes
	scriptInterface.SetProperty(attrs, "settings", settings);

	// Get optional playername
	CStrW userName = this->username;

	// Add additional scripts to the TriggerScripts property
	std::vector<CStrW> triggerScriptsVector;
	JS::RootedValue triggerScripts(cx);

	if (scriptInterface.HasProperty(settings, "TriggerScripts"))
	{
		scriptInterface.GetProperty(settings, "TriggerScripts", &triggerScripts);
		FromJSVal_vector(cx, triggerScripts, triggerScriptsVector);
	}

	if (this->nonVisual)
	{
		CStr nonVisualScript = "scripts/NonVisualTrigger.js";
		triggerScriptsVector.push_back(nonVisualScript.FromUTF8());
	}

	std::vector<CStr> victoryConditions;
    for (size_t i = 0; i < this->victoryConditions.size(); ++i)
    {
        victoryConditions.push_back(this->victoryConditions[i]);
    }

	if (victoryConditions.size() == 1 && victoryConditions[0] == "endless")
		victoryConditions.clear();

	scriptInterface.SetProperty(settings, "VictoryConditions", victoryConditions);

	for (const CStr& victory : victoryConditions)
	{
		JS::RootedValue scriptData(cx);
		JS::RootedValue data(cx);
		JS::RootedValue victoryScripts(cx);

		CStrW scriptPath = L"simulation/data/settings/victory_conditions/" + victory.FromUTF8() + L".json";
		scriptInterface.ReadJSONFile(scriptPath, &scriptData);
		if (!scriptData.isUndefined() && scriptInterface.GetProperty(scriptData, "Data", &data) && !data.isUndefined()
			&& scriptInterface.GetProperty(data, "Scripts", &victoryScripts) && !victoryScripts.isUndefined())
		{
			std::vector<CStrW> victoryScriptsVector;
			FromJSVal_vector(cx, victoryScripts, victoryScriptsVector);
			triggerScriptsVector.insert(triggerScriptsVector.end(), victoryScriptsVector.begin(), victoryScriptsVector.end());
		}
		else
		{
            // FIXME: Change this
			LOGERROR("Autostart: Error reading victory script '%s'", utf8_from_wstring(scriptPath));
			throw PSERROR_Game_World_MapLoadFailed("Error reading victory script.\nCheck application log for details.");
		}
	}

	ToJSVal_vector(cx, &triggerScripts, triggerScriptsVector);
	scriptInterface.SetProperty(settings, "TriggerScripts", triggerScripts);

	scriptInterface.SetProperty(settings, "WonderDuration", this->wonderDuration);
	scriptInterface.SetProperty(settings, "RelicDuration", this->relicDuration);
	scriptInterface.SetProperty(settings, "RelicCount", this->relicCount);

    // TODO: Can I print it??
	return &attrs;
}

