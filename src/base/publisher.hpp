///////////////////////////////////////////////////////////////////////
///////
/////// \file     publilsher.hpp
/////// \author   Chegf
/////// \version  1.0.0
/////// \date     2021-6-29 12:17
/////// \brief
//
///// Description:
/////
/////
/////////////////////////////////////////////////////////////////////////

#ifndef publisher_hpp__
#define publisher_hpp__

#include <string>
#include <memory>
#include "base/publisher_proxy.hpp"


namespace huleibao
{

class StubWrapper;

template <typename MessageType>
class Publisher
{
public:
    typedef std::shared_ptr<Publisher> Ptr;

    /// Construct a topic container
    /// \param core_stub        : grpc core's stub
    /// \param node_name        : node name who send msg
    /// \param topic_name       : the topic name
    /// \param buffer_size      : Maximum number of messages retained
    Publisher(
        std::shared_ptr<StubWrapper> core_stub,
        std::string node_name,
        std::string topic_name,
        int buffer_size
    ):  m_node_name_(node_name), 
        m_topic_name_(topic_name), 
        m_buffer_size_(buffer_size),
        m_proxy_(new PublisherProxy(core_stub))
    {
        m_proxy_->AdvertiseTopic(node_name, topic_name, buffer_size);
    }


    /// Push messages with proxy 
    void Publish(MessageType msg)
    {
        // - Serialized message
        std::vector<uint8_t> serializeData = msg.Serialize();
        m_proxy_->Publish(m_topic_name_, serializeData);
    }


private:
    /// Communication proxy with core 
    std::shared_ptr<PublisherProxy> m_proxy_;
    /// The topic name
    std::string m_topic_name_;
    /// The node name
    std::string m_node_name_;
    /// The buffer Maximum
    int m_buffer_size_;
};

} // namespace huleibao
#endif//publisher_hpp__
