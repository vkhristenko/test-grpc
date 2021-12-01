#include <iostream>
#include <string>
#include <utility>

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
        std::shared_ptr<ClientContext> ctx = std::make_shared<ClientContext>();
        ctx->set_deadline(deadline);
        std::shared_ptr<ProcessRequest> request = std::make_shared<ProcessRequest>();
        auto reply = std::make_shared<ProcessReply>();
        // https://newbedev.com/why-is-a-lambda-not-movable-if-it-captures-a-not-copiable-object-using-std-move
        // can not move into the lambda that we pass below as 
        // std function expects a lambda that is copyable
        stub->async()->Process(ctx.get(), request.get(), reply.get(), 
            [ctx, request, reply, &done] (Status status) {
                CHECK_STATUS(status);
                done = true;
            }
        );

        //bool cancel_tried = false;
        while (!done) {
       /*     if (cancel_tried) continue;
            if (std::chrono::system_clock::now() > deadline) {
                ctx.TryCancel();
                cancel_tried= true;
            }
            */
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
