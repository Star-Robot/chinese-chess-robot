#include "util/util.hpp"
#include "base/topic.hpp"

namespace huleibao
{


Topic::Topic(
    const std::string& publisher, const std::string& topic_name, int buffer_size)
{
    m_publisher_   = publisher;
    m_topic_name_  = topic_name;
    m_msg_maximum_ = buffer_size;
}


bool Topic::AddSubscriber(const std::string& subscriber)
{
    // - The same subscriber cannot subscribe twice
    if (m_subscribers_.count(subscriber)) return false;
    m_subscribers_.insert(subscriber);
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
    m_message_queue_.push(msg);
    // - can not exceed the maximum
    if(m_message_queue_.size()>m_msg_maximum_)
        m_message_queue_.pop();
    // - Wake up the pushers who are waiting for this topic
    m_input_condition_.notify_all();
}


bool Topic::HasNewMessage()
{
    std::lock_guard<std::mutex> lock(m_msg_mtx_);
    return !m_message_queue_.empty();
}


Topic::MessageType Topic::GetLastestMessage()
{
    MessageType msg;
    std::lock_guard<std::mutex> lock(m_msg_mtx_);
    if(!m_message_queue_.empty()){
        msg = std::move(m_message_queue_.front());
        m_message_queue_.pop();
    }
    return msg;
}

} // namespace huleibao
