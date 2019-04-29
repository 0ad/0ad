#include <mutex>
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "rlinterface/RLAPI.grpc.pb.h"

#include "lib/precompiled.h"
#include "simulation2/Simulation2.h"
#include "ps/Game.h"
#include "ps/ThreadUtil.h"
#include <boost/fiber/unbuffered_channel.hpp>

using grpc::ServerContext;

class RLInterface final : public RLAPI::Service
{
    typedef boost::fibers::unbuffered_channel<int> channel_t;

    public:

        grpc::Status Connect(ServerContext* context, const ConnectRequest* _, Observation* obs) override
        {
            std::lock_guard<std::mutex> lock(m_lock);
            std::cout << ">>> Connect" << std::endl;
            m_GameCommands.push(1);
            // TODO: Initialize the scenario
            return grpc::Status::OK;
        }

        grpc::Status Step(ServerContext* context, const Actions* actions, Observation* obs) override
        {
            std::lock_guard<std::mutex> lock(m_lock);
            std::cout << ">>> Step" << std::endl;
            // TODO: Step the game X ticks and return the resultant game state
            // TODO: If the game is over, set that in the observation, too??

            //g_Profiler2.RecordFrameStart();
            //PROFILE2("frame");
            //g_Profiler2.IncrementFrameNumber();
            //PROFILE2_ATTR("%d", g_Profiler2.GetFrameNumber());

            // TODO: Update this...
            //debug_printf("Turn %u (%u)...\n", m_Turn++, DEFAULT_TURN_LENGTH_SP);

            m_GameCommands.push(2);
            int state = 0;
            m_GameStates.pop(state);
            std::cout << "game state is " << state << std::endl;

            //g_Profiler.Frame();

            return grpc::Status::OK;
        }

        grpc::Status Reset(ServerContext* context, const ResetRequest* _, Observation* obs) override
        {
            std::lock_guard<std::mutex> lock(m_lock);
            std::cout << ">>> Reset" << std::endl;
            m_GameCommands.push(3);
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

            // TODO: Create two channels for interacting with the game engine...
            int msg = 0;
            while (boost::fibers::channel_op_status::success == m_GameCommands.pop(msg))
            {
                // TODO: handle the message and return the game state
                if (msg == 2) {
                    std::cout << "updating game state" << std::endl;
                    g_Game->GetSimulation2()->Update(200);
                    m_GameStates.push(5);
                }
            }
        }

        // TODO: Add a render method??
        // TODO: Add a disconnect method??
        // TODO: Add a setSpeed method??

    private:
        std::unique_ptr<grpc::Server> m_Server;
        float m_StepsBtwnActions = 10.0f;
        unsigned int m_Turn = 0;
        std::mutex m_lock;
        channel_t m_GameCommands;
        channel_t m_GameStates;
};
