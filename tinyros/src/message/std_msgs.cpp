#include "message/std_msgs.hpp"

namespace tinyros
{
namespace std_msgs
{

//////////////// Int8:: Serialize() Deserialize() //////////////////////////////
std::vector<uint8_t> Int8::Serialize()
{
    uint8_t* ptr = (uint8_t*)&data;
    std::vector<uint8_t> msgData(ptr, ptr+1);
    return msgData;
}

void Int8::Deserialize(std::vector<uint8_t>& buffer)
{
    data = *(int8_t*)(buffer.data());
}

//////////////// Int16:: Serialize() Deserialize() /////////////////////////////
std::vector<uint8_t> Int16::Serialize()
{
    uint8_t* ptr = (uint8_t*)&data;
    std::vector<uint8_t> msgData(ptr, ptr+2);
    return msgData;
}

void Int16::Deserialize(std::vector<uint8_t>& buffer)
{
    data = *(int16_t*)(buffer.data());
}

//////////////// Int16MultiArray:: Serialize() Deserialize() ///////////////////
std::vector<uint8_t> Int16MultiArray::Serialize()
{
    std::vector<uint8_t> msgData(data.size() * 2);
    memcpy(msgData.data(), data.data(), data.size() * 2);
    return msgData;
}

void Int16MultiArray::Deserialize(std::vector<uint8_t>& buffer)
{
    data.resize(buffer.size() >> 1);
    memcpy(data.data(), buffer.data(), buffer.size());
}

//////////////// Int16:: Serialize() Deserialize() /////////////////////////////
std::vector<uint8_t> Int32::Serialize()
{
    uint8_t* ptr = (uint8_t*)&data;
    std::vector<uint8_t> msgData(ptr, ptr+4);
    return msgData;
}

void Int32::Deserialize(std::vector<uint8_t>& buffer)
{
    data = *(int32_t*)(buffer.data());
}

////////////// Int32MultiArray:: Serialize() Deserialize() /////////////////////
std::vector<uint8_t> Int32MultiArray::Serialize()
{
    std::vector<uint8_t> msgData(data.size() * 4);
    memcpy(msgData.data(), data.data(), data.size() * 4);
    return msgData;
}

void Int32MultiArray::Deserialize(std::vector<uint8_t>& buffer)
{
    data.resize(buffer.size() >> 2);
    memcpy(data.data(), buffer.data(), buffer.size());
}

//////////////// Int64:: Serialize() Deserialize() /////////////////////////////
std::vector<uint8_t> Int64::Serialize()
{
    uint8_t* ptr = (uint8_t*)&data;
    std::vector<uint8_t> msgData(ptr, ptr+8);
    return msgData;
}

void Int64::Deserialize(std::vector<uint8_t>& buffer)
{
    data = *(int64_t*)(buffer.data());
}

//////////// Int64MultiArray:: Serialize() Deserialize() ///////////////////////
std::vector<uint8_t> Int64MultiArray::Serialize()
{
    std::vector<uint8_t> msgData(data.size() * 8);
    memcpy(msgData.data(), data.data(), data.size() * 8);
    return msgData;
}

void Int64MultiArray::Deserialize(std::vector<uint8_t>& buffer)
{
    data.resize(buffer.size() >> 4);
    memcpy(data.data(), buffer.data(), buffer.size());
}

//////////////// Float32:: Serialize() Deserialize() ///////////////////////////
std::vector<uint8_t> Float32::Serialize()
{
    uint8_t* ptr = (uint8_t*)&data;
    std::vector<uint8_t> msgData(ptr, ptr+4);
    return msgData;
}

void Float32::Deserialize(std::vector<uint8_t>& buffer)
{
    data = *(float*)(buffer.data());
}

////////////// Float32MultiArray:: Serialize() Deserialize() ///////////////////
std::vector<uint8_t> Float32MultiArray::Serialize()
{
    std::vector<uint8_t> msgData(data.size() * 4);
    memcpy(msgData.data(), data.data(), data.size() * 4);
    return msgData;
}

void Float32MultiArray::Deserialize(std::vector<uint8_t>& buffer)
{
    data.resize(buffer.size() >> 2);
    memcpy(data.data(), buffer.data(), buffer.size());
}

//////////////// Float64:: Serialize() Deserialize() ///////////////////////////
std::vector<uint8_t> Float64::Serialize()
{
    uint8_t* ptr = (uint8_t*)&data;
    std::vector<uint8_t> msgData(ptr, ptr+8);
    return msgData;
}

void Float64::Deserialize(std::vector<uint8_t>& buffer)
{
    data = *(double*)(buffer.data());
}

////////////// Float64MultiArray:: Serialize() Deserialize() ///////////////////
std::vector<uint8_t> Float64MultiArray::Serialize()
{
    std::vector<uint8_t> msgData(data.size() * 8);
    memcpy(msgData.data(), data.data(), data.size() * 8);
    return msgData;
}

void Float64MultiArray::Deserialize(std::vector<uint8_t>& buffer)
{
    data.resize(buffer.size() >> 4);
    memcpy(data.data(), buffer.data(), buffer.size());
}

//////////////// Char:: Serialize() Deserialize() //////////////////////////////
std::vector<uint8_t> Char::Serialize()
{
    uint8_t* ptr = (uint8_t*)&data;
    std::vector<uint8_t> msgData(ptr, ptr+1);
    return msgData;
}

void Char::Deserialize(std::vector<uint8_t>& buffer)
{
    data = *(char*)(buffer.data());
}

//////////////// String:: Serialize() Deserialize() ////////////////////////////
std::vector<uint8_t> String::Serialize()
{
    std::vector<uint8_t> msgData(data.begin(), data.end());
    return msgData;
}

void String::Deserialize(std::vector<uint8_t>& buffer)
{
    data.assign(buffer.begin(), buffer.end());
}

/////////////// Image:: Serialize() Deserialize() //////////////////////////////
std::vector<uint8_t> Image::Serialize()
{
    int h = data.image_height;
    int w = data.image_width;
    int f = data.image_format;
    std::vector<uint8_t>& imgData = data.image_data;
    std::vector<uint8_t> msgData(imgData.size() + 3*4);
    memcpy(msgData.data()+0, &h, 4);
    memcpy(msgData.data()+4, &w, 4);
    memcpy(msgData.data()+8, &f, 4);
    memcpy(msgData.data()+12, imgData.data(), imgData.size());
    return msgData;
}

void Image::Deserialize(std::vector<uint8_t>& buffer)
{
    int len = buffer.size();
    memcpy(&data.image_height, buffer.data()+0, 4);
    memcpy(&data.image_width, buffer.data()+4, 4);
    memcpy(&data.image_format, buffer.data()+8, 4);
    data.image_data.assign(buffer.begin()+12, buffer.end());
}


} // namespace std_msgs
} // namespace tinyros
