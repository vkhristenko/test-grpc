#include <iostream> 
#include <string>
#include <unordered_map>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "publisher.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using publisher::Publisher;
using publisher::TickRequest;
using publisher::TickReply;

class PublisherServiceImpl final : public Publisher::Service {
public:


private:
    Status GetNextTick(ServerContext* ctx, 
            TickRequest const* request,
            TickReply* reply) {
        std::cout << "served client" << std::endl;
        return Status::OK;
    }

    // market data
    std::unordered_map<std::string, std::vector<publisher::Tick>> ticks;

    void Server::parseInput() {
        std::ifstream f{pathToData};
        if (!f.is_open()) throw FileError{pathToData};

        std::string buffer;
        unsigned int nlines = 0;
        while (std::getline(f, buffer)) {
            // skip the header...
            if (buffer[0] == '#') { nlines++; continue; }

            // fill in according to the symbol
            auto tick = msg_inf::Tick::fromString(buffer);
            auto it = ticks.find(tick.symbol);
            if (it == ticks.end()) {
                ticks[tick.symbol].push_back(tick);
            } else {
                it->second.push_back(tick);
            }
            nlines++;
        }
    }
};

int main(int argc, char** argv) {
    std::string addr{"localhost:50051"};
    PublisherServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
      // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << addr << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();

    return 0;
}
