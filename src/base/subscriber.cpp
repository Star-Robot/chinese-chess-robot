#include <grpcpp/grpcpp.h>
#include "tinyros.grpc.pb.h"
#include "util/util.hpp"
#include "base/subscriber.hpp"
#include "base/stub.hpp"
#include "message/std_msgs.hpp"


using grpc::ClientContext;
using grpc::ClientReader;
using huleibao::CoreSrv;
using huleibao::TopicData;
using huleibao::SubscribeRequest;

namespace huleibao
{

template <typename MessageType>
Subscriber<MessageType>::Subscriber(
    std::shared_ptr<StubWrapper> core_stub,
    std::string node_name,
    std::string topic_name,
    int buffer_size,
    CallBackFunction callback_func
    ):
    m_node_name_(node_name),
    m_topic_name_(topic_name),
    m_buffer_size_(buffer_size),
    m_core_stub_(core_stub),
    m_callback_func_(callback_func)
{
    m_thread_continue_	= true;
    // - Subscribe to each Node topic and start a stream reading thread
	m_stream_reader_thread_ = std::thread(&Subscriber::StreamReaderThread, this);
    // - In order to avoid the read flow blocking,
    // - a separate thread for callback processing is started
	m_callback_thread_ = std::thread(&Subscriber::TopicCallBackThread, this);
}


template <typename MessageType>
Subscriber<MessageType>::~Subscriber()
{
    // - notify all and wait thread exit
	m_thread_continue_ = false;
	m_input_condition_.notify_all();
	m_stream_reader_thread_.join();
	m_callback_thread_.join();
}


template <typename MessageType>
void Subscriber<MessageType>::StreamReaderThread()
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

    TopicData topicData;
    while (m_thread_continue_ && reader->Read(&topicData))
    {
        std::cout << "Read topic data "
            << topicData.topic_name() << " at "
            << topicData.buffer() << ", "
            << topicData.timestamp() << std::endl;
        // - parse read out data
        const std::string& topicName = topicData.topic_name();
        uint64_t timestamp = topicData.timestamp();
        const std::string& buffer = topicData.buffer();
        // - Deserialize buffer to MessageType
        typename MessageType::Ptr msg(new MessageType());
        std::vector<uint8_t> serializeData(buffer.begin(), buffer.end());
        msg->Deserialize(serializeData, timestamp);
        m_message_queue_.push(msg);
        // - can not exceed the maximum
        if (m_message_queue_.size() > m_buffer_size_)
            m_message_queue_.pop();
        // - Wake up the callback who are waiting for this topic
        m_input_condition_.notify_all();
    }
}


template <typename MessageType>
void Subscriber<MessageType>::TopicCallBackThread()
{
    while (m_thread_continue_)
    {
        std::unique_lock<std::mutex> lock(m_msg_mtx_);
        m_input_condition_.wait(lock,
			[this] { return !m_thread_continue_ || !m_message_queue_.empty(); });
        // - Jump out of the loop if the thread exits
        if(!m_thread_continue_) break;
        MessageType msgs = std::move(m_message_queue_.front());
        m_message_queue_.pop();
		lock.unlock();
        // - execuate subscriber's callback function
        m_callback_func_(msgs);
    }
}

} // namespace huleibao
