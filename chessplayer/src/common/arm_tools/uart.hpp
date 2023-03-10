#ifndef UART_H
#define UART_H
 
#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <memory>
 
// #define UART1_DEV "/dev/ttyS1"
// #define UART2_DEV "/dev/ttyS2"
// #define UART3_DEV "/dev/ttyS3"
// #define UART4_DEV "/dev/ttyS4"
// #define UART5_DEV "/dev/ttyS5"

#define UART1_DEV "/dev/ttyUSB0"
#define UART2_DEV "/dev/ttyUSB1"
#define UART3_DEV "/dev/ttyUSB2"
#define UART4_DEV "/dev/ttyUSB3"
#define UART5_DEV "/dev/ttyUSB4"
 
#define UART1 0
#define UART2 1
#define UART3 2
#define UART4 3
#define UART5 4
 
#define BAUDRATE1 B1000000
// #define BAUDRATE1 B115200
 
using namespace std;
 
class Uart {
public :
  ~Uart();
   Uart();
  int uartOpen(int port, int flag, speed_t buadrate);
  int uartWrite(int fd, char *data, int num);
  int  uartRead(int fd, char *data, int num);
  void uartClose(int fd);
  char convertHexChar(char ch);
  int str2Hex(char* str, char* data, int inLen);
private:
  int uartInit(int fd,speed_t buadrate);
 
  static int uart1_only_fd;
  static int uart2_only_fd;
  static int uart3_only_fd;
  static int uart4_only_fd;
  static int uart5_only_fd;

  static bool uart1_inited_flag;
  static bool uart2_inited_flag;
  static bool uart3_inited_flag;
  static bool uart4_inited_flag;
  static bool uart5_inited_flag;

};
 
#endif // UART_H
