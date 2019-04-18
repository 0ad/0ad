#include <grpc/grpc.h>
#include <grpcpp/server_context.h>
#include "rlinterface/RLInterface.grpc.pb.h"
//#include "rlinterface/RLInterface.pb.h"

using grpc::ServerContext;

class RLInterfaceImpl final : public RLInterface::Service
{
    public:
        grpc::Status Connect(ServerContext* context, const ConnectRequest* _, const Observation* obs) 
        {
            std::cout << ">>> Connect" << std::endl;
            return grpc::Status::OK;
        }

        grpc::Status Step(ServerContext* context, const Actions* actions, const Observation* obs) 
        {
            std::cout << ">>> Step" << std::endl;
            return grpc::Status::OK;
        }

        grpc::Status Reset(ServerContext* context, const ResetRequest* _, const Observation* obs) 
        {
            std::cout << ">>> Reset" << std::endl;
            return grpc::Status::OK;
        }

        // TODO: Add a render method??
        // TODO: Add a disconnect method??
};
