#include <iostream>
#include <math.h>
#include <unistd.h>

#include "arm_controller.hpp"
#include "command_manager.hpp"
#include "config.hpp"
#include "base/node.hpp"

#define PI 3.141592653

ArmController::ArmController(bool enable_force)
{
    mpComManager_.reset(new CommandManager());
    int upArmConn = mpComManager_->PingDevice(Config::upper_arm_id);
    int lowerArmConn = mpComManager_->PingDevice(Config::lower_arm_id);
    int handConn = mpComManager_->PingDevice(Config::hand_id);
    if (upArmConn < 0 ||
        lowerArmConn < 0 ||
        handConn < 0)
    {
        LOG_FATAL("[arm_controller] arm connected failed");
        return;
    }
    LOG_INFO("[arm_controller] arm connected success");
    
    Reset();
    if(!enable_force)
    {
        mpComManager_->SwitchDeviceTwise(Config::upper_arm_id, 0);
        mpComManager_->SwitchDeviceTwise(Config::lower_arm_id, 0);
    }
}


std::vector<int> ArmController::CalVel(float dst_angle1, float dst_angle2)
{
    float curAngle1 = mpComManager_->ReadArmStatus(Config::upper_arm_id);
    float curAngle2 = mpComManager_->ReadArmStatus(Config::lower_arm_id);

    float diffArm1Angle = std::abs(curAngle1 - dst_angle1);
    float diffArm2Angle = std::abs(curAngle2 - dst_angle2);
    float minAngle = std::min(diffArm1Angle, diffArm2Angle);

    int arm1Vel = int(diffArm1Angle / minAngle * Config::min_arm_omiga);
    int arm2Vel = int(diffArm2Angle / minAngle * Config::min_arm_omiga);
    arm1Vel = std::max(Config::min_arm_omiga, std::min(arm1Vel, Config::max_arm_omiga));
    arm2Vel = std::max(Config::min_arm_omiga, std::min(arm2Vel, Config::max_arm_omiga));

    return std::vector<int>{arm1Vel, arm2Vel};
}


void ArmController::Reset()
{
    std::vector<int> armVels = CalVel(Config::upper_arm_id, Config::lower_arm_id);
    DriveByAbsAngle(Config::upper_arm_id, Config::upper_arm_init_angle, armVels[0]);
    DriveByAbsAngle(Config::lower_arm_id, Config::lower_arm_init_angle, armVels[1]);

    LOG_INFO("[arm_controller] upper_arm check ...");
    Check(Config::upper_arm_id, Config::upper_arm_init_angle, false);
    LOG_INFO("[arm_controller] lower_arm check ...");
    Check(Config::lower_arm_id, Config::lower_arm_init_angle, false);
    DriveByAbsAngle(Config::hand_id, Config::hand_init_angle, Config::hand_omiga, true);
    LOG_INFO("[arm_controller] hand check ...");
    Check(Config::hand_id, Config::hand_init_angle, true);
    LOG_INFO("[arm_controller] arm reset done.");
}


void ArmController::ResetHand()
{
    DriveByAbsAngle(Config::hand_id, Config::hand_init_angle, Config::hand_omiga, true);
    Check(Config::hand_id, Config::hand_init_angle, true);
}


void ArmController::DriveByAbsAngle(
    int arm_id, 
    float angle,
    int speed,
    bool is_SCS
    )
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mpComManager_->WriteData(arm_id, angle, speed, is_SCS);
}


void ArmController::DriveByXY(float x, float y)
{
    const float& uppLen = Config::upper_arm_len;
    const float& lowLen = Config::lower_arm_len;
    const float& handLen = Config::hand_len;

    float curAngle1 = mpComManager_->ReadArmStatus(Config::upper_arm_id);
    float curAngle2 = mpComManager_->ReadArmStatus(Config::lower_arm_id);

    float distance = std::sqrt(x*x + y*y);
    if (distance > (uppLen+handLen) || y < 0)
    {
        std::cout<<"Cloud not arrive the position!"<<std::endl;
        return;
    }
    float angleInit;
    if (x == 0)
        angleInit = 180;
    else if (x < 0)
        angleInit = std::atan(y/x)/PI*180+270;
    else
        angleInit = std::atan(y/x)/PI*180+90;
    
    float cosAngleTmp = ((uppLen*uppLen+distance*distance-handLen*handLen)/(2*uppLen*distance));
    float angleTmp = std::acos(cosAngleTmp)/PI*180;

    bool choice = std::abs(angleInit-angleTmp-curAngle1) < std::abs(angleInit+angleTmp-curAngle1);
    float dstAngle1;
    if (angleInit-angleTmp < Config::upper_arm_angle_range[0] ||
        angleInit-angleTmp > Config::upper_arm_angle_range[1])
    {
        if (angleInit+angleTmp < Config::upper_arm_angle_range[0] ||
            angleInit+angleTmp > Config::upper_arm_angle_range[1])
        {
            std::cout<<"solve angle1 error!"<<std::endl;
            return;
        }
        else
        {
            dstAngle1 = angleInit+angleTmp;
        }
    }
    else
    {
        if (angleInit+angleTmp < Config::upper_arm_angle_range[0] ||
            angleInit+angleTmp > Config::upper_arm_angle_range[1])
        {
            dstAngle1 = angleInit-angleTmp;
        }
        else
        {
            dstAngle1 = choice ? angleInit-angleTmp : angleInit+angleTmp;
        }
    }

    cosAngleTmp = ((uppLen*uppLen+handLen*handLen-distance*distance)/(2*uppLen*handLen));
    angleTmp = std::acos(cosAngleTmp)/PI*180;
    float dstAngle2 = dstAngle1<angleInit ? 360-angleTmp+Config::wrist_angle : angleTmp+Config::wrist_angle;
    // LOG_INFO("uppLen: "<<uppLen<<", handLen: "<<handLen<<", distance: "<<distance<<", cosAngleTmp: "<<cosAngleTmp<<", angleTmp: "<<angleTmp<<", wrist_angle: "<<Config::wrist_angle);
    
    std::vector<int> armVels = CalVel(dstAngle1, dstAngle2);
    // LOG_INFO("dstAngle1: "<<dstAngle1<<", dstAngle2: "<<dstAngle2);
    DriveByAbsAngle(Config::upper_arm_id, dstAngle1, armVels[0]);
    DriveByAbsAngle(Config::lower_arm_id, dstAngle2, armVels[1]);

    Check(Config::upper_arm_id, dstAngle1, false);
    Check(Config::lower_arm_id, dstAngle2, false);
}


void ArmController::TakePiece()
{
    const int& handId = Config::hand_id;
    const float& handTakeAngle = Config::hand_take_angle;
    const int& w = Config::hand_omiga;
    mpComManager_->WriteData(handId, handTakeAngle, w, true);
    Check(handId, handTakeAngle, true);
    const float& handInitAngle = Config::hand_init_angle;
    mpComManager_->WriteData(handId, handInitAngle, w, true);
    Check(handId, handInitAngle, true);
}


void ArmController::DropPiece()
{
    const int& handId = Config::hand_id;
    const float& handDropAngle = Config::hand_drop_angle;
    const int& w = Config::hand_omiga;
    mpComManager_->WriteData(handId, handDropAngle, w, true);
    Check(handId, handDropAngle, true);
    // ResetHand();
    // const float& handInitAngle = Config::hand_init_angle;
    // mpComManager_->WriteData(handId, handInitAngle, w, true);
    // Check(handId, handInitAngle, true);
}


void ArmController::Check(int arm_id, float arm_angle, bool is_SCS)
{
    // return;
    mpComManager_->ClearMsgBuf();
    float curAngle = mpComManager_->ReadArmStatus(arm_id, is_SCS);
    while(std::abs(curAngle - arm_angle) > 20)
    {
        std::cout<<"curAngle="<<curAngle<<", dstAngle="<<arm_angle<<std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        curAngle = mpComManager_->ReadArmStatus(arm_id, is_SCS);
    }
}


std::pair<float, float> ArmController::ReadArmXY()
{
    //todo
    float curAngle1 = mpComManager_->ReadArmStatus(Config::upper_arm_id);
    float curAngle2 = mpComManager_->ReadArmStatus(Config::lower_arm_id);
    curAngle2 -= Config::wrist_angle;

    float arm1_x = Config::upper_arm_len*std::sin(curAngle1/180.0*PI);
    float arm1_y = Config::upper_arm_len*std::cos(curAngle1/180.0*PI);

    float arm2_x = Config::lower_arm_len*std::sin((curAngle2+curAngle1-180)/180.0*PI);
    float arm2_y = Config::lower_arm_len*std::cos((curAngle2+curAngle1-180)/180.0*PI);

    float pos_x = arm1_x + arm2_x;
    float pos_y = arm1_y + arm2_y;
    pos_y = -pos_y;
    LOG_INFO("cur pos: "<<pos_x<<", "<<pos_y);
    return std::make_pair(pos_x, pos_y);
}
