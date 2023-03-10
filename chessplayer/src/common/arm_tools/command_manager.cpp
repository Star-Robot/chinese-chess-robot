#include "command_manager.hpp"
#include "serial_manager.hpp"
#include "config.hpp"
#include "util.hpp"

CommandManager::CommandManager()
{
    mpSerial_.reset(new SerialManager(Config::serial_name));
}


/**
 * function: 计算校验位
 * params:
 *  id: 舵机编号
 *  length: 指令参数的长度
 *  instruction: 指令编号
 *  params: 所有指令参数
 * return: 返回十六进制字符串格式的校验位
*/
std::string CommandManager::CalCheckSum(
    const int arm_id,
    const int length,
    const int instruction,
    std::vector<std::string> params)
{
    int sum = arm_id + length + instruction;
    for (auto param : params)
    {
        sum += Util::hexStr2Int(param);
    }
    sum = ~sum & 0xFF;
    return Util::int2Hex(sum, 2);
}


/**
 * function: 查询舵机工作状态PING
 * params:
 *  id: 舵机的编号
 * return: 工作状态量：1表示正常，-1表示异常
*/
int CommandManager::PingDevice(const int arm_id)
{
    int instruction = 1; // 位置驱动指令（十进制）
    std::vector<std::string> params = {"02"};
    SendCommand(arm_id, instruction, params);
    std::vector<std::string> recParams = ReceiveAnswer(arm_id);
    // if (recParams.size() == 0 || recParams[0] != "02")
    if (recParams.size() == 0)
    {
        std::cout<<"Steering gear "<<arm_id<<" is in error work"<<std::endl;
        return -1;
    }
    std::cout<<"Steering gear "<<arm_id<<" is in normal work"<<std::endl;
    return 1;
}


/**
 * function: 扭矩开关，
            0表示关闭扭矩使能，可以人工拖动机械臂的转动；
            1表示开始扭矩使能，人工无法转动机械臂。
 * params:
 *  id: 舵机的编号
 *  enable: 使能参数，0为关闭，1为开启
 * return: 
*/
void CommandManager::SwitchDeviceTwise(const int arm_id, int enable)
{
    int instruction = 3; // 位置驱动指令（十进制）
    std::vector<std::string> params = {"28"};
    if (enable == 0) params.emplace_back("00");
    else params.emplace_back("01");
    // std::vector<std::string> params = {"25", "00"};
    SendCommand(arm_id, instruction, params);
    std::vector<std::string> recParams = ReceiveAnswer(arm_id);
    if (enable==0)
        std::cout<<"Steering gear "<<arm_id<<" torque enable is close"<<std::endl;
    else
        std::cout<<"Steering gear "<<arm_id<<" torque enable is open"<<std::endl;
}


/**
 * function: 读取舵机当前的角度值
 * params:
 *  id: 舵机的编号
 * return: 舵机当前的角度值，单位为度
*/
float CommandManager::ReadArmStatus(int arm_id, bool is_SCS)
{
    int instruction= 2; // 位置驱动指令（十进制）
    std::vector<std::string> params = {"38", "08"}; // 指令首地址为0x38, 读取数据长度为0x08
    SendCommand(arm_id, instruction, params);

    std::vector<std::string> recParams = ReceiveAnswer(arm_id);
    // if (recParams.size() == 0 || recParams[0] != "02")
    if (recParams.size() == 0)
    {
        std::cout<<"Steering gear "<<arm_id <<" is in error work"<<std::endl;
        return -1;
    }
    int curPos, curVec, curBurden, curVoltage, curTemperature;
    if (is_SCS)
    {
        curPos = Util::hexStr2Int(recParams[1] + recParams[2]);
        curVec = Util::hexStr2Int(recParams[3] + recParams[4]);
        curBurden = Util::hexStr2Int(recParams[5] + recParams[6]);
        curVoltage = Util::hexStr2Int(recParams[7]);
        curTemperature = Util::hexStr2Int(recParams[8]);
    }
    else
    {
        curPos = Util::hexStr2Int(recParams[2] + recParams[1]);
        curVec = Util::hexStr2Int(recParams[4] + recParams[3]);
        curBurden = Util::hexStr2Int(recParams[6] + recParams[5]);
        curVoltage = Util::hexStr2Int(recParams[7]);
        curTemperature = Util::hexStr2Int(recParams[8]);
    }
    // std::cout<<"curPos= "<<curPos
    //     <<", curVec= "<<curVec
    //     <<", curBurden= "<<curBurden
    //     <<", curVoltage= "<<curVoltage
    //     <<", curTemperature= "<<curTemperature<<std::endl;
    return PositionToAngle(curPos, is_SCS);
}


/**
 * function: 将舵机旋转到指定角度位置
 * params:
 *  id: 舵机的编号
 * dst_angle: 0-360度，最小分辨角度为0.088度
 * speed: 0-3400， 换算方法为：speed*0.732/50 rpm(rpm：设备每分钟的旋转次数)
 * accel: 0-255, 单位为8.878度/s2 即换算方法为：accel*8.878
 * return: 舵机当前的角度值，单位为度
*/
void CommandManager::WriteData(
    int arm_id, 
    float dst_angle, 
    int speed, 
    bool is_SCS,
    int accel)
{
    int instruction = 3; // 位置驱动指令（十进制）
    // 舵机目的地位置解算
    std::string dstHexStr = Util::int2Hex(int(AngleToPosition(dst_angle, is_SCS)), 4);
    std::vector<std::string> dstItems = Util::SplitWithStl(dstHexStr, " ");
    // std::cout<<"set curSpeed is: "<<speed<<std::endl;
    std::string speedHexStr = Util::int2Hex(int(AngleToPosition(speed, is_SCS)), 4);
    std::vector<std::string> speedItems = Util::SplitWithStl(speedHexStr, " ");
    std::vector<std::string> params;
    params.emplace_back("2A"); // 无加速度指令的首地址为2A（十六进制）
    // 如果有加速度设定
    if (accel != -1)
    {
        params[0] = "29"; // 有加速度指令的首地址为29（十六进制）
        // std::cout<<"set curAccel is: " <<accel<<std::endl;
        std::string accelHexStr = Util::int2Hex(accel, 2);
        params.emplace_back(accelHexStr); // 加速度字节
    }
    if (is_SCS)
    {
        std::string tmp;
        tmp = dstItems[0];
        dstItems[0] = dstItems[1];
        dstItems[1] = tmp;

        tmp = speedItems[0];
        speedItems[0] = speedItems[1];
        speedItems[1] = tmp;
    }
    params.emplace_back(dstItems[1]); // 位置的低字节
    params.emplace_back(dstItems[0]); // 位置的高字节
    params.emplace_back("00"); // 时间低字节（在位置控制模式下无功能）
    params.emplace_back("00"); // 时间高字节（在位置控制模式下无功能）
    params.emplace_back(speedItems[1]); // 速度低字节
    params.emplace_back(speedItems[0]); // 速度高字节
    // 将指令发出去
    SendCommand(arm_id, instruction, params);
    std::vector<std::string> recParams = ReceiveAnswer(arm_id);
}


/**
 * function: 计算指令长度、计算校验位，将指令发出
 * params:
 *  id: 需要控制的舵机ID号
 *  instruction: 舵机控制指令
 *  params: 具体控制指令参数等
 * return:
*/
void CommandManager::SendCommand(
    const int arm_id, 
    const int instruction, 
    const std::vector<std::string> params)
{
    int length = 2; // 指令+校验
    length += params.size();
    std::string checkSum = CalCheckSum(arm_id, length, instruction, params);
    // 字头 + ID号 + 长度 + 指令
    std::string command = msPrefix_ + " " + 
        Util::int2Hex(arm_id, 2) + " " +
        Util::int2Hex(length, 2) + " " + 
        Util::int2Hex(instruction, 2) + " ";
    for (auto param : params)
        command += " " + param;
    // 校验位
    command += checkSum;
    // std::cout<<"command: "<<command<<std::endl;
    mpSerial_->sendMessage(command);
}

/**
 * function: 接收
 * params:
 *  id: 需要控制的舵机ID号
 * return:
*/
std::vector<std::string> CommandManager::ReceiveAnswer(int arm_id)
{
    std::string receiveMessage = mpSerial_->reciveMessage();
    // std::cout<<"receiveMessage: "<<receiveMessage<<std::endl;
    std::vector<std::string> items = Util::SplitWithStl(receiveMessage, " ");
    if (items.size() < 6)
    {
        std::cout<<"arm_id "<< arm_id <<" answer is error, and len is "<<items.size()<<std::endl;
        return {};
    }
    std::string blanks("\f\v\r\t\n ");
    std::string armIdStr = Util::int2Hex(arm_id, 2);
    armIdStr.erase(0,armIdStr.find_first_not_of(blanks));
    armIdStr.erase(armIdStr.find_last_not_of(blanks) + 1);
    if (items[2] != armIdStr)
    {
        std::cout<<"arm_id is "<<items[2]<<", not match "<<armIdStr<<std::endl;
        std::cout<<"length: "<<items[2].size()<<" vs "<<armIdStr.size()<<std::endl;
        return {};
    }
    return {items.begin()+4, items.end()};
}


float CommandManager::PositionToAngle(int data, bool is_SCS)
{
    if (is_SCS)
        return (float)(data % 1024) / 1024.0 * 360.0;
    else
        return (float)(data % 4096) / 4096.0 * 360.0;
}

float CommandManager::AngleToPosition(float angle, bool is_SCS)
{
    if (is_SCS)
        return angle / 360.0 * 1024.0;
    else
        return angle / 360.0 * 4096.0;
}


void CommandManager::ClearMsgBuf()
{
    mpSerial_->clearMsgBuf();
}
