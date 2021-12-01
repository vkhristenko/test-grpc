#include <iostream>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "service.grpc.pb.h"

using namespace grpc;

class EchoService : public echo::Echo::Service {
public:

    Status Test(
            ServerContext* ctx,
            echo::TestRequest const* request,
            echo::TestReply* reply) override { 
        std::cout << ctx->peer() << std::endl;

        return Status::OK; 
    }
};

int main(int argc, char** argv) {
    EchoService service;

    auto const port = static_cast<uint16_t>(std::stoi(argv[1]));
    std::string const addr = "0.0.0.0:" + std::to_string(port);

    ServerBuilder builder;
    builder.AddListeningPort(addr, InsecureServerCredentials());
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    server->Wait();

    return 0;
}
