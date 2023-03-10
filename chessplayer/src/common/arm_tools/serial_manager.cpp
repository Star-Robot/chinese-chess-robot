#include "serial_manager.hpp"
#include "config.hpp"
#include "util.hpp"
#include "uart.hpp"
#include "base/node.hpp"

SerialManager::SerialManager(int uart_id)
{
    uart_ptr_.reset(new Uart());
    uart_fd_ = uart_ptr_->uartOpen(uart_id, O_RDWR |O_NONBLOCK | O_NOCTTY | O_NDELAY, Config::serial_freq);
    if (uart_fd_ < 0)
    {
        LOG_FATAL("[arm_controller] Serial open failed");
        return;
    }
    LOG_INFO("[arm_controller] Serial open success");
}

std::string SerialManager::sendAndReciveMessage(const std::string hes_str_message)
{
    std::string outMess = Util::hex2Str(hes_str_message);
    int outLen = outMess.size();
    // std::string outMess = hes_str_message;
    // int outLen = outMess.size();
    // std::cout<<" - [SerialManager] sendAndReciveMessage outMess: "<<outMess<<std::endl;
    uart_ptr_->uartWrite(uart_fd_,(char*)outMess.data(),outLen);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    int num;
    char rbuff[200];
    std::string feedbackMessage;
    while(true)
    {
        num = uart_ptr_->uartRead(uart_fd_, rbuff, 200);
        if (num <= 0) continue;
        feedbackMessage = Util::char2Hex(rbuff, num);
        break;
    }
    return feedbackMessage;
}

void SerialManager::sendMessage(const std::string hes_str_message)
{
    std::string outMess = Util::hex2Str(hes_str_message);
    int outLen = outMess.size();
    // std::string outMess = hes_str_message;
    // int outLen = outMess.size();
    // std::cout<<" - [SerialManager] sendMessage outMess: "<<outMess<<std::endl;
    uart_ptr_->uartWrite(uart_fd_,(char*)outMess.data(),outLen);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

std::string SerialManager::reciveMessage()
{
    int num;
    char rbuff[200];
    std::string feedbackMessage;
    while(true)
    {
        num = uart_ptr_->uartRead(uart_fd_, rbuff, 200);
        if (num <= 0) continue;
        feedbackMessage = Util::char2Hex(rbuff, num);
        break;
    }
    return feedbackMessage;
}

void SerialManager::clearMsgBuf()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    char rbuff[200];
    uart_ptr_->uartRead(uart_fd_, rbuff, 200);
}

