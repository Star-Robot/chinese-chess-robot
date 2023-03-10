#include <atomic>
#include <csignal>
#include "util/util.hpp"
#include "base/node.hpp"


namespace tinyros
{

Rate::Rate(float hz)
{ 
    // - to milliseconds
    interval = 1000.0 / hz;
};


void Rate::Sleep()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
}


int64_t GetTimeStamp()
{
    //获取当前时间点
    std::chrono::time_point<std::chrono::system_clock,std::chrono::microseconds> tp
        = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    //计算距离1970-1-1,00:00的时间长度
    std::time_t timestamp =  tp.time_since_epoch().count();
    return uint64_t(timestamp);
}


/// Node for SayBey 
Node* g_node_ptr = nullptr;

/// Global signal received flag
std::atomic_bool g_loop_go(true);

void SignalHandler(int signum)
{
    LOG_ERROR("Interrupt signal (" << signum << ") received.");
    g_loop_go = false;
    // - Disconnect itself from core
    if(g_node_ptr) g_node_ptr->ByeCore();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // - Force exit pragram.
    std::exit(0);
}


void Spin()
{
    while(g_loop_go)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


bool Ok()
{
    return g_loop_go;
}


std::vector<std::string> Split(const std::string& s, const std::string& delimiters)
{
    std::vector<std::string> tokens;
    std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
    std::string::size_type pos = s.find_first_of(delimiters, lastPos);
    while (std::string::npos != pos || std::string::npos != lastPos) {
        tokens.push_back(s.substr(lastPos, pos - lastPos));//use emplace_back after C++11
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
    return tokens;
}
} // namespace tinyros
