#include <grpcpp/grpcpp.h>
#include "tinyros.grpc.pb.h"
#include "util/util.hpp"
#include "base/stub.hpp"
#include "base/publisher_proxy.hpp"

using grpc::Status;
using grpc::ClientContext;
using huleibao::ReplyStatusCode;
using huleibao::CommonReply;
using huleibao::TopicData;
using huleibao::AdvertiseRequest;


namespace huleibao
{
PublisherProxy::PublisherProxy(
    std::shared_ptr<StubWrapper> core_stub
    ):
    m_core_stub_(core_stub)
{
}


void PublisherProxy::AdvertiseTopic(
    std::string node_name,
    std::string topic_name,
    int buffer_size)
{
    // - Context for the client. It could be used to convey extra information to
    // - the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // - Data we are sending to the server.
    AdvertiseRequest request;
    request.set_publisher_name(node_name);
    request.set_topic_name(topic_name);
    request.set_buffer_size(buffer_size);
    // - Container for the data we expect from the server.
    CommonReply reply;
    // - The actual RPC.
    Status status = m_core_stub_->GetStub()->AdvertiseTopic(&context, request, &reply);
    // - Act upon its status.
    if (!status.ok())
        LOG_FATAL(" - AdvertiseTopic " <<status.error_code() << ": " << status.error_message());

    if(reply.code() != ReplyStatusCode::success)
        LOG_FATAL(reply.info());

    LOG_INFO(reply.info());
}


void PublisherProxy::Publish(std::string& topic_name, std::vector<uint8_t>& serialize_data)
{
    uint64_t timestamp = GetTimeStamp();
    // - Copy the message and start pushing
    TopicData topicData;
    topicData.set_topic_name(topic_name);
    topicData.set_timestamp(timestamp);
    topicData.set_buffer(serialize_data.data(), serialize_data.size());
    // - Context for the client. It could be used to convey extra information to
    // - the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // - Container for the data we expect from the server.
    CommonReply reply;
    // - Send to Core
    Status status = m_core_stub_->GetStub()->PublishTopic(&context, topicData, &reply);
    // - Act upon its status.
    if (!status.ok())
        LOG_FATAL("AdvertiseTopic " <<status.error_code() << ": " << status.error_message());

    if(reply.code() != ReplyStatusCode::success)
        LOG_FATAL(reply.info());

    LOG_INFO(reply.info());

}


}// namespace huleibao
