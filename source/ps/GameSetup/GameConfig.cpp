#include "ps/GameSetup/GameConfig.h"

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
