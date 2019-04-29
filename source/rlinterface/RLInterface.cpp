#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "rlinterface/RLAPI.grpc.pb.h"

#include "lib/precompiled.h"
#include "simulation2/Simulation2.h"
#include "ps/Game.h"

using grpc::ServerContext;

class RLInterface final : public RLAPI::Service
{
    public:

        // TODO: Add a mutex for interacting with the game
        grpc::Status Connect(ServerContext* context, const ConnectRequest* _, Observation* obs) override
        {
            std::cout << ">>> Connect" << std::endl;
            // TODO: Initialize the scenario
            return grpc::Status::OK;
        }

        grpc::Status Step(ServerContext* context, const Actions* actions, Observation* obs) override
        {
            std::cout << ">>> Step" << std::endl;
            // TODO: Step the game X ticks and return the resultant game state
            // TODO: If the game is over, set that in the observation, too??

            //g_Profiler2.RecordFrameStart();
            //PROFILE2("frame");
            //g_Profiler2.IncrementFrameNumber();
            //PROFILE2_ATTR("%d", g_Profiler2.GetFrameNumber());

            // TODO: Update this...
            //debug_printf("Turn %u (%u)...\n", m_Turn++, DEFAULT_TURN_LENGTH_SP);

            g_Game->GetSimulation2()->Update(200);

            //g_Profiler.Frame();

            return grpc::Status::OK;
        }

        grpc::Status Reset(ServerContext* context, const ResetRequest* _, Observation* obs) override
        {
            std::cout << ">>> Reset" << std::endl;
            // TODO: Initialize the scenario and return the game state
            return grpc::Status::OK;
        }

        void Listen(std::string server_address)
        {
            grpc::ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(this);
            m_Server = builder.BuildAndStart();
            std::cout << "Server listening on " << server_address << std::endl;
            std::cout << "About to set the sim rate to " << m_StepsBtwnActions << std::endl;
            if (!g_Game)
            {
                    std::cout << "g_Game is falsey!" << std::endl;
            }
            g_Game->SetSimRate(m_StepsBtwnActions);
            std::cout << "Set the sim rate to " << m_StepsBtwnActions << std::endl;

            m_Server->Wait();
        }

        // TODO: Add a render method??
        // TODO: Add a disconnect method??
        // TODO: Add a setSpeed method??

    private:
        std::unique_ptr<grpc::Server> m_Server;
        float m_StepsBtwnActions = 10.0f;
        unsigned int m_Turn = 0;
};
