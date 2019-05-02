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

class RLInterface final : public RLAPI::Service
{

    public:

        grpc::Status Connect(ServerContext* context, const ConnectRequest* _, Observation* obs) override
        {
            std::lock_guard<std::mutex> lock(m_lock);
            std::cout << ">>> Connect" << std::endl;
            //m_GameCommands.push(1);
            // TODO: Initialize the scenario
            return grpc::Status::OK;
        }

        grpc::Status Step(ServerContext* context, const Actions* commands, Observation* obs) override
        {
            std::lock_guard<std::mutex> lock(m_lock);
            std::cout << ">>> Step" << std::endl;
            //g_Profiler2.RecordFrameStart();
            //PROFILE2("frame");
            //g_Profiler2.IncrementFrameNumber();
            //PROFILE2_ATTR("%d", g_Profiler2.GetFrameNumber());

            // TODO: Update this...
            //debug_printf("Turn %u (%u)...\n", m_Turn++, DEFAULT_TURN_LENGTH_SP);

            // Interactions with the game engine (g_Game) must be done in the main
            // thread as there are specific checks for this. We will pass our commands
            // to the main thread to be applied
            const int size = commands->actions_size();
            std::vector<std::string> action_v;
            for (int i = 0; i < size; i++) 
            {
                std::string json_cmd = commands->actions(i).content();
                action_v.push_back(json_cmd);
            }
            m_GameCommands.push(action_v);
            std::string state;
            m_GameStates.pop(state);
            obs->set_content(state);

            //g_Profiler.Frame();

            return grpc::Status::OK;
        }

        grpc::Status Reset(ServerContext* context, const ResetRequest* _, Observation* obs) override
        {
            std::lock_guard<std::mutex> lock(m_lock);
            std::cout << ">>> Reset" << std::endl;
            //m_GameCommands.push(3);
            // TODO: Initialize the scenario and return the game state
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
            std::vector<std::string> commands;
            std::chrono::milliseconds duration(50);
            if (boost::fibers::channel_op_status::success == m_GameCommands.pop_wait_for(commands, duration))
            {
                for (std::string cmd : commands)  // Apply the game commands
                {
                    const ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
                    JSContext* cx = scriptInterface.GetContext();
                    JS::RootedValue command(cx);
                    scriptInterface.ParseJSON(cmd, &command);
                    std::cout << "Posting command: " << cmd << std::endl;
                    g_Game->GetTurnManager()->PostCommand(command);
                }

                // FIXME: Why am I updating the turn manager??
                //g_Game->GetTurnManager()->Update(200, 2);
                g_Game->Update(200);

                // Get the Game State
                m_GameStates.push(GetGameState());
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
        unbuffered_channel<std::vector<std::string>> m_GameCommands;
        unbuffered_channel<std::string> m_GameStates;
        //unbuffered_channel<std::string> m_GameConfigs;
};
