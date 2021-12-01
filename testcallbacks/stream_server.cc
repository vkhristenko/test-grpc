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

struct SimpleServerReactor : public ServerWriteReactor<StartStreamMessageOut> {
    SimpleServerReactor(RequestManager& manager)
        : manager{manager} 
    {}
    RequestManager& manager;
    std::atomic<bool> writeDone = true;

    void OnDone() override;
    void OnWriteDone(bool) override;
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

void SimpleServerReactor::OnWriteDone(bool ok) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    // TODO what do we do in case of not ok ?
    writeDone = true;
    if (ok) {
        std::cout << "write was ok" << std::endl;
    } else {
        std::cout << "write was __not__ ok" << std::endl;
    }
}

void WriteMsgs(Request* req, unsigned int const n) {
    for (unsigned int i=0; i<n; i++) {
        auto secs = std::chrono::seconds(GenRandom());
        std::this_thread::sleep_for(secs);
        std::cout << "slept for " << secs.count() << " sending msg now..."<< std::endl;
        StartStreamMessageOut msg;
        msg.set_value(i);
        req->reactor.writeDone = false;
        req->reactor.StartWrite(&msg);
        while (!req->reactor.writeDone);
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
            WriteMsgs(request, 10);
            /*
            std::cout << "Processing request" << std::endl;
            auto secs = std::chrono::seconds(GenRandom());
            std::this_thread::sleep_for(secs);
            std::cout << "slept for " << secs.count() << std::endl;
            */
            request->reactor.Finish(Status::OK);

            {
                std::lock_guard<std::mutex> lck{mu};
                requests.pop();
            }
        }
    }
}

class StreamProcessorImpl : public StreamProcessor::CallbackService {
public:
    StreamProcessorImpl() {}

    ServerWriteReactor<StartStreamMessageOut>* 
    StartStream(
            CallbackServerContext* ctx,
            StartStreamRequest const* request) override {
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
    StreamProcessorImpl service;

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
