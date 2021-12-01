#include <iostream> 
#include <mutex>
#include <string>
#include <unordered_map>
#include <thread>
#include <queue>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "defs.grpc.pb.h"

using namespace grpc;
using namespace testcallbacks;

struct SimpleServerReactor : public ServerUnaryReactor {
    SimpleServerReactor() {}

    void OnDone() override {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        delete this;
    }
};

struct Request {
    SimpleServerReactor* reactor = nullptr;
};

std::mutex mu;
std::queue<Request> requests;

void RunRequests() {
    for (;;) {
        Request request;
        {
            std::lock_guard<std::mutex> lck{mu};
            if (requests.size() > 0)
                request = requests.front();
        }
        
        if (request.reactor) {
            std::cout << "Processing request" << std::endl;
            request.reactor->Finish(Status::OK);

            {
                std::lock_guard<std::mutex> lck{mu};
                requests.pop();
            }
        }
    }
}

class ProcessorImpl : public Processor::CallbackService {
public:
    ProcessorImpl() {}

    ServerUnaryReactor* Process(
            CallbackServerContext* ctx,
            ProcessRequest const* request,
            ProcessReply* reply) override {
        std::cout << __PRETTY_FUNCTION__ << std::endl;

        auto reactor = new SimpleServerReactor{};
        {
            std::lock_guard<std::mutex> lck{mu};
            requests.push(Request{reactor});
        }

        return reactor; 
    }
};

void Run() {
    std::thread t{RunRequests};

    std::string addr{"localhost:50051"};
    ProcessorImpl service;

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
