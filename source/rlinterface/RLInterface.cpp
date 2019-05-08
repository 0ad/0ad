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
#include "ps/VideoMode.h"
#include "ps/GameSetup/GameSetup.h"
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

        grpc::Status Connect(ServerContext* context, const ConnectRequest* _, Observation* obs) override
        {
            std::cout << ">>> Aquiring lock for Connect" << std::endl;
            std::lock_guard<std::mutex> lock(m_lock);

            std::cout << ">>> Acquired!" << std::endl;
            std::string config("scenarios/Arcadia");
            std::cout << "1" << std::endl;
            struct GameMessage msg = { GameMessageType::Reset, config };
            std::cout << "2" << std::endl;
            m_GameMessages.push_back(msg);

            std::cout << ">>> waiting for game state..." << std::endl;
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
            // TODO: Update this...
            //debug_printf("Turn %u (%u)...\n", m_Turn++, DEFAULT_TURN_LENGTH_SP);

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

        grpc::Status Reset(ServerContext* context, const ResetRequest* _, Observation* obs) override
        {
            std::cout << ">>> Acquiring lock for Reset" << std::endl;
            std::lock_guard<std::mutex> lock(m_lock);
            //m_GameMessages.push(3);
            // TODO: Initialize the scenario and return the game state
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
            bool shouldStepGame = false;
            while (m_GameMessages.size() > 0)
            {
                GameMessage msg = m_GameMessages.back();
                m_GameMessages.pop_back();

                std::cout << "Applying game message!" << std::endl;
                switch (msg.type)
                {
                    case GameMessageType::Reset:
                        std::cout << "Received reset command!!" << std::endl;
                        std::cout << "Ending Game" << std::endl;
                        EndGame();
                        // TODO: Save the metadata files
                        // TODO: Get the game info??
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
        //unbuffered_channel<std::string> m_GameConfigs;
};
