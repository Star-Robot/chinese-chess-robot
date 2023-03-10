#ifndef serial_manager_hpp__
#define serial_manager_hpp__
#include <memory>
#include <string>

class Uart;
class SerialManager
{
public:
    SerialManager(int uart_id);
    void sendMessage(const std::string hes_str_message);
    std::string sendAndReciveMessage(const std::string hes_str_message);
    std::string reciveMessage();
    void clearMsgBuf();
private:
    std::shared_ptr<Uart> uart_ptr_;
    int uart_fd_ = -1;
};

#endif// serial_manager_hpp__


