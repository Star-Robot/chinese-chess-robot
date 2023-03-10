#include <cstdlib>
#include <yaml-cpp/yaml.h>

#include "base/node.hpp"
#include "base/publisher.hpp"
#include "base/subscriber.hpp"
#include "message/std_msgs.hpp"

using namespace huleibao;

//////////////////////////////////////
//// funcions decl////////////////////
void OnButtonDown(std_msgs::Int8::ConstPtr msg);

//////////////////////////////////////
//// global vars//////////////////////
std::shared_ptr<Node> g_node;
Publisher<std_msgs::String>::Ptr  g_pub;
Subscriber<std_msgs::Int8>::Ptr   g_sub;
// - keycode to mode
std::map<int, std::string> g_mode_mapper = 
{
    {79, "arm_calibrator"},
    {80, "arm_tester"},
    {81, "chinese_chess"}
};
bool g_mode_lock = false;

//////////////////////////////////////
//// main ////////////////////////////
int main(int argc, char* argv[])
{
    // - create mode_ctrl node
    g_node.reset(new Node("mode_ctrl"));

    // - publish topic_voice
    g_pub = g_node->Advertise<std_msgs::String>("topic_voice", 10);

    // - subscrib topic_button_down
    g_sub = g_node->Subscribe<std_msgs::Int8>("topic_button_down", 1, OnButtonDown);

    // - init empty mode
    g_node->SetParam("mode", "");

    LOG_INFO("[voice] checking node's starting ...");
    while(!(g_node->GetParam<bool>("start_interactor", false)) ||
          !(g_node->GetParam<bool>("start_voice", false)) )
    {
        // - wait a little moment
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    LOG_INFO("[voice] main nodes start done");

    // - play system_ready 
    std_msgs::String sysReadyMsg;
    sysReadyMsg.data = std::string("system_ready");
    g_pub->Publish(sysReadyMsg);

    // - notify core
    g_node->SetParam("start_mode_ctrl", true);

    // - wait signal
    Spin();

    // - leave empty mode
    g_node->SetParam("mode", "");

    // - notify core
    g_node->SetParam("start_mode_ctrl", false);

    return 0;
}


//////////////////////////////////////
//// function define//////////////////

void OnButtonDown(std_msgs::Int8::ConstPtr msg)
{
    LOG_INFO("[mode_ctrl] OnButtonDown");
    int keyCode = msg->data;

    // check if mode is locked.
    // 14, "BackSpace"
    if(g_mode_lock)
    {
        if(keyCode != 14) return;
        // - get mode and stop it
        std::string oldModeName = g_node->GetParam<std::string>("mode");
        if(!oldModeName.empty())
        {
            std::string stopProc = std::string("./scripts/stop_") + oldModeName + ".sh";
            LOG_INFO("[mode_ctrl] stop mode cmd: " << stopProc);
            system(stopProc.c_str());
            std_msgs::String stopMsg;
            stopMsg.data = std::string("stop_mode_") + oldModeName;
            g_pub->Publish(stopMsg);
            LOG_WARN("[mode_ctrl] stop mode: " << oldModeName);
        }
        // - leave empty mode
        g_node->SetParam("mode", "");
        // - play system ready voice
        std_msgs::String sysReadyMsg;
        sysReadyMsg.data = std::string("system_ready");
        g_pub->Publish(sysReadyMsg);
        g_mode_lock = false;
    }
    
    /* 
    * 79(1): arm_calibrator
    * 80(2): arm_tester
    * 81(3): chinese_chess 
    */
    if(g_mode_mapper.count(keyCode) == 1)
    {
        g_mode_lock = true;
        std::string& modeName = g_mode_mapper[keyCode];
        // - start new mode 
        std::string startProc = std::string("./scripts/start_") + modeName + ".sh";
        LOG_INFO("[mode_ctrl] start mode cmd: " << startProc);
        system(startProc.c_str());
        std_msgs::String startMsg;
        startMsg.data = std::string("mode_ctrl_") + modeName;
        g_pub->Publish(startMsg);

        // - set mode 
        g_node->SetParam("mode", modeName);
        LOG_WARN("[mode_ctrl] start mode: " << modeName);
    }
}
