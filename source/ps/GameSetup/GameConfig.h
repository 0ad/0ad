#ifndef INCLUDED_GAMECONFIG
#define INCLUDED_GAMECONFIG
#include <string>
#include <vector>
#include "ps/GameSetup/CmdLineArgs.h"
#include "scriptinterface/ScriptVal.h"
#include "scriptinterface/ScriptInterface.h"

enum NetworkGameType { Host, Client, Local };
struct GameConfig {
    GameConfig(std::wstring t, std::wstring n) :
        type(t), name(n), size(192), numPlayers(2), seed(0), aiseed(0), nonVisual(false),
        teams(std::vector<std::tuple<int, int>>()), ceasefire(0),
        ai(std::vector<std::tuple<int, std::string>>()),
        civs(std::vector<std::tuple<int, std::string>>()),
        difficulties(std::vector<std::tuple<int, int>>()),
        victoryConditions(std::vector<std::string>(1, "conquest")),
        username(L"anonymous"), wonderDuration(10), relicDuration(10), relicCount(2),
        playerID(1), netGameType(NetworkGameType::Local), maxPlayersToHost(2),
        hostAddress("127.0.0.1")
    {}

    static GameConfig from (const CmdLineArgs& args);
    JS::MutableHandleValue toJSValue (const ScriptInterface& scriptInterface) const;
    //bool toJSValue (const ScriptInterface& scriptInterface, JS::RootedValue attrs) const;

    void setNetworkHost()
    {
        netGameType = NetworkGameType::Host;
    }

    bool isNetworkHost() const
    {
        return netGameType == NetworkGameType::Host;
    }

    void setNetworkClient()
    {
        netGameType = NetworkGameType::Client;
    }

    bool isNetworkClient() const
    {
        return netGameType == NetworkGameType::Client;
    }

    std::wstring getFullName() const
    {
        return this->getMapDirectory() + L"/" + name;
    }

    std::wstring getMapDirectory() const
    {
		if (type == L"scenario")
			return L"scenarios";
		else if (type == L"skirmish")
			return L"skirmishes";
        else
            return type;
    }

    std::wstring type;
    std::wstring name;
    std::wstring username;
    int playerID;
    uint size;
    uint numPlayers;
	u32 seed;
	u32 aiseed;
    std::vector<std::tuple<int, int>> teams;
    int ceasefire;
    std::vector<std::tuple<int, std::string>> ai;
    std::vector<std::tuple<int, int>> difficulties;
    std::vector<std::tuple<int, std::string>> civs;
    std::vector<std::string> victoryConditions;
    int wonderDuration;
    int relicDuration;
    int relicCount;
    size_t maxPlayersToHost;
    NetworkGameType netGameType;
    std::string hostAddress;
    bool nonVisual;
};
#endif // INCLUDED_GAMECONFIG
