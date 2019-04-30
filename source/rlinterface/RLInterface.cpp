#include <mutex>
#include <vector>
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "rlinterface/RLAPI.grpc.pb.h"

#include "lib/precompiled.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpAIInterface.h"
#include "simulation2/system/TurnManager.h"
#include "ps/Game.h"
#include "ps/ThreadUtil.h"
#include <boost/fiber/unbuffered_channel.hpp>

using grpc::ServerContext;

class RLInterface final : public RLAPI::Service
{
    typedef boost::fibers::unbuffered_channel<std::vector<std::string>> test_channel_t;
    typedef boost::fibers::unbuffered_channel<std::string> channel_t;

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

            // Apply and edits from the RPC messages to the game engine
            std::vector<std::string> commands;
            while (boost::fibers::channel_op_status::success == m_GameCommands.pop(commands))
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

                g_Game->GetSimulation2()->Update(200);  // FIXME: This updates a fixed time when we want a fixed number of steps...? Or do we want to use time?

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
        test_channel_t m_GameCommands;
        channel_t m_GameStates;
};
