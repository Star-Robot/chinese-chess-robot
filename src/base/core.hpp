///////////////////////////////////////////////////////////////////////
///////
/////// \file     core.hpp
/////// \author   Chegf
/////// \version  1.0.0
/////// \date     2021-6-29 12:05
/////// \brief
//
///// Description:
///// Listen to Node's request:
///// 1. Start a new node
///// 2. Topic posting request
///// 3. Topic subscription request
/////////////////////////////////////////////////////////////////////////


#ifndef core_hpp__
#define core_hpp__

#include <grpcpp/grpcpp.h>
#include "tinyros.grpc.pb.h"
#include <memory>
#include <mutex>
#include <map>
#include <set>
#include <string>
#include <vector>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ServerWriter;

using huleibao::GreetData;
using huleibao::CommonReply;
using huleibao::AdvertiseRequest;
using huleibao::SubscribeRequest;
using huleibao::TopicData;
using huleibao::CoreSrv;
using huleibao::ReplyStatusCode;


namespace huleibao
{
class Topic;


// Logic and data behind the server's behavior.
class CoreServiceImpl final : public CoreSrv::Service
{
public:
    typedef std::shared_ptr<CoreServiceImpl> Ptr;

private:
	/// Listen to Node's request and reply
	Status NodeGreet(
		ServerContext* context, const GreetData* request, CommonReply* reply) override;
    /// The node will disconnect with the core
    Status NodeDisconnect(
        ServerContext* context, const GreetData* request, CommonReply* reply) override;
    /// Register a message queue with a size of buffer_size,
    /// which is used to store the topic topic_name of the node publisher
	Status AdvertiseTopic(
		ServerContext* context, const AdvertiseRequest* request, CommonReply* reply) override;
    /// Subscribers request notification of topics
	Status SubscribeTopic(
		ServerContext* context, const SubscribeRequest* request, ServerWriter<TopicData>* writer) override;
    /// publish request of topics
	Status PublishTopic(
		ServerContext* context, const TopicData* request, CommonReply* reply) override;
    /// set global parameter 
	Status SetParam(
		ServerContext* context, const Parameter* request, CommonReply* reply) override;
    /// get global parameter 
	Status GetParam(
		ServerContext* context, const Parameter* request, CommonReply* reply) override;

private:
	/// Topic name, indexed data container
    std::map<std::string, std::shared_ptr<huleibao::Topic>> m_topic_mapper_;
    /// node: [topic1, topic2, ...]
    std::map<std::string, std::set<std::string>> m_node_mapper_;
    std::map<std::string, std::atomic_bool> m_kill_subscribe_stream_of_;
    /// parameter container
    std::map<std::string, std::string> m_param_mapper_;
    /// Sync lock
    std::mutex m_topic_mtx_;
};


// Core service, used for message storage and distribution
class Core
{
public:
    typedef std::shared_ptr<Core> Ptr;
    /// Start core
    void Start();

private:
    /// grpc server
    CoreServiceImpl::Ptr m_core_srv_;
    ServerBuilder m_builder_;
    std::unique_ptr<Server> m_server_;
};

} // namespace huleibao

#endif//core_hpp__
