#include <iostream> 
#include <string>
#include <unordered_map>
#include <thread>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "publisher.grpc.pb.h"

using namespace grpc;

using publisher::PubSubService;
using publisher::SubscribeOneRequest;
using publisher::SubscribeOneReply;
using publisher::SubscribeTwoRequest;
using publisher::SubscribeTwoReply;
using publisher::SubscribeThreeRequest;
using publisher::SubscribeThreeReply;
using publisher::SubscribeFourRequest;
using publisher::SubscribeFourReply;
    
std::atomic<bool> g_shouldRun{false};

struct MyReactor : public ServerUnaryReactor {
    MyReactor(
            SubscribeOneRequest const* request,
            SubscribeOneReply* reply)
        : request_{request}
        , reply_{reply}
    {}

    void OnDone() override {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        delete this;
    }

    SubscribeOneRequest const* request_ = nullptr;
    SubscribeOneReply* reply_ = nullptr;
};
MyReactor* g_r = nullptr;

std::atomic<bool> g_shouldWriteNext{false};
struct MyWriteReactor : public ServerWriteReactor<SubscribeFourReply> {
    MyWriteReactor(SubscribeFourRequest const* request) : request_{request} {
        g_shouldWriteNext = true;
    }

    void OnWriteDone(bool) override {
        static int counter = 0;
        counter++;
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        if (counter==5)
            Finish(Status::OK);
        else
            g_shouldWriteNext = true;
    }

    void OnDone() override {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        delete this;
    }

    SubscribeFourRequest const* request_ = nullptr;
};
MyWriteReactor* g_wr = nullptr;

void Process() {
    while (true) {
        if (g_shouldRun) {
            // set reply if needed
            g_r->Finish(Status::OK);
            g_shouldRun = false;
        }

        if (g_shouldWriteNext) {
            g_shouldWriteNext = false;
            SubscribeFourReply reply;
            g_wr->StartWrite(&reply);
        }
    }
}

class Publisher : public PubSubService::CallbackService {
public:
    Publisher() {}

    ServerUnaryReactor* SubscribeOne(
            CallbackServerContext* ctx,
            SubscribeOneRequest const* request,
            SubscribeOneReply* reply) override {
        g_r = new MyReactor{request, reply};
        g_shouldRun = true;
        return g_r;
    }

    ServerWriteReactor<SubscribeFourReply>*
    SubscribeFour(
            CallbackServerContext* ctx,
            SubscribeFourRequest const* request) override {
        g_wr = new MyWriteReactor{request};
        return g_wr;
    }
};

void Run() {
    std::thread t{Process};

    std::string addr{"localhost:50051"};
    Publisher service;

    ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    // completion queue for async comm
    auto server = builder.BuildAndStart();
    server->Wait();
    t.join();
}

int main() {
    Run();

    return 0;
}
