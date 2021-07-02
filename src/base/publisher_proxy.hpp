///////////////////////////////////////////////////////////////////////
///////
/////// \file     publisher_proxy.hpp
/////// \author   Chegf
/////// \version  1.0.0
/////// \date     2021-7-01 12:17
/////// \brief
//
///// Description:
/////
/////
/////////////////////////////////////////////////////////////////////////

#ifndef publisher_proxy_hpp__
#define publisher_proxy_hpp__

#include <memory>
#include <string>
#include <vector>


namespace huleibao
{

class StubWrapper;

class PublisherProxy
{
public:
    /// Constructor
    /// \param core_stub        : grpc core's stub
    PublisherProxy(
        std::shared_ptr<StubWrapper> core_stub
    );

    /// Advertise a Topic from core
    /// \param node_name        : node name who send msg
    /// \param topic_name       : the topic name
    /// \param buffer_size      : Maximum number of messages retained
    void AdvertiseTopic(
            std::string node_name,
            std::string topic_name,
            int buffer_size
    );

    /// Push messages to core
    void Publish(std::string& topic_name, std::vector<uint8_t>& serialize_data);

private:
    /// Topic handle registered in core
    std::shared_ptr<StubWrapper> m_core_stub_;
};

}// namespace huleibao
#endif//publisher_proxy_hpp__
