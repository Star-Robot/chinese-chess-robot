///////////////////////////////////////////////////////////////////////
/////
///// \file     util.hpp
///// \author   Chegf
///// \version  1.0.0
///// \date     2019-1-30 16:45:35
///// \brief    tools

/// Description:
/// Define some publicly used functions or tools.
///////////////////////////////////////////////////////////////////////
#ifndef util_hpp__
#define util_hpp__

#include <cstring>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
//
//the following are UBUNTU/LINUX ONLY terminal color codes.
#define RESET       "\033[0m"
#define BLACK       "\033[30m"             /* Black */
#define RED         "\033[31m"             /* Red */
#define GREEN       "\033[32m"             /* Green */
#define YELLOW      "\033[33m"             /* Yellow */
#define BLUE        "\033[34m"             /* Blue */
#define MAGENTA     "\033[35m"             /* Magenta */
#define CYAN        "\033[36m"             /* Cyan */
#define WHITE       "\033[37m"             /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

#define __Code_location__ std::string("(") + \
    (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)  +  \
    ":" + std::to_string(__LINE__) + ")"
#define LOG_TAG std::setiosflags(std::ios::left) << std::setw(32) \
    << (__Code_location__).c_str() \
    << std::resetiosflags(std::ios::left)

#define LOG_VERBOSE(...) \
    std::cout << LOG_TAG << " " << __VA_ARGS__ << std::endl << RESET;
#define LOG_INFO(...) \
    std::cout << YELLOW << LOG_TAG << " " << __VA_ARGS__ << std::endl << RESET;
#define LOG_WARN(...) \
    std::cout << BLUE <<LOG_TAG << " " << __VA_ARGS__ << std::endl << RESET;
#define LOG_ERROR(...) \
    std::cout << RED << LOG_TAG << " " << __VA_ARGS__ << std::endl << RESET;
#define LOG_FATAL(...) \
    {   \
        LOG_ERROR(__VA_ARGS__); \
        LOG_ERROR("If this message is printed, there is a bug here."); \
        std::abort();   \
    }

namespace huleibao
{
//////////////// Commonly used global static function encapsulation ////////////
//
/// Used to control frequency 
struct Rate
{
    /// Constructor
    /// \param hz     : frequency 
    Rate(float hz);

    /// Sleep until the time is up 
    void Sleep();

    /// sleep interval(milliseconds)
    int interval;
};
    

/// Timestamp accurate to microseconds
int64_t GetTimeStamp();

/// signal handler
void SignalHandler(int signum);

/// Waiting for signal 
void Spin();

/// check signal
bool Ok();


///////////////////////// Parameter Serialization ///////////////////////////////
std::vector<std::string> Split(const std::string& s, const std::string& delimiters = " ");

template<class T>
inline std::string ParamToString(T& val)
{ return std::to_string(val); }

/// bool->str
template<>
inline std::string ParamToString<bool>(bool& val)
{
    return val?"true":"false";
}

/// const char*->str
template<>
inline std::string ParamToString<const char*>(const char*& val)
{
    return val;
}

/// str->str
template<>
inline std::string ParamToString<std::string>(std::string& val)
{
    return val;
}

/// str->str
template<>
inline std::string ParamToString<const std::string>(const std::string& val)
{
    return val;
}

/// map->str
template<typename K, typename V>
inline std::string ParamToString(std::map<K, V>& val)
{
    std::string str = "";
    typename std::map<K, V>::iterator it = val.begin();
    for (; it != val.end(); it++)
    {
        str += ParamToString(it->first) + ":" + ParamToString(it->second);
        str += "+";
    }
    return str;
}
///////////////////////// Parameter Serialization ///////////////////////////////
template<class T>
T StringToParam(std::string& val);

/// str->int
template<>
inline int StringToParam<int>(std::string& val)
{
    return std::stoi(val);
}

/// str->long
template<>
inline long StringToParam<long>(std::string& val)
{
    return std::stol(val);
}

/// str->unsigned long
template<>
inline unsigned long StringToParam<unsigned long>(std::string& val)
{
    return std::stoul(val);
}

/// str->long long
template<>
inline long long StringToParam<long long>(std::string& val)
{
    return std::stoll(val);
}

/// str->float
template<>
inline float StringToParam<float>(std::string& val)
{
    return std::stof(val);
}

/// str->double
template<>
inline double StringToParam<double>(std::string& val)
{
    return std::stod(val);
}

/// str->bool
template<>
inline bool StringToParam<bool>(std::string& val)
{
    return val=="true"?true:false;
}

/// str->str
template<>
inline std::string StringToParam<std::string>(std::string& val)
{
    return val;
}

template<typename K, typename V>
inline std::map<K, V> StringToParam(const std::string& str)
{
    std::map<K, V> ret;
    if (str.empty()) return ret;
    std::string keySeg("+");
    std::string valSeg(":");
    std::vector<std::string> splits = Split(str, keySeg);
    // LOG_INFO("StringToParam: " << str);
    std::cout << splits.size() << std::endl;
    for (auto&& item : splits)
    {
        std::cout << item << std::endl;
        std::string::size_type pos = item.find(valSeg);
        std::string key = item.substr(0, pos);
        std::string val = item.substr(pos + valSeg.size());
        ret[StringToParam<K>(key)] = StringToParam<V>(val);
    }
    return ret;
}


} // namespace huleibao
#endif//util_hpp__
