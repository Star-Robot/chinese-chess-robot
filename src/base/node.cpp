#include <grpcpp/grpcpp.h>
#include "tinyros.grpc.pb.h"
#include "util/util.hpp"
#include "base/node.hpp"
#include "base/stub.hpp"
#include "message/std_msgs.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::Channel;
using grpc::ClientContext;

using huleibao::TopicData;
using huleibao::ReplyStatusCode;
using huleibao::CoreSrv;


namespace huleibao
{

Node::Node(std::string node_name)
{
    m_node_name_ = node_name;
    // - Try to connect to the core and get the stub
    m_core_stub_.reset(new StubWrapper());
    // -Introduce itself to core
    HiCore();
    // - Register signal SIGINT and signal handle
    signal(SIGINT, SignalHandler);
}


Node::~Node()
{
    // -Disconnect itself from core
    ByeCore();
}


void Node::HiCore()
{
	ClientContext context;
	GreetData request;
    request.set_node_name(m_node_name_);
	CommonReply reply;
	// The actual RPC.
	Status status = m_core_stub_->GetStub()->NodeGreet(&context, request, &reply);
	// Act upon its status.
	if (!status.ok())
    {
        LOG_FATAL("HiCore " <<status.error_code() << ": " << status.error_message());
    }

    if(reply.code() != ReplyStatusCode::success)
    {
        LOG_INFO("[node] HiCore reply.code= " << reply.code() << 
            ", reply.info=" << reply.info());
        LOG_FATAL(reply.info());
    }

    LOG_INFO(reply.info());
}


void Node::ByeCore()
{
	ClientContext context;
	GreetData request;
    request.set_node_name(m_node_name_);
	CommonReply reply;
	// The actual RPC.
	Status status = m_core_stub_->GetStub()->NodeDisconnect(&context, request, &reply);
	// Act upon its status.
	if (!status.ok())
    {
        LOG_FATAL("ByeCore " <<status.error_code() << ": " << status.error_message());
    }

    if(reply.code() != ReplyStatusCode::success)
    {
        LOG_INFO("[node] ByeCore reply.code= " << reply.code() << 
            ", reply.info=" << reply.info());
        LOG_FATAL(reply.info());
    }

    LOG_INFO(reply.info());
}

} // namespace huleibao
