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
}


Node::~Node()
{
    // -Disconnect itself to core
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
        LOG_FATAL(" - HiCore " <<status.error_code() << ": " << status.error_message());
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
        LOG_FATAL(" - ByeCore " <<status.error_code() << ": " << status.error_message());
    }

    if(reply.code() != ReplyStatusCode::success)
    {
        LOG_INFO("[node] ByeCore reply.code= " << reply.code() << 
            ", reply.info=" << reply.info());
        LOG_FATAL(reply.info());
    }

    LOG_INFO(reply.info());
}

/*
template <typename MessageType>
typename Publisher<MessageType>::Ptr Node::Advertise(
    std::string topic_name, int buffer_size)
{
    return Publisher<MessageType>::Ptr(new Publisher<MessageType>(
        m_core_stub_, m_node_name_, topic_name, buffer_size));
}


template <typename MessageType>
typename Subscriber<MessageType>::Ptr Node::Subscribe(
    std::string topic_name,
    int buffer_size,
    void(*callback_func)(typename MessageType::ConstPtr))
{
    return Subscriber<MessageType>::Ptr(new Subscriber<MessageType>(
        m_core_stub_, m_node_name_, topic_name, buffer_size, callback_func));
}


template <typename MessageType, typename T>
typename Subscriber<MessageType>::Ptr Node::Subscribe(
    std::string topic_name,
    int buffer_size,
    void (T::* callback_func)(typename MessageType::ConstPtr),
    T* obj)
{
    auto cbfunc = std::bind(callback_func, obj, std::placeholders::_1);
    return Subscriber<MessageType>::Ptr(new Subscriber<MessageType>(
        m_core_stub_, m_node_name_, topic_name, buffer_size, cbfunc));
}
*/
} // namespace huleibao
