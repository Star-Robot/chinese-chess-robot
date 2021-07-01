///////////////////////////////////////////////////////////////////////
///////
/////// \file     node.hpp
/////// \author   Chegf
/////// \version  1.0.0
/////// \date     2021-6-29 12:05
/////// \brief
//
///// Description:
/////
/////
/////////////////////////////////////////////////////////////////////////

#ifndef node_hpp__
#define node_hpp__


#include <string>
#include <memory>

namespace huleibao
{

template <typename MessageType>
class Publisher;

template <typename MessageType>
class Subscriber;

class StubWrapper;

class Node
{
public:
    typedef std::shared_ptr<Node> Ptr;

    /// Node constructor
    /// \param node_name    : node name
    Node(std::string node_name);

    /// Node deconstructor
    ~Node();

    /// Factory function to create a message publisher
    /// \param topic_name    : topic name
    /// \param buffer_size   : Maximum number of messages retained
    template <typename MessageType>
    typename Publisher<MessageType>::Ptr Advertise(std::string topic_name, int buffer_size)
    {
        return typename Publisher<MessageType>::Ptr(new Publisher<MessageType>(
            m_core_stub_, m_node_name_, topic_name, buffer_size));
    }

    /// Factory function to create a message subscriber
    /// \param topic_name    : topic name
    /// \param buffer_size   : Maximum number of messages retained
    /// \param callback_func : Subscriber's callback function
    template <typename MessageType>
    typename Subscriber<MessageType>::Ptr Subscribe(
        std::string topic_name,
        int buffer_size,
        void(*callback_func)(typename MessageType::ConstPtr))
    {
        return typename Subscriber<MessageType>::Ptr(new Subscriber<MessageType>(
            m_core_stub_, m_node_name_, topic_name, buffer_size, callback_func));
    }

    /// Factory function to create a message subscriber
    /// \param topic_name    : topic name
    /// \param buffer_size   : Maximum number of messages retained
    /// \param callback_func : Member function
    /// \param obj           : Instance of the class
    template <typename MessageType, typename T>
    typename Subscriber<MessageType>::Ptr Subscribe(
        std::string topic_name,
        int buffer_size,
        void (T::* callback_func)(typename MessageType::ConstPtr),
        T* obj)
    {
        auto cbfunc = std::bind(callback_func, obj, std::placeholders::_1);
        return typename Subscriber<MessageType>::Ptr(new Subscriber<MessageType>(
            m_core_stub_, m_node_name_, topic_name, buffer_size, cbfunc));
    }

    /// When node is initialized, it needs to register with core
    void HiCore();
    /// When the node is shut down, notifu the core
    void ByeCore();
private:
    /// The name of the node that sent the message
    std::string m_node_name_;
    /// Stub used to communicate with core
    std::shared_ptr<StubWrapper> m_core_stub_;
};

} // namespace huleibao
#endif//node_hpp__
