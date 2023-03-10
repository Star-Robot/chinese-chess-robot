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
#include "base/subscriber_proxy.hpp"
#include "util/util.hpp"

namespace tinyros
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
    /// \param buffer_size      : Maximum number of messages retained
    /// \param callback_func    : Subscriber's callback function
    Subscriber(
        std::shared_ptr<StubWrapper> core_stub,
        std::string node_name,
        std::string topic_name,
        int buffer_size,
        CallBackFunction callback_func
    ):  m_callback_func_(callback_func)
    {
        LOG_INFO("subscribeTopic " << topic_name << " ...");
        m_proxy_.reset(new SubscriberProxy(
            core_stub,
            node_name,
            topic_name,
            buffer_size
        ));

        m_proxy_->m_thread_continue_ = true;
        // - Subscribe to each Node topic and start a stream reading thread
        m_stream_reader_thread_ = std::thread(&SubscriberProxy::StreamReaderThread, m_proxy_.get());
        // m_stream_reader_thread_.detach();
        // - In order to avoid the read flow blocking,
        // - a separate thread for callback processing is started
        m_callback_thread_ = std::thread(&Subscriber::TopicCallBackThread, this);
    }

    /// Deconstructor
    ~Subscriber()
    {
        // - notify all and wait thread exit
        m_proxy_->m_thread_continue_ = false;
        m_proxy_->m_input_condition_.notify_all();
        m_callback_thread_.join();
        m_stream_reader_thread_.join();
    }


    /// The thread that execute Subscriber's callback function
    void TopicCallBackThread()
    {
        while (m_proxy_->m_thread_continue_)
        {
            std::unique_lock<std::mutex> lock(m_proxy_->m_msg_mtx_);
            m_proxy_->m_input_condition_.wait(lock,
                [this] { return !m_proxy_->m_thread_continue_ || !m_proxy_->m_message_queue_.empty(); });
            // - Jump out of the loop if the thread exits
            if(!m_proxy_->m_thread_continue_) break;

            SubscriberProxy::MessageType msgs = std::move(m_proxy_->m_message_queue_.front());
            m_proxy_->m_message_queue_.pop();
            lock.unlock();

            // - Deserialize buffer to MessageType
            typename MessageType::Ptr msg(new MessageType());
            msg->Deserialize(msgs.first);
            msg->timestamp = msgs.second;
            // - execuate subscriber's callback function
            m_callback_func_(msg);
        }
        LOG_INFO("TopicCallBackThread exit");
    }


private:
    /// Communication proxy with core 
    std::shared_ptr<SubscriberProxy> m_proxy_;
    /// Subscriber's callback function
    CallBackFunction m_callback_func_;
    /// stream reading thread
    std::thread m_stream_reader_thread_;
    /// callback function thread
    std::thread m_callback_thread_;
};

} // namespace tinyros
#endif//subscriber_hpp__
