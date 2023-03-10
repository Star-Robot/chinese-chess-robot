#include "uart.hpp"
 
int Uart::uart1_only_fd = 0;
int Uart::uart2_only_fd = 0;
int Uart::uart3_only_fd = 0;
int Uart::uart4_only_fd = 0;
int Uart::uart5_only_fd = 0;
bool Uart::uart1_inited_flag = false;
bool Uart::uart2_inited_flag = false;
bool Uart::uart3_inited_flag = false;
bool Uart::uart4_inited_flag = false;
bool Uart::uart5_inited_flag = false;
 
Uart::Uart()
{
}
 
 
int Uart::uartInit(int fd,speed_t buadrate)
{
    struct termios opt;
 
    tcgetattr(fd,&opt);
 
    cfsetispeed(&opt,buadrate);
    cfsetospeed(&opt,buadrate);
 
    opt.c_cflag  |=  CLOCAL | CREAD;
    opt.c_cflag  &=  ~CRTSCTS;
    opt.c_cflag &= ~CSIZE;
    opt.c_cflag |= CS8;
    opt.c_cflag &= ~PARENB;
    opt.c_cflag &= ~CSTOPB;
    opt.c_iflag &= ~INPCK;
    opt.c_iflag &= ~(ICRNL|BRKINT|ISTRIP);
    opt.c_iflag &= ~(IXON|IXOFF|IXANY);
    opt.c_oflag &= ~OPOST;
 
    opt.c_lflag = ~(ICANON | ECHO | ECHOE | ISIG);
 
    tcflush(fd,TCIFLUSH);
    if (tcsetattr(fd,TCSANOW,&opt) != 0)
      {
	 return -1;
      }
	return 0;
}
 
 
int Uart::uartOpen(int port, int flag, speed_t buadrate)
{
    if(port == UART1)
    {
        if(uart1_inited_flag == false)
        {
            uart1_only_fd = open(UART1_DEV, flag);
            if(uart1_only_fd < 0)
            {
                return -1;
            }
            else
            {
                uartInit(uart1_only_fd,buadrate);
                uart1_inited_flag = true;
                return uart1_only_fd;
            }
        }
        else
        {
            return uart1_only_fd;
        }
    }
    else if(port == UART2)
    {
        if(uart2_inited_flag == false)
        {
            uart2_only_fd = open(UART2_DEV, flag);
            if(uart2_only_fd < 0)
            {
                return -1;
            }
            else
            {
                uartInit(uart2_only_fd,buadrate);
                uart2_inited_flag = true;
                return uart2_only_fd;
            }
        }
        else
        {
            return uart2_only_fd;
        }
    }
    else if(port == UART3)
    {
        if(uart3_inited_flag == false)
        {
            uart3_only_fd = open(UART3_DEV, flag);
            if(uart3_only_fd < 0)
            {
                return -1;
            }
            else
            {
                uartInit(uart3_only_fd,buadrate);
                uart3_inited_flag = true;
                return uart3_only_fd;
            }
        }
        else
        {
            return uart3_only_fd;
        }
    }
    else if(port == UART4)
    {
        if(uart4_inited_flag == false)
        {
            uart4_only_fd = open(UART4_DEV, flag);
            if(uart4_only_fd < 0)
            {
                return -1;
            }
            else
            {
                uartInit(uart4_only_fd,buadrate);
                uart4_inited_flag = true;
                return uart4_only_fd;
            }
        }
        else
        {
            return uart4_only_fd;
        }
    }
    else if(port == UART5)
    {
        if(uart5_inited_flag == false)
        {
            uart5_only_fd = open(UART5_DEV, flag);
            if(uart5_only_fd < 0)
            {
                return -1;
            }
            else
            {
                uartInit(uart5_only_fd,buadrate);
                uart5_inited_flag = true;
                return uart5_only_fd;
            }
        }
        else
        {
            return uart5_only_fd;
        }
    }
    return -1;
}

 
int Uart::uartWrite(int fd, char *data , int num)
{    
  int ret  = -1;
  ret = write(fd, data, num); 
  return ret;
}

char Uart::convertHexChar(char ch)
{    
    if((ch >= '0') && (ch <= '9'))    
        return ch-0x30;    
    else if((ch >= 'A') && (ch <= 'F'))    
        return ch-'A'+10;    
    else if((ch >= 'a') && (ch <= 'f'))    
        return ch-'a'+10;    
    else return (-1);    
}

int Uart::str2Hex(char* str, char* data, int inLen)
{
    int t,t1;
    int rlen=0,len=inLen;
    std::string message;
    for(int i=0;i<len;)
    {
        char l,h=str[i];
        if(h==' ')
        {
            i++;
            continue;
        }
        i++;
        if(i>=len) break;
        l=str[i];
        t=convertHexChar(h);
        t1=convertHexChar(l);
        if((t==16)||(t1==16))
            break;
        else 
            t=t*16+t1;
        i++;
        message += t;
        rlen++;
    }
    data = (char*)message.c_str();
    return rlen;
} 

 
int  Uart::uartRead(int fd, char *data, int num)
{
   int ret = -1;
   int i=0;
   char buf[1024] = {0};
   if(num > 1024)
   {
     ret = read(fd, buf, 1024);
   }
   else
   {
      ret = read(fd, buf, num);
   }
 
   for (i=0;i<num;i++)
   {
      // data[i]=convertHexChar(buf[i]);
      data[i]=buf[i];
   }
 
   return ret;
}
void Uart::uartClose(int fd)
{
   if(fd > 0)
      close(fd);
}
 
Uart::~Uart()
{
 
}
