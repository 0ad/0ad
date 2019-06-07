#ifndef INCLUDED_RLINTERFACE
#define INCLUDED_RLINTERFACE
#include <mutex>
#include <queue>
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "rlinterface/proto/RLAPI.grpc.pb.h"

#include "lib/precompiled.h"
#include "lib/external_libraries/libsdl.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpAIInterface.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/system/TurnManager.h"
#include "ps/Game.h"
#include "ps/Loader.h"
#include "gui/GUIManager.h"
#include "ps/VideoMode.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/GameSetup/GameConfig.h"
#include "ps/ThreadUtil.h"
#include <boost/fiber/unbuffered_channel.hpp>
#include <boost/fiber/buffered_channel.hpp>

using grpc::ServerContext;
using boost::fibers::unbuffered_channel;
using boost::fibers::buffered_channel;

enum GameMessageType { Reset, Commands };
struct GameMessage {
    GameMessageType type;
    std::queue<std::string> data;
};
extern void EndGame();

class RLInterface final : public RLAPI::Service
{

    public:

        grpc::Status Connect(ServerContext* context, const ConnectRequest* req, Observation* obs) override;
        grpc::Status Step(ServerContext* context, const Actions* commands, Observation* obs) override;
        grpc::Status Reset(ServerContext* context, const ResetRequest* req, Observation* obs) override;
        grpc::Status GetTemplates(ServerContext* context, const GetTemplateRequest* req, Templates* res) override;

        void Listen(std::string server_address);
        void ApplyEvents();  // Apply RPC messages to the game engine
        std::string GetGameState();
        // TODO: Add a render method??

    private:
        std::unique_ptr<grpc::Server> m_Server;
        unsigned int m_Turn = 0;
        std::mutex m_lock;
        buffered_channel<GameMessage> m_GameMessages{2};
        unbuffered_channel<std::string> m_GameStates;
        bool m_NeedsGameState = false;
        GameConfig m_GameConfig = GameConfig(L"scenario", L"Arcadia");
};
#endif // INCLUDED_RLINTERFACE
