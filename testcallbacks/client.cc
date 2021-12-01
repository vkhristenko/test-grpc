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

void RunOneStdFunc(Processor::Stub* stub) {
    if (stub) {
        std::atomic<bool> done = false;

        auto const deadline = std::chrono::system_clock::now() +
            std::chrono::milliseconds(1000);
        ClientContext ctx;
        //ctx.set_deadline(deadline);
        ProcessRequest request;
        ProcessReply reply;
        stub->async()->Process(&ctx, &request, &reply, [&done](Status status) {
            CHECK_STATUS(status);
            done = true;
        });

        bool cancel_tried = false;
        while (!done) {
            if (cancel_tried) continue;
            if (std::chrono::system_clock::now() > deadline) {
                ctx.TryCancel();
                cancel_tried= true;
            }
        }
        std::cout << "all done " << std::endl;
    }
}

void RunOneReactor(Processor::Stub* stub) {
    if (stub) {
        std::atomic<bool> done = false;

        ClientContext ctx;
        ProcessRequest request;
        ProcessReply reply;
        stub->async()->Process(&ctx, &request, &reply, [&done](Status status) {
            CHECK_STATUS(status);
            done = true;
        });

        while (!done);
        std::cout << "all done " << std::endl;
    }
}

int main(int argc, char** argv) {
    std::string target_str = "localhost:50051";
    auto channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
    auto stub = Processor::NewStub(channel);

    RunOneStdFunc(stub.get());

    return 0;
}
