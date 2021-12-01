#include <iostream>
#include <string>

#include <grpcpp/grpcpp.h>

#include "defs.grpc.pb.h"

using namespace grpc;
using namespace testcallbacks;

#define CHECK_STATUS(status) \
    if (status.ok()) { \
        std::cout << "rpc is ok" << std::endl; \
    } else { \
        std::cout \
            << "rpc is not ok" << std::endl \
            << "msg: " << status.error_message() << std::endl \
            << "code: " << status.error_code() << std::endl; \
    }

struct SimpleClientReactor : public ClientReadReactor<StartStreamMessageOut> {
    std::atomic<bool> done = false;
    std::atomic<bool> readDone = true;

    void OnDone(Status const& status) override {
        done = true;

        CHECK_STATUS(status);
    }

    void OnReadDone(bool ok) override {
        readDone = true;
        if (ok) {
            std::cout << "read was ok" << std::endl;
        } else {
            std::cout << "read was __not__ ok" << std::endl;
        }
    }
};

void RunOneSync(StreamProcessor::Stub* stub) {
    if (stub) {
        std::atomic<bool> done = false;

        auto const deadline = std::chrono::system_clock::now() +
            std::chrono::milliseconds(5000);
        ClientContext ctx;
        //ctx.set_deadline(deadline);
        StartStreamRequest request;
        SimpleClientReactor reactor;
        stub->async()->StartStream(&ctx, &request, &reactor);
        reactor.StartCall();

        StartStreamMessageOut msg;
        while (!reactor.done) {
            reactor.readDone = false;
            reactor.StartRead(&msg);
            while (!reactor.readDone);

            std::cout << msg.value() << std::endl;
        }
        
        std::cout << "all done " << std::endl;
    }
}

int main(int argc, char** argv) {
    std::string target_str = "localhost:50051";
    auto channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
    auto stub = StreamProcessor::NewStub(channel);

    RunOneSync(stub.get());

    return 0;
}
