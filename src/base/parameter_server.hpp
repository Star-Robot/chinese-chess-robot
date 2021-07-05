///////////////////////////////////////////////////////////////////////
///////
/////// \file     parameter_server.hpp
/////// \author   Chegf
/////// \version  1.0.0
/////// \date     2021-7-05 18:17
/////// \brief
//
///// Description:
/////
/////
/////////////////////////////////////////////////////////////////////////

#ifndef parameter_server_hpp__
#define parameter_server_hpp__

#include <memory>
#include <string>
#include <vector>


namespace huleibao
{

class StubWrapper;

class ParameterServer
{
public:
    /// Constructor
    /// \param core_stub        : grpc core's stub
    ParameterServer(
        std::shared_ptr<StubWrapper> core_stub
    );

    /// Set param and Push to core
    /// \param param_name  : param name
    /// \param param_val   : param val 
    void SetParam(const std::string& param_name, std::string& param_val);

    /// Get param from core
    /// \param param_name   : param name
    /// \param default_val  : default_val 
    std::string GetParam(const std::string& param_name, const std::string& default_val="");

private:
    /// Topic handle registered in core
    std::shared_ptr<StubWrapper> m_core_stub_;
};

}// namespace huleibao
#endif//parameter_server_hpp__

