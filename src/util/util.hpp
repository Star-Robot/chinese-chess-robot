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

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <mutex>
#include <memory>
#include <queue>
#include <stdio.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/stat.h>
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
    
    /// Timestamp accurate to microseconds
    int64_t GetTimeStamp();

	/// Progress visualization function
	/// \param cnt     : Current loops
	/// \param total   : Total loops
	/// \param interval: Visual interval
	static void ShowProgressBar(int cnt, int total, int interval);


} // namespace huleibao
#endif//util_hpp__
