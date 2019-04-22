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
        grpc::Status Connect(ServerContext* context, const ConnectRequest* _, const Observation* obs);
        grpc::Status Step(ServerContext* context, const Actions* actions, const Observation* obs);
        grpc::Status Reset(ServerContext* context, const ResetRequest* _, const Observation* obs);
        void Listen(std::string server_address);
        // TODO: Add a render method??
        // TODO: Add a disconnect method??

    private:
        std::unique_ptr<grpc::Server> server;
};
