///////////////////////////////////////////////////////////////////////
///////
/////// \file     std_msgs.hpp
/////// \author   Chegf
/////// \version  1.0.0
/////// \date     2021-6-29 18:00
/////// \brief
//
///// Description:
/////
/////
/////////////////////////////////////////////////////////////////////////

#ifndef std_msgs_hpp__
#define std_msgs_hpp__

#include <string> 
#include <vector> 
#include <memory> 
#include <cstring> 

#include "base/publisher.hpp" 
#include "base/subscriber.hpp" 


#define REGISTER_MESSAGE(Name, DType)   	    		        \
    namespace std_msgs                                          \
    {                                                           \
    struct Name: public BaseMsgs                                \
    {                                                           \
    public:                                                     \
        typedef std::shared_ptr<Name> Ptr;          			\
        typedef const std::shared_ptr<Name> ConstPtr;			\
        virtual std::vector<uint8_t> Serialize();               \
        virtual void Deserialize(std::vector<uint8_t>&);        \
        DType data;                                             \
    };                                                          \
    }                                                           \

    // template class Publisher<std_msgs::Name>;                   \
    // template class Subscriber<std_msgs::Name>; 


namespace tinyros
{
namespace std_msgs
{

/// The parent class of the message type
struct BaseMsgs
{
public:
    /// Only messages that can be serialized can communicate with core!
    virtual std::vector<uint8_t> Serialize() = 0;
    virtual void Deserialize(std::vector<uint8_t>&) = 0;
    uint64_t timestamp;
};
} // namespace std_msgs


////////////////////////////////////////////////////////////////////////////////
/// define std_msgs
REGISTER_MESSAGE(Int8, int8_t);
REGISTER_MESSAGE(Int16, int16_t);
REGISTER_MESSAGE(Int16MultiArray, std::vector<int16_t>);
REGISTER_MESSAGE(Int32, int32_t);
REGISTER_MESSAGE(Int32MultiArray, std::vector<int32_t>);
REGISTER_MESSAGE(Int64, int64_t);
REGISTER_MESSAGE(Int64MultiArray, std::vector<int64_t>);
REGISTER_MESSAGE(Float32, float);
REGISTER_MESSAGE(Float32MultiArray, std::vector<float>);
REGISTER_MESSAGE(Float64, double);
REGISTER_MESSAGE(Float64MultiArray, std::vector<double>);
REGISTER_MESSAGE(Char, int8_t);
REGISTER_MESSAGE(String, std::string);

/// define custom msgs
struct ImageData
{
    enum Format
    {
        BGR  = 0,
        RGB  = 1,
        GRAY = 2,
    };
    int image_height;
    int image_width;
    Format image_format;
    std::vector<uint8_t> image_data;
};
REGISTER_MESSAGE(Image, ImageData);

} // namespace tinyros

#endif//std_msgs_hpp__
