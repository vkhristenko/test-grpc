#include <iostream>
#include <string>

#include <grpcpp/grpcpp.h>

#include "service.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using namespace echo;

using namespace std::string_literals;

int main(int argc, char** argv) {
    auto const targetPort = static_cast<uint16_t>(std::stoi(argv[1]));
    auto const target = "localhost:"s + std::to_string(targetPort);

    auto ch = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    auto stub = Echo::NewStub(ch);

    {
        ClientContext ctx;
        TestRequest request;
        TestReply reply;
        auto status = stub->Test(&ctx, request, &reply);
        if (status.ok()) {
            std::cout << "rpc is ok" << std::endl;
        } else {
            std::cout << "rpc is not ok" << std::endl;
        }
    }

    return 0;
}
