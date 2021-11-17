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
    
std::unique_ptr<ServerCompletionQueue> cq1 = nullptr, cq2 = nullptr;
std::unique_ptr<Server> server = nullptr;
PubSubService::AsyncService service;

class CallData {
public:
};

void HandleRpcs(ServerCompletionQueue* cq, int id) {
    ServerContext context;
    SubscribeOneRequest request;
    ServerAsyncResponseWriter<SubscribeOneReply> responder{&context};
    service.RequestSubscribeOne(&context, &request, &responder, cq, cq, (void*)1);

    std::cout << "requested call in pipeline " << id  << std::endl;

    {
        SubscribeOneReply reply;
        Status status;
        void* got_tag;
        bool ok = false;
        cq->Next(&got_tag, &ok);
        if (ok && got_tag == (void*)1) {
            // set reply and status
            responder.Finish(reply, status, (void*)2);
        }
    }

    std::cout << "wrote to cq" << std::endl;

    {
        void* got_tag;
        bool ok = false;
        cq->Next(&got_tag, &ok);
        if (ok && got_tag == (void*)2) {
            // clean up
        }
    }

    std::cout << "rpc is finished" << std::endl;

    HandleRpcs(cq, id);
}

void Run() {
    std::string addr{"localhost:50051"};

    ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    // completion queue for async comm
    cq1 = builder.AddCompletionQueue();
    cq2 = builder.AddCompletionQueue();
    // our server
    server = builder.BuildAndStart();

    std::thread t1{HandleRpcs, cq1.get(), 1};
    //std::thread t3{HandleRpcs, cq3.get(), 3};
    HandleRpcs(cq2.get(), 2);
    t1.join();
}

int main() {
    Run();

    // order is important!
    server->Shutdown();
    cq1->Shutdown();
    cq2->Shutdown();

    // have to drain this manually
    void* ignored_tag;
    bool ignored_ok;
    while (cq1->Next(&ignored_tag, &ignored_ok)) { }
    while (cq2->Next(&ignored_tag, &ignored_ok)) { }

    std::cout << "things are shut down!" << std::endl;

    return 0;
}
