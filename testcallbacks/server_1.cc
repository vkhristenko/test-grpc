#include <iostream> 
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <thread>
#include <queue>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "defs.grpc.pb.h"
#include "common.h"

using namespace grpc;
using namespace testcallbacks;

struct RequestManager;

struct SimpleServerReactor : public ServerUnaryReactor {
    SimpleServerReactor(RequestManager& manager) : manager{manager} {}
    RequestManager& manager;

    void OnDone() override;
};

struct Request {
    Request(RequestManager& manager, CallbackServerContext* ctx) 
        : reactor{manager} 
        , ctx{ctx}
    {}
    SimpleServerReactor reactor;
    CallbackServerContext* ctx;
};

struct RequestManager {
    RequestManager() {}

    std::mutex mu;
    std::unordered_map<size_t, std::unique_ptr<Request>> all;
};
    
std::mutex mu;
std::queue<Request*> requests;
RequestManager manager;

void SimpleServerReactor::OnDone() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    {
        std::lock_guard<std::mutex> lck{manager.mu};
        manager.all.erase((size_t)this);
    }
}

void RunRequests() {
    for (;;) {
        Request* request = nullptr;
        {
            std::lock_guard<std::mutex> lck{mu};
            if (requests.size() > 0)
                request = requests.front();
        }
        
        if (request) {
            std::cout << "Processing request" << std::endl;
            auto secs = std::chrono::seconds(GenRandom());
            std::this_thread::sleep_for(secs);
            std::cout << "slept for " << secs.count() << std::endl;
            if (request->ctx->IsCancelled()) {
                request->reactor.Finish(Status::CANCELLED);
                std::cout << "request was cancelled" << std::endl;
            } else {
                request->reactor.Finish(Status::OK);
            }

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

        auto req = std::make_unique<Request>(manager, ctx);
        auto req_ptr = req.get();
        {
            std::lock_guard<std::mutex> lck{manager.mu};
            manager.all[(size_t)req.get()] = std::move(req);
        }
        {
            std::lock_guard<std::mutex> lck{mu};
            requests.push(req_ptr);
        }

        return &req_ptr->reactor; 
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
