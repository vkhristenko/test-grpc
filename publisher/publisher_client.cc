#include <iostream>
#include <string>

#include <grpcpp/grpcpp.h>

#include "publisher.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using publisher::PubSubService;
using publisher::SubscribeOneRequest;
using publisher::SubscribeOneReply;
using publisher::SubscribeFourRequest;
using publisher::SubscribeFourReply;

int main(int argc, char** argv) {
    std::string target_str = "localhost:50051";
    auto channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
    auto stub = PubSubService::NewStub(channel);

    ClientContext ctx;
    SubscribeOneRequest request;
    SubscribeOneReply reply;
    auto status = stub->SubscribeOne(&ctx, request, &reply);

    if (not status.ok()) {
        std::cout << "RPC failed. exiting..." << std::endl;
        return 1;
    } else {
        std::cout << "rpc succeeded" << std::endl;
    }

    {
        ClientContext ctx;
        SubscribeFourRequest request;
        auto reader = stub->SubscribeFour(&ctx, request);

        SubscribeFourReply reply;
        while (reader->Read(&reply)) {
            std::cout << "one more reply" << std::endl;
        }
        auto status = reader->Finish();
        if (not status.ok()) {
            std::cout << "RPC failed. exiting..." << std::endl;
            return 1;
        } else {
            std::cout << "rpc succeeded" << std::endl;
        }
    }

    return 0;
}
