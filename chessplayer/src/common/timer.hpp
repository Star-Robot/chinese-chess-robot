#ifndef timer_hpp__
#define timer_hpp__

#include <iostream>
#include <string>
#include <chrono>

class Timer
{
public:
    Timer()
    {
        Restart();
    }

    /**
    * 启动计时
    */
    inline void Restart()
    {
        _start_time = std::chrono::steady_clock::now();
    }

    /**
    * 结束计时
    * @return 返回ms数
    */
    inline double Elapsed(bool restart = false)
    {
        _end_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> diff = _end_time - _start_time;
        if (restart)
            this->Restart();
        return diff.count() * 1000;
    }

private:
    std::chrono::steady_clock::time_point _start_time;
    std::chrono::steady_clock::time_point _end_time;
}; // timer

#endif //timer_hpp__

