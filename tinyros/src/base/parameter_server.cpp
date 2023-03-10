#include <grpcpp/grpcpp.h>
#include "tinyros.grpc.pb.h"
#include "util/util.hpp"
#include "base/stub.hpp"
#include "base/parameter_server.hpp"

using grpc::Status;
using grpc::ClientContext;
using tinyros::ReplyStatusCode;
using tinyros::CommonReply;
using tinyros::Parameter;


namespace tinyros
{
ParameterServer::ParameterServer(
    std::shared_ptr<StubWrapper> core_stub
    ):
    m_core_stub_(core_stub)
{
}


void ParameterServer::SetParam(
    const std::string& param_name, 
    std::string& param_val)
{
    // - Copy the message and start pushing
    Parameter paramData;
    paramData.set_param_name(param_name);
    paramData.set_param_val(param_val);
    // - Context for the client. It could be used to convey extra information to
    // - the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // - Container for the data we expect from the server.
    CommonReply reply;
    // - Send to Core
    Status status = m_core_stub_->GetStub()->SetParam(&context, paramData, &reply);
    // - Act upon its status.
    if (!status.ok())
        LOG_FATAL("SetParam " <<status.error_code() << ": " << status.error_message());

    if(reply.code() != ReplyStatusCode::success)
        LOG_FATAL(reply.info());

    LOG_INFO(reply.info() << ", param_name= " << param_name);
}


std::string ParameterServer::GetParam(
    const std::string& param_name, const std::string& default_val)
{
    // - Copy the message and start pushing
    Parameter paramData;
    paramData.set_param_name(param_name);
    // - Context for the client. It could be used to convey extra information to
    // - the server and/or tweak certain RPC behaviors.
    ClientContext context;
    // - Container for the data we expect from the server.
    CommonReply reply;
    // - Send to Core
    Status status = m_core_stub_->GetStub()->GetParam(&context, paramData, &reply);
    // - Act upon its status.
    if (!status.ok())
        LOG_FATAL("GetParam " <<status.error_code() << ": " << status.error_message());

    if(reply.code() != ReplyStatusCode::success)
        return default_val;

    return reply.info();
}

}// namespace tinyros

