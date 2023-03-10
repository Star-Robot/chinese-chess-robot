#ifndef command_manager_H__
#define command_manager_H__

#include <string>
#include <vector>
#include <memory>

class SerialManager;
class CommandManager{
public:
    CommandManager();
    ~CommandManager(){}

    /**
     * function: 计算校验位
     * params:
     *  id: 舵机编号
     *  length: 指令参数的长度
     *  instruction: 指令编号
     *  params: 所有指令参数
     * return: 返回十六进制字符串格式的校验位
    */
    std::string CalCheckSum(
        const int arm_id, 
        const int length, 
        const int instruction, 
        std::vector<std::string> params);
    

    /**
     * function: 将舵机数据转换为具体的角度
     * params:
     *  data: 舵机计量数据(0-4096)
     * return: 舵机计量数据对应的角度值（0-360）
    */
    float PositionToAngle(int data, bool is_SCS=false);


    /**
     * function: 将角度转换为舵机计量数据
     * params:
     *  angle: 角度数据(0-360)
     * return: 角度值对应的舵机计量数据(0-4096)
    */
    float AngleToPosition(float angle, bool is_SCS=false);


    /**
     * function: 查询舵机工作状态PING
     * params:
     *  id: 舵机的编号
     * return: 工作状态量：1表示正常，-1表示异常
    */
    int PingDevice(int arm_id);

    
    /**
     * function: 扭矩开关，
                0表示关闭扭矩使能，可以人工拖动机械臂的转动；
                1表示开始扭矩使能，人工无法转动机械臂。
     * params:
     *  id: 舵机的编号
     * return: 
    */
    void SwitchDeviceTwise(const int arm_id, int enable);


    /**
     * function: 读取舵机当前的角度值
     * params:
     *  id: 舵机的编号
     *  is_SCS: 舵机是否是SCS，SCS型号的舵机数据高低位与STS的不同
     * return: 舵机当前的角度值，单位为度
    */
    float ReadArmStatus(int arm_id, bool is_SCS=false);


    /**
     * function: 将舵机旋转到指定角度位置
     * params:
     *  id: 舵机的编号
     * dst_angle: 0-360度，最小分辨角度为0.088度
     * speed: 0-3400， 换算方法为：speed*0.732/50 rpm(rpm：设备每分钟的旋转次数)
     * accel: 0-255, 单位为8.878度/s2 即换算方法为：accel*8.878
     * return: 舵机当前的角度值，单位为度
    */
    void WriteData(
        int arm_id, 
        float dst_angle, 
        int speed = 1000, 
        bool is_SCS = false,
        int accel = -1);
    
    /**
     * function: 计算指令长度、计算校验位，将指令发出
     * params:
     *  id: 需要控制的舵机ID号
     *  instruction: 舵机控制指令
     *  params: 具体控制指令参数等
     * return:
    */
    void SendCommand(int arm_id, int instruction, std::vector<std::string> params);

    /**
     * function: 接收
     * params:
     *  id: 需要控制的舵机ID号
     * return:
    */
    std::vector<std::string> ReceiveAnswer(int arm_id);


    void ClearMsgBuf();


private:
    const std::string msPrefix_ = "FF FF";
    const std::string msBroadcastId_ = "FE";
    bool mbAsynQueEmpty_ = true;
    std::shared_ptr<SerialManager> mpSerial_;
};

#endif // command_manager_H__
