///////////////////////////////////////////////////////////////////////
///////
/////// \file     subscriber_proxy.hpp
/////// \author   Chegf
/////// \version  1.0.0
/////// \date     2021-7-01 12:17
/////// \brief
//
///// Description:
/////
/////
/////////////////////////////////////////////////////////////////////////

#ifndef subscriber_proxy_hpp__
#define subscriber_proxy_hpp__

#include <memory>
#include <queue>
#include <string>
#include <vector>


namespace huleibao
{

class StubWrapper;

class SubscriberProxy
{
public:
    typedef std::pair<std::vector<uint8_t>, uint64_t> MessageType;
    /// Construct a SubscriberProxy
    /// \param core_stub        : grpc core's stub
    /// \param node_name        : node name who send msg
    /// \param topic_name       : the topic name
    /// \param buffer_size      : Maximum number of messages retained
    SubscriberProxy(
        std::shared_ptr<StubWrapper> core_stub,
        std::string node_name,
        std::string topic_name,
        int buffer_size
    );

    /// The thread that reads the topic stream executed
    void StreamReaderThread();
private:
    /// Topic handle registered in core
    std::shared_ptr<StubWrapper> m_core_stub_;
    /// The topic name
    std::string m_topic_name_;
    /// The node name
    std::string m_node_name_;
    /// The buffer Maximum
    int m_buffer_size_;

public:
    /// Store a particular type of message queue
    std::queue<MessageType> m_message_queue_;
    /// Sync lock
    std::mutex m_msg_mtx_;
    /// The threads continue flag
    std::atomic_bool m_thread_continue_;
    /// when read out  a message
    std::condition_variable m_input_condition_;

};


}// namespace huleibao
#endif//subscriber_proxy_hpp__

