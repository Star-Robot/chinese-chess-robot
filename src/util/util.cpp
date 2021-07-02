#include "util/util.hpp"


namespace huleibao
{

int64_t GetTimeStamp()
{
    //获取当前时间点
    std::chrono::time_point<std::chrono::system_clock,std::chrono::microseconds> tp
        = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    //计算距离1970-1-1,00:00的时间长度
    std::time_t timestamp =  tp.time_since_epoch().count();
    return uint64_t(timestamp);
}


} // namespace huleibao
