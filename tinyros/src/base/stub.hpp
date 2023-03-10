///////////////////////////////////////////////////////////////////////
///////
/////// \file     stub.hpp
/////// \author   Chegf
/////// \version  1.0.0
/////// \date     2021-7-01 15:34
/////// \brief
//
///// Description:
/////
/////
/////////////////////////////////////////////////////////////////////////

#ifndef stub_hpp__
#define stub_hpp__


#include <string>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "tinyros.grpc.pb.h"

using grpc::Channel;
using tinyros::CoreSrv;

namespace tinyros
{
class StubWrapper
{
public:
    /// Construct a StubWrapper 
    StubWrapper()
    {
        // - Try to connect to the core and get the stub
        // std::shared_ptr<Channel> channel(
        //     grpc::CreateChannel("127.0.0.1:50051",
        //     grpc::InsecureChannelCredentials())
        // );
        grpc::ChannelArguments ch_args;
        ch_args.SetMaxReceiveMessageSize(-1);
        std::shared_ptr<grpc::Channel> channel = 
        	grpc::CreateCustomChannel("localhost:50051", grpc::InsecureChannelCredentials(), ch_args);
        m_core_stub_ = std::move(CoreSrv::NewStub(channel));
    }


    /// Get the real stub 
    std::shared_ptr<CoreSrv::Stub> GetStub() 
    { return m_core_stub_; }
 
private:
    std::shared_ptr<CoreSrv::Stub> m_core_stub_;
};

}// namespace tinyros
#endif//stub_hpp__
