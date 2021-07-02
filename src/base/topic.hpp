///////////////////////////////////////////////////////////////////////
///////
/////// \file     topic.hpp
/////// \author   Chegf
/////// \version  1.0.0
/////// \date     2021-6-29 11:18
/////// \brief
//
///// Description:
/////
/////
/////////////////////////////////////////////////////////////////////////

#ifndef topic_hpp__
#define topic_hpp__

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>

namespace huleibao
{

class Topic
{
public:
    typedef std::shared_ptr<Topic> Ptr;
    typedef std::pair<std::vector<uint8_t>, uint64_t> MessageType;

    /// Construct a topic container
    /// \param publisher    : node name who publish this topic
    /// \param topic_name   : the topic name
    /// \param buffer_size  : Maximum number of messages retained
    Topic(const std::string& publisher, const std::string& topic_name, int buffer_size);

    /// request notification of topics
    bool AddSubscriber(const std::string& subscriber);

    /// cancel notification of topics
    bool RemoveSubscriber(const std::string& subscriber);

    /// Push messages are stored in the queue
    void PushMessage(MessageType&& msg);

    /// if queue is not empty
    bool HasNewMessage();

    /// Get the data at the end of the queue
    MessageType GetLastestMessage();

private:
    /// Store a particular type of message queue
    std::queue<MessageType> m_message_queue_;
    /// Maximum number of messages retained
    int m_msg_maximum_;
    /// The name of the node that send the message
    std::string m_publisher_;
    /// The name of the node that resv the message
    std::set<std::string> m_subscribers_;
    /// The topic name
    std::string m_topic_name_;
public:
    /// Sync lock
    std::mutex m_msg_mtx_;
    /// when there is a message input
    std::condition_variable m_input_condition_;
};
} // namespace huleibao
#endif//topic_hpp__
