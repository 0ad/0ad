#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include "rlinterface/RLAPI.grpc.pb.h"

using grpc::ServerContext;

class RLInterface final : public RLAPI::Service
{
    public:
        grpc::Status Connect(ServerContext* context, const ConnectRequest* _, Observation* obs) override
        {
            std::cout << ">>> Connect" << std::endl;
            std::string content("Test Content");
            obs->set_content(content);
            return grpc::Status::OK;
        }

        grpc::Status Step(ServerContext* context, const Actions* actions, Observation* obs) override
        {
            std::cout << ">>> Step" << std::endl;
            return grpc::Status::OK;
        }

        grpc::Status Reset(ServerContext* context, const ResetRequest* _, Observation* obs) override
        {
            std::cout << ">>> Reset" << std::endl;
            return grpc::Status::OK;
        }

        void Listen(std::string server_address)
        {
            grpc::ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(this);
            //std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
            this->server = builder.BuildAndStart();
            std::cout << "Server listening on " << server_address << std::endl;
            this->server->Wait();
        }

        // TODO: Add a render method??
        // TODO: Add a disconnect method??

    private:
        std::unique_ptr<grpc::Server> server;
};
