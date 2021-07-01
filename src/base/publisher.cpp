#include <grpcpp/grpcpp.h>
#include "tinyros.grpc.pb.h"
#include "util/util.hpp"
#include "base/publisher.hpp"
#include "base/stub.hpp"
#include "message/std_msgs.hpp"


using grpc::Status;
using grpc::ClientContext;
using huleibao::CoreSrv;
using huleibao::ReplyStatusCode;
using huleibao::CommonReply;
using huleibao::TopicData;
using huleibao::AdvertiseRequest;


namespace huleibao
{

template<typename MessageType>
Publisher<MessageType>::Publisher(
    std::shared_ptr<StubWrapper> core_stub,
    std::string node_name,
    std::string topic_name,
    int buffer_size)
    :m_node_name_(node_name), m_topic_name_(topic_name), m_buffer_size_(buffer_size),
    m_core_stub_(core_stub)
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
	Status status = core_stub->GetStub()->AdvertiseTopic(&context, request, &reply);
    // - Act upon its status.
	if (!status.ok())
        LOG_FATAL(" - AdvertiseTopic " <<status.error_code() << ": " << status.error_message());

    if(reply.code() != ReplyStatusCode::success)
        LOG_FATAL(reply.info());

    LOG_INFO(reply.info());
}


template<typename MessageType>
void Publisher<MessageType>::Publish(MessageType msg)
{
    // - Serialized message
    std::vector<uint8_t> serializeData = msg.Serialize();
    uint64_t timestamp = Util::GetTimeStamp();
    // - Copy the message and start pushing
    TopicData topicData;
    topicData.set_topic_name(m_topic_name_);
    topicData.set_timestamp(timestamp);
    topicData.set_buffer(serializeData.data(), serializeData.size());
    // - Context for the client. It could be used to convey extra information to
	// - the server and/or tweak certain RPC behaviors.
	ClientContext context;
    // - Container for the data we expect from the server.
    CommonReply reply;
    // - Send to Core
    Status status = m_core_stub_->GetStub()->PublishTopic(&context, topicData, &reply);
    // - Act upon its status.
	if (!status.ok())
        LOG_FATAL(" - AdvertiseTopic " <<status.error_code() << ": " << status.error_message());

    if(reply.code() != ReplyStatusCode::success)
        LOG_FATAL(reply.info());

    LOG_INFO(reply.info());
}

} // namespace huleibao
