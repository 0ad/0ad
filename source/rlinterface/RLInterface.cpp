#include <mutex>
#include <vector>
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "rlinterface/RLAPI.grpc.pb.h"

#include "lib/precompiled.h"
#include "lib/external_libraries/libsdl.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpAIInterface.h"
#include "simulation2/system/TurnManager.h"
#include "ps/Game.h"
#include "ps/Loader.h"
#include "gui/GUIManager.h"
#include "ps/VideoMode.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/GameSetup/GameConfig.h"
#include "ps/ThreadUtil.h"
#include <boost/fiber/unbuffered_channel.hpp>

using grpc::ServerContext;
using boost::fibers::unbuffered_channel;

enum GameMessageType { Reset, Command };
struct GameMessage {
    GameMessageType type;
    std::string data;
};
extern void EndGame();

class RLInterface final : public RLAPI::Service
{

    public:

        grpc::Status Connect(ServerContext* context, const ConnectRequest* req, Observation* obs) override
        {
            std::cout << ">>> Aquiring lock for Connect" << std::endl;
            std::lock_guard<std::mutex> lock(m_lock);

            if (req->has_scenario())
            {
                m_GameConfig = GameConfig::from(req->scenario());
            }

            GameMessage msg = { GameMessageType::Reset };
            m_GameMessages.push_back(msg);

            std::string state;
            m_GameStates.pop(state);
            obs->set_content(state);

            std::cout << ">>> Connect Complete" << std::endl;
            return grpc::Status::OK;
        }

        grpc::Status Step(ServerContext* context, const Actions* commands, Observation* obs) override
        {
            std::cout << ">>> Acquiring lock for Step" << std::endl;
            std::lock_guard<std::mutex> lock(m_lock);

            // Interactions with the game engine (g_Game) must be done in the main
            // thread as there are specific checks for this. We will pass our commands
            // to the main thread to be applied
            const int size = commands->actions_size();
            for (int i = 0; i < size; i++) 
            {
                std::string json_cmd = commands->actions(i).content();
                struct GameMessage msg = { GameMessageType::Command, json_cmd };
                m_GameMessages.push_back(msg);
            }
            std::string state;
            m_GameStates.pop(state);
            obs->set_content(state);

            std::cout << ">>> Step Complete" << std::endl;
            return grpc::Status::OK;
        }

        grpc::Status Reset(ServerContext* context, const ResetRequest* req, Observation* obs) override
        {
            std::cout << ">>> Acquiring lock for Reset" << std::endl;
            std::lock_guard<std::mutex> lock(m_lock);
            if (req->has_scenario())
            {
                m_GameConfig = GameConfig::from(req->scenario());
            }
            struct GameMessage msg = { GameMessageType::Reset };
            m_GameMessages.push_back(msg);

            std::string state;
            m_GameStates.pop(state);
            obs->set_content(state);

            std::cout << ">>> Reset Complete" << std::endl;
            return grpc::Status::OK;
        }

        void Listen(std::string server_address)
        {
            g_Game->SetSimRate(m_StepsBtwnActions);
            grpc::ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(this);
            m_Server = builder.BuildAndStart();
            std::cout << "Server listening on " << server_address << std::endl;
        }

        void ApplyEvents()
        {
            // Apply and edits from the RPC messages to the game engine
            if (m_NeedsGameState && g_Game->IsGameStarted())
            {
                //m_GameStates.push(GetGameState());  // Send the game state back to the request
                std::cout << "Skipping sending the game state. Game should be ready." << std::endl;
                m_NeedsGameState = false;
            }

            bool shouldStepGame = false;
            while (m_GameMessages.size() > 0)
            {
                GameMessage msg = m_GameMessages.back();
                m_GameMessages.pop_back();

                std::cout << "Applying game message!" << std::endl;
                switch (msg.type)
                {
                    case GameMessageType::Reset:
                        {
                            const bool nonVisual = !g_GUI;
                            std::cout << "Received reset command!!" << std::endl;
                            EndGame();

                            m_GameConfig.nonVisual = nonVisual;
                            const bool saveReplay = !m_GameConfig.nonVisual;
                            g_Game = new CGame(m_GameConfig.nonVisual, saveReplay);

                            ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
                            JSContext* cx = scriptInterface.GetContext();
                            JS::RootedValue attrs(cx, m_GameConfig.toJSValue(scriptInterface));

                            g_Game->SetPlayerID(m_GameConfig.playerID);
                            g_Game->StartGame(&attrs, "");

                            if (nonVisual)
                            {
                                LDR_NonprogressiveLoad();  // TODO: Can this be used for visual mode, too?
                                ENSURE(g_Game->ReallyStartGame() == PSRETURN_OK);
                                m_GameStates.push(GetGameState());  // Send the game state back to the request
                            }
                            else
                            {
                                JS::RootedValue initData(cx);
                                scriptInterface.Eval("({})", &initData);
                                scriptInterface.SetProperty(initData, "attribs", attrs);

                                JS::RootedValue playerAssignments(cx);
                                scriptInterface.Eval("({})", &playerAssignments);
                                scriptInterface.SetProperty(initData, "playerAssignments", playerAssignments);

                                g_GUI->SwitchPage(L"page_loading.xml", &scriptInterface, initData);
                                m_NeedsGameState = true;  // FIXME: Can we load immediately? Or should we stick with progressive loading?
                            }
                        }
                        break;
                    case GameMessageType::Command:
                        const ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
                        JSContext* cx = scriptInterface.GetContext();
                        JS::RootedValue command(cx);
                        scriptInterface.ParseJSON(msg.data, &command);
                        std::cout << "Posting command: " << msg.data << std::endl;
                        g_Game->GetTurnManager()->PostCommand(command);
                        shouldStepGame = true;
                        break;
                }
            }

            if (shouldStepGame)
            {
                g_Game->Update(200);
                m_GameStates.push(GetGameState());  // Send the game state back to the request
            }
        }

        std::string GetGameState()
        {
            const ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
            const auto simContext = g_Game->GetSimulation2()->GetSimContext();
            CmpPtr<ICmpAIInterface> cmpAIInterface(simContext.GetSystemEntity());
            JSContext* cx = scriptInterface.GetContext();
            JS::RootedValue state(cx);
            cmpAIInterface->GetFullRepresentation(&state, true);
            return scriptInterface.StringifyJSON(&state, false);
        }

        // TODO: Add a render method??
        // TODO: Add a disconnect method??
        // TODO: Add a setSpeed method??

    private:
        std::unique_ptr<grpc::Server> m_Server;
        float m_StepsBtwnActions = 10.0f;
        unsigned int m_Turn = 0;
        std::mutex m_lock;
        std::vector<GameMessage> m_GameMessages;
        unbuffered_channel<std::string> m_GameStates;
        bool m_NeedsGameState = false;
        GameConfig m_GameConfig = GameConfig(L"scenario", L"Arcadia");
        //unbuffered_channel<std::string> m_GameConfigs;
};
