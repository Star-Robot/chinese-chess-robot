#include "util/util.hpp"
#include "base/topic.hpp"

namespace tinyros
{


Topic::Topic(const std::string& topic_name, int buffer_size)
{
    m_topic_name_  = topic_name;
    m_msg_maximum_ = buffer_size;
}


void Topic::SetBufferSize(int buffer_size)
{
    m_msg_maximum_ = buffer_size;
}


bool Topic::AddSubscriber(const std::string& subscriber)
{
    // - The same subscriber cannot subscribe twice
    if (m_subscribers_.count(subscriber)) return false;
    m_subscribers_[subscriber] = GetTimeStamp();
    return true;
}


bool Topic::RemoveSubscriber(const std::string& subscriber)
{
    // - The same subscriber cannot subscribe twice
    if (0 == m_subscribers_.count(subscriber)) return false;
    m_subscribers_.erase(subscriber);
    return true;
}


void Topic::PushMessage(MessageType&& msg)
{
    std::lock_guard<std::mutex> lock(m_msg_mtx_);
    m_message_queue_.emplace_back(msg);
    // - can not exceed the maximum
    if(m_message_queue_.size()>m_msg_maximum_)
        m_message_queue_.pop_front();
    // - Wake up the pushers who are waiting for this topic
    m_input_condition_.notify_all();
}


bool Topic::HasNewMessage(const std::string& subscriber)
{
    // std::lock_guard<std::mutex> lock(m_msg_mtx_);
    bool hasNew = false;
    // - Check whether there are new messages that have not been taken away 
    for(auto& msg : m_message_queue_)
    {
        if (msg.second > m_subscribers_[subscriber])
        {
            hasNew = true;
            break;
        }
    }
    return hasNew;
}


Topic::MessageType Topic::GetLastestMessage(const std::string& subscriber)
{
    MessageType retMsg;
    // - find the new messages
    for(auto& msg : m_message_queue_)
    {
        if (msg.second > m_subscribers_[subscriber])
        {
            retMsg = msg;
            break;
        }
    }
    // - Update the lastest timestamp
    m_subscribers_[subscriber] = retMsg.second;
    return retMsg;
}

} // namespace tinyros
