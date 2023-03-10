#include <grpcpp/grpcpp.h>
#include "tinyros.grpc.pb.h"
#include "util/util.hpp"
#include "base/stub.hpp"
#include "base/subscriber_proxy.hpp"

using grpc::ClientContext;
using grpc::ClientReader;
using tinyros::CoreSrv;
using tinyros::TopicData;
using tinyros::SubscribeRequest;


namespace tinyros
{

SubscriberProxy::SubscriberProxy(
    std::shared_ptr<StubWrapper> core_stub,
    std::string node_name,
    std::string topic_name,
    int buffer_size
):
    m_core_stub_(core_stub),
    m_node_name_(node_name),
    m_topic_name_(topic_name),
    m_buffer_size_(buffer_size)
{
}


void SubscriberProxy::StreamReaderThread()
{
    // - Context for the client. It could be used to convey extra information to
	// - the server and/or tweak certain RPC behaviors.
	ClientContext context;
    // - Data we are sending to the server.
	SubscribeRequest request;
    request.set_subscriber_name(m_node_name_);
    request.set_topic_name(m_topic_name_);

    // - Reader for the data we expect from the server.
    std::unique_ptr<ClientReader<TopicData> > reader(
        m_core_stub_->GetStub()->SubscribeTopic(&context, request));
    LOG_INFO("subscribeTopic " <<m_topic_name_ << " success");

    TopicData topicData;
    while (m_thread_continue_ && reader->Read(&topicData))
    {
        // - parse read out data
        const std::string& topicName = topicData.topic_name();
        uint64_t timestamp = topicData.timestamp();
        const std::string& buffer = topicData.buffer();
        // - push into queue
        std::vector<uint8_t> serializeData(buffer.begin(), buffer.end());
        m_message_queue_.push(std::move(MessageType(serializeData, timestamp)));

        // - can not exceed the maximum
        if (m_message_queue_.size() > m_buffer_size_)
            m_message_queue_.pop();
        // - Wake up the callback who are waiting for this topic
        m_input_condition_.notify_all();
    }
    LOG_INFO("StreamReaderThread exit");
}

}// namespace tinyros

