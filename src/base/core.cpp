#include "util/util.hpp"
#include "base/core.hpp"
#include "base/topic.hpp"

namespace huleibao
{


void Core::Start()
{
    std::string serverAddress("0.0.0.0:50051");
    // set max message size= 32MB
    const int maxMsgSize = 32*1024*1024;
    m_builder_.SetMaxReceiveMessageSize(maxMsgSize);
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

    LOG_VERBOSE("[core] Node " << nodeName << " registered.");
    // - Register Node : Node name cannot be repeated
    m_node_mapper_[nodeName].clear();
    m_kill_subscribe_stream_of_[nodeName] = false;
    // - set reply
    std::string info = std::string("Node ") + nodeName + " registered.";
    reply->set_info(info);
    reply->set_code(ReplyStatusCode::success);
    return Status::OK;
}


/// The node will disconnect with the core
Status CoreServiceImpl::NodeDisconnect(
    ServerContext* context, const GreetData* request, CommonReply* reply)
{
    // - parse input
    const std::string& nodeName = request->node_name();

    // - delete Node : Node name must be registered
    std::lock_guard<std::mutex> lock(m_topic_mtx_);
    if (m_node_mapper_.count(nodeName))
    {
        m_kill_subscribe_stream_of_[nodeName] = true;
        // - Cancel all subscriptions of this node
        for(auto& topicName : m_node_mapper_[nodeName])
        {
            // - Wake up subscribers who are waiting for the topic, to quit 
            m_topic_mapper_[topicName]->m_input_condition_.notify_all();
            m_topic_mapper_[topicName]->RemoveSubscriber(nodeName);
        }

        m_node_mapper_.erase(nodeName);
    	std::string info = std::string("Node ") + nodeName + " disconnected.";
        reply->set_info(info);
        reply->set_code(ReplyStatusCode::success);
    }
    else
    {
    	std::string info = std::string("Node ") + nodeName + " is not found.";
        reply->set_info(info);
        reply->set_code(ReplyStatusCode::failed);
    }

    LOG_VERBOSE("[core] Node " << nodeName << " disconnected.");
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
        Topic::Ptr topicPtr(new Topic(topicName, buffSize));
        m_topic_mapper_[topicName] = topicPtr;
        m_node_mapper_[publisherName].insert(topicName);
	std::string info = std::string("Topic ") + topicName + " registered.";
        reply->set_info(info);
        reply->set_code(ReplyStatusCode::success);
    }
    else
    {
        m_topic_mapper_[topicName]->SetBufferSize(buffSize);
	std::string info = std::string("Topic ") + topicName + " already registered.";
        reply->set_info(info);
        reply->set_code(ReplyStatusCode::success);
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
    // - If this topic does not exist, then Register it
    if (0 == m_topic_mapper_.count(topicName))
    {
        Topic::Ptr topicPtr(new Topic(topicName));
        m_topic_mapper_[topicName] = topicPtr;
    }

    // - Topics subscribed by the same node cannot be repeated 
    if (0 == m_node_mapper_[subscriberName].count(topicName))
        m_node_mapper_[subscriberName].insert(topicName);
 
    // - Try to add AddSubscriber to the topic 
    Topic::Ptr topic = m_topic_mapper_[topicName];
    if (false == topic->AddSubscriber(subscriberName))
    {
        LOG_WARN("[core] SubscribeTopic from " << subscriberName << ", topicName= " 
            << topicName << ", but duplicated node name.");
    }

    LOG_VERBOSE("[core] SubscribeTopic from " << subscriberName << ", topicName= " 
        << topicName);
    
    LOG_INFO("[core] Establish a connection with " << subscriberName << ", topicName= " 
        << topicName);
    // - Wait for topic notification until the connection is disconnected
    while(!m_kill_subscribe_stream_of_[subscriberName] || !context->IsCancelled())
    {
        std::unique_lock<std::mutex> lock(topic->m_msg_mtx_);
        topic->m_input_condition_.wait(lock,
            [this, context, topic, subscriberName] {
                return m_kill_subscribe_stream_of_[subscriberName] || 
                    context->IsCancelled() || topic->HasNewMessage(subscriberName);
            });
        // - Jump out of the loop if disconnected
        if (m_kill_subscribe_stream_of_[subscriberName] || context->IsCancelled()) break;
        Topic::MessageType msgs = std::move(topic->GetLastestMessage(subscriberName));
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

    // - push the data to topic's queue
    Topic::Ptr topic = m_topic_mapper_[topicName];
    std::vector<uint8_t> serializeData(buffer.begin(), buffer.end());
    topic->PushMessage(std::move(Topic::MessageType(serializeData, timestamp)));
    // - set reply
    std::string info = std::string("Topic ") + topicName + " publish";
    reply->set_info(info);
    reply->set_code(ReplyStatusCode::success);
    return Status::OK;
}



/// publish request of topics
Status CoreServiceImpl::SetParam(
    ServerContext* context, const Parameter* request, CommonReply* reply)
{
    // - parse input
    const std::string& paramName = request->param_name();
    const std::string& paramVal= request->param_val();

    m_param_mapper_[paramName] = paramVal;
    // LOG_INFO("SetParam " << paramName << ": " << paramVal);
    // - set reply
    reply->set_info("parameter set success.");
    reply->set_code(ReplyStatusCode::success);
    return Status::OK;
}


/// publish request of topics
Status CoreServiceImpl::GetParam(
    ServerContext* context, const Parameter* request, CommonReply* reply)
{
    // - parse input
    const std::string& paramName = request->param_name();

    // - set param if exist
    if (m_param_mapper_.count(paramName))
    {
        reply->set_info(m_param_mapper_[paramName]);
        reply->set_code(ReplyStatusCode::success);
        // LOG_INFO("GetParam " << paramName << ": " << m_param_mapper_[paramName]);
    }
    else
    {
        reply->set_info("parameter get failed.");
        reply->set_code(ReplyStatusCode::failed);
        // LOG_INFO("GetParam failed");

    }
    return Status::OK;
}

} // namespace huleibao
