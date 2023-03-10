#include <fstream>
#include <yaml-cpp/yaml.h>

#include "base/node.hpp"
#include "base/publisher.hpp"
#include "base/subscriber.hpp"
#include "message/std_msgs.hpp"
#include "common/arm_tools/arm_controller.hpp"

using namespace huleibao;

//////////////////////////////////////
//// funcions decl////////////////////
void OnButtonDown(std_msgs::Int8::ConstPtr msg);

//////////////////////////////////////
//// global vars//////////////////////
std::vector<std::pair<float, float>> g_arm_xys;
Subscriber<std_msgs::Int8>::Ptr     g_sub;
Publisher<std_msgs::String>::Ptr    g_pub;
std::shared_ptr<ArmController>      g_arm_ctrl;
std::string                         g_calib_save; 

//////////////////////////////////////
//// main ////////////////////////////
int main(int argc, char* argv[])
{
    // - create arm_calibrator node
    Node node("arm_calibrator");
    
    // - get configs
    std::string resrc = "./resource/";
    std::string cfgFile = resrc + "game_config.yml";
    YAML::Node config = YAML::LoadFile(cfgFile);
    g_calib_save = resrc + config["game_config"]["arm"]["calib"].as<std::string>();

    // - creat arm ctrl
    bool enable_force = false;
    g_arm_ctrl.reset(new ArmController(enable_force));

    // - publish topic_voice
    g_pub = node.Advertise<std_msgs::String>("topic_voice", 1);

    // - subscrib topic_button_down
    g_sub = node.Subscribe<std_msgs::Int8>("topic_button_down", 1, OnButtonDown);

    // - notify core
    node.SetParam("start_arm_calibrator", true);

    // - wait signal
    Spin();

    // - notify core
    node.SetParam("start_arm_calibrator", false);

    return 0;
}


//////////////////////////////////////
//// function define//////////////////

void OnButtonDown(std_msgs::Int8::ConstPtr msg)
{
    LOG_INFO("[arm_calibrator] OnButtonDown");
    int keyCode = msg->data;

    /* 
    * 96(ENTER)
    */
    // - waitting for 'Enter' only
    if(keyCode != 96) return;

    // - 必须按[↖↗↙↘]顺序标定
    LOG_INFO("[arm_calibrator] read arm xy ...");
    std::pair<float, float> armXY = g_arm_ctrl->ReadArmXY();
    if(!g_arm_xys.empty())
    {
        auto& lastXY = g_arm_xys.back();
        // - 如果和上次读取坐标是距离小于2cm，则覆盖之
        if( std::abs(lastXY.first - armXY.first)   < 0.02 ||
            std::abs(lastXY.second - armXY.second) < 0.02
        )
        {
            LOG_WARN("[arm_calibrator] delete grid " << g_arm_xys.size());
            g_arm_xys.pop_back();
        }
        // - 如果差距过大，之前已读取4个了，则清除之前的
        else if(g_arm_xys.size() == 4)
        {
            LOG_WARN("[arm_calibrator] delete grid 0 1 2 3");
            g_arm_xys.clear();
        }
    }
    g_arm_xys.emplace_back(armXY);
    LOG_INFO("[arm_calibrator] arm(x,y)=(" << armXY.first << "," 
        << armXY.second << ") set to grid " << g_arm_xys.size());
    // - play calib voice
    std_msgs::String calibMsg;
    calibMsg.data = std::string("arm_calibrator_") + std::to_string(g_arm_xys.size());
    g_pub->Publish(calibMsg);
    // - 保存起来
    if(g_arm_xys.size() == 4) 
    {
        YAML::Node nArmXYS;
        nArmXYS["top_left"]     = g_arm_xys[0];
        nArmXYS["top_right"]    = g_arm_xys[1];
        nArmXYS["bottom_left"]  = g_arm_xys[2];
        nArmXYS["bottom_right"] = g_arm_xys[3];
        std::ofstream of(g_calib_save);
        of << nArmXYS;
        of.close();
        // - play save voice
        std_msgs::String saveMsg;
        saveMsg.data = "arm_calibrator_save";
        g_pub->Publish(saveMsg);
        LOG_INFO("[arm_calibrator] save arm xys yaml:\n" << nArmXYS);
    }
}
