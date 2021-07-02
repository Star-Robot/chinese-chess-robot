#include "util/util.hpp"
#include "base/core.hpp"
#include "base/topic.hpp"

namespace huleibao
{


void Core::Start()
{
    std::string serverAddress("0.0.0.0:50051");
	// Listen on the given address without any authentication mechanism.
	m_builder_.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
	// Register "service" as the instance through which we'll communicate with
	// clients. In this case it corresponds to an *synchronous* service.
    m_core_srv_.reset(new CoreServiceImpl());
	m_builder_.RegisterService(m_core_srv_.get());
	// Finally assemble the server.
	m_server_ = m_builder_.BuildAndStart();
    LOG_WARN("[core] Start"); 
	// Wait for the server to shutdown. Note that some other thread must be
	// responsible for shutting down the server for this call to ever return.
	m_server_->Wait();
}


/// The new node starts and communicates with the core
Status CoreServiceImpl::NodeGreet(
    ServerContext* context, const GreetData* request, CommonReply* reply)
{
    // - parse input
    const std::string& nodeName = request->node_name();

    LOG_VERBOSE("[core] NodeGreet from " << nodeName);
    // - Register Node : Node name cannot be repeated
    std::lock_guard<std::mutex> lock(m_topic_mtx_);
    if (0 == m_publisher_mapper_.count(nodeName))
    {
        reply->set_info("node register success.");
        reply->set_code(ReplyStatusCode::success);
        m_publisher_mapper_[nodeName];
    }
    else
    {
        reply->set_info("This node name already exists.");
        reply->set_code(ReplyStatusCode::failed);
    }
    return Status::OK;
}


/// The node will disconnect with the core
Status CoreServiceImpl::NodeDisconnect(
    ServerContext* context, const GreetData* request, CommonReply* reply)
{
    // - parse input
    const std::string& nodeName = request->node_name();

    LOG_VERBOSE("[core] NodeDisconnect from " << nodeName);
    // - delete Node : Node name must be registered
    std::lock_guard<std::mutex> lock(m_topic_mtx_);
    if (m_publisher_mapper_.count(nodeName))
    {
        reply->set_info("node disconnect success.");
        reply->set_code(ReplyStatusCode::success);
        // - delete all topic from this node
        for(auto& topicName : m_publisher_mapper_[nodeName])
        {
            // - Wake up subscribers who are waiting for the topic, to quit 
            m_topic_mapper_[topicName]->m_input_condition_.notify_all();
            m_topic_mapper_.erase(topicName);
        }
        // - Cancel all subscriptions of this node
        for(auto& topic : m_topic_mapper_)
            topic.second->RemoveSubscriber(nodeName);

        m_publisher_mapper_.erase(nodeName);
    }
    else
    {
        reply->set_info("This node name does not exists.");
        reply->set_code(ReplyStatusCode::failed);
    }
    return Status::OK;
}


/// Publisher requests to register topics
Status CoreServiceImpl::AdvertiseTopic(
    ServerContext* context, const AdvertiseRequest* request, CommonReply* reply)
{
    // - parse input
    const std::string& publisherName = request->publisher_name();
	const std::string& topicName = request->topic_name();
	int buffSize = request->buffer_size();

    LOG_VERBOSE("[core] AdvertiseTopic from " << publisherName << ", topicName= " 
        << topicName << ", buffer_size= " << buffSize);
	// - Register topic : Topic name cannot be repeated
    std::lock_guard<std::mutex> lock(m_topic_mtx_);
    if (0 == m_topic_mapper_.count(topicName))
    {
        Topic::Ptr topicPtr(new Topic(publisherName, topicName, buffSize));
        m_topic_mapper_[topicName] = topicPtr;
        m_publisher_mapper_[publisherName].push_back(topicName);
        reply->set_info("topic register success.");
        reply->set_code(ReplyStatusCode::success);
    }
    else
    {
        reply->set_info("This topic name already exists.");
        reply->set_code(ReplyStatusCode::failed);
    }
	return Status::OK;
}


/// Subscribers request notification of topics
Status CoreServiceImpl::SubscribeTopic(
    ServerContext* context, const SubscribeRequest* request, ServerWriter<TopicData>* writer)
{
    // - parse input
    const std::string& subscriberName = request->subscriber_name();
	const std::string& topicName = request->topic_name();

    
    // - If this topic does not exist, return
    if (0 == m_topic_mapper_.count(topicName))
    {
        LOG_VERBOSE("[core] SubscribeTopic from " << subscriberName << ", topicName= " 
            << topicName << ", but topic does not exist.");
        return Status::OK;
    }

    // - Once awakened, push the data to node
    Topic::Ptr topic = m_topic_mapper_[topicName];
    if (false == topic->AddSubscriber(subscriberName))
    {
        LOG_VERBOSE("[core] SubscribeTopic from " << subscriberName << ", topicName= " 
            << topicName << ", but duplicated so refused.");
        return Status::OK;
    }

    LOG_VERBOSE("[core] SubscribeTopic from " << subscriberName << ", topicName= " 
        << topicName);
    
    LOG_INFO("[core] Establish a connection with " << subscriberName << ", topicName= " 
        << topicName);
    // - Wait for topic notification until the connection is disconnected
    while(!context->IsCancelled())
    {
        std::unique_lock<std::mutex> lock(topic->m_msg_mtx_);
        topic->m_input_condition_.wait(lock,
			[context, topic] {
                return context->IsCancelled() || topic->HasNewMessage();
            });
        // - Jump out of the loop if disconnected
        if (context->IsCancelled()) break;
        Topic::MessageType msgs = std::move(topic->GetLastestMessage());
		lock.unlock();
        // - Copy the message and start pushing
        TopicData topicData;
        topicData.set_topic_name(topicName);
        topicData.set_buffer(msgs.first.data(), msgs.first.size());
        topicData.set_timestamp(msgs.second);
        writer->Write(topicData);
    }
    // - 断开连接
    LOG_WARN("[core] connection is disconnected from " << subscriberName);
	return Status::OK;
}


/// publish request of topics
Status CoreServiceImpl::PublishTopic(
    ServerContext* context, const TopicData* request, CommonReply* reply)
{
    // - parse input
	const std::string& topicName = request->topic_name();
	uint64_t timestamp = request->timestamp();
    const std::string& buffer = request->buffer();

    LOG_VERBOSE("[core] PublishTopic from topicName= " << topicName);
    // - push the data to topic's queue
    Topic::Ptr topic = m_topic_mapper_[topicName];
    std::vector<uint8_t> serializeData(buffer.begin(), buffer.end());
    topic->PushMessage(std::move(Topic::MessageType(serializeData, timestamp)));
    // - set reply
    reply->set_info("topic publish success.");
    reply->set_code(ReplyStatusCode::success);
	return Status::OK;
}

} // namespace huleibao
