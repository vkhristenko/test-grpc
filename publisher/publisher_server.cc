#include <iostream> 
#include <string>
#include <unordered_map>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "publisher.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using publisher::PubSubService;
using publisher::SubscribeOneRequest;
using publisher::SubscribeOneReply;
using publisher::SubscribeTwoRequest;
using publisher::SubscribeTwoReply;
using publisher::SubscribeThreeRequest;
using publisher::SubscribeThreeReply;

class Publisher final : public PubSubService::Service {
public:


private:
    Status SubscribeOne(
            ServerContext* ctx,
            SubscribeOneRequest const* request,
            SubscribeOneReply* reply) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;

        return Status::OK;
    }
    
    Status SubscribeTwo(
            ServerContext* ctx,
            SubscribeTwoRequest const* request,
            SubscribeTwoReply* reply) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;

        return Status::OK;
    }
    
    Status SubscribeThree(
            ServerContext* ctx,
            SubscribeThreeRequest const* request,
            SubscribeThreeReply* reply) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;

        return Status::OK;
    }
};

int main(int argc, char** argv) {
    std::string addr{"localhost:50051"};
    Publisher service;

    //grpc::EnableDefaultHealthCheckService(true);
    //grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
      // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << addr << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();

    return 0;
}
