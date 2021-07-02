#include <atomic>
#include <csignal>
#include "util/util.hpp"


namespace huleibao
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


/// Global signal received flag
std::atomic_bool g_loop_go(true);

void SignalHandler(int signum)
{
    LOG_ERROR("Interrupt signal (" << signum << ") received.");
    g_loop_go = false;
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


} // namespace huleibao
