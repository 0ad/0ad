#include "RLInterface.h"
#include <grpcpp/server_context.h>

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

void RLInterface::Listen(std::string server_address)
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(this);
    //std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    this->server = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;
    this->server->Wait();
}

