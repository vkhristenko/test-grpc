#include <iostream>
#include <string>

#include <grpcpp/grpcpp.h>

#include "publisher.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using publisher::Publisher;
using publisher::TickRequest;
using publisher::TickReply;

int main(int argc, char** argv) {
    std::string target_str = "localhost:50051";
    auto channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
    auto stub = Publisher::NewStub(channel);

    ClientContext ctx;
    TickRequest request;
    TickReply reply;
    auto status = stub->GetNextTick(&ctx, request, &reply);

    if (not status.ok()) {
        std::cout << "RPC failed. exiting..." << std::endl;
        return 1;
    }

    return 0;
}
