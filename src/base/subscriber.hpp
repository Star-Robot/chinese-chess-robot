///////////////////////////////////////////////////////////////////////
///////
/////// \file     subscriber.hpp
/////// \author   Chegf
/////// \version  1.0.0
/////// \date     2021-6-30 12:17
/////// \brief
//
///// Description:
/////
/////
/////////////////////////////////////////////////////////////////////////

#ifndef subscriber_hpp__
#define subscriber_hpp__

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace huleibao
{

class StubWrapper;

template <typename MessageType>
class Subscriber
{
public:
    typedef std::shared_ptr<Subscriber> Ptr;
    typedef std::function<void(typename MessageType::ConstPtr)> CallBackFunction;

    /// Construct a topic subscriber
    /// \param core_stub        : grpc core's stub
    /// \param node_name        : node name who send msg
    /// \param topic_name       : the topic name
    /// \param serialize_func   : topic's message serialized function
    /// \param buffer_size      : Maximum number of messages retained
    /// \param callback_func    : Subscriber's callback function
    Subscriber(
        std::shared_ptr<StubWrapper> core_stub,
        std::string node_name,
        std::string topic_name,
        int buffer_size,
        CallBackFunction callback_func
    );

    ~Subscriber();

    /// The thread that reads the topic stream executed
    void StreamReaderThread();
    /// The thread that execute Subscriber's callback function
    void TopicCallBackThread();
private:
    /// Topic handle registered in core
    std::shared_ptr<StubWrapper> m_core_stub_;
    /// The topic name
    std::string m_topic_name_;
    /// The node name
    std::string m_node_name_;
    /// The buffer Maximum
    int m_buffer_size_;
    /// Store a particular type of message queue
    std::queue<typename MessageType::ConstPtr> m_message_queue_;
    /// Sync lock
    std::mutex m_msg_mtx_;
    /// Subscriber's callback function
    CallBackFunction m_callback_func_;
    /// The threads continue flag
    std::atomic_bool m_thread_continue_;
    /// when read out  a message
    std::condition_variable m_input_condition_;
    /// stream reading thread
    std::thread m_stream_reader_thread_;
    /// callback function thread
    std::thread m_callback_thread_;
};

} // namespace huleibao
#endif//subscriber_hpp__
