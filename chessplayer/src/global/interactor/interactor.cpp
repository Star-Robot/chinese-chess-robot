#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <yaml-cpp/yaml.h>

#include <stdio.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <map>

#include "base/node.hpp"
#include "base/publisher.hpp"
#include "message/std_msgs.hpp"

using namespace huleibao;

//////////////////////////////////////
//// funcions decl////////////////////

//////////////////////////////////////
//// global vars//////////////////////
Publisher<std_msgs::Int8>::Ptr      g_btn_pub;
Publisher<std_msgs::String>::Ptr    g_snd_pub;
// 键盘输入value与真实按键含义的对应map
std::map<int, std::string> g_keyboard_map = {
    {82, "0"}, {83, "."}, {96, "ENTER"},
    {79, "1"}, {80, "2"}, {81, "3"},
    {75, "4"}, {76, "5"}, {77, "6"},{14, "BackSpace"}, 
    {71, "7"}, {72, "8"},{73, "9"}, {78, "+"},
    {69, "NumLock"}, {98, "/"}, {55, "*"}, {74, "-"}
};


//////////////////////////////////////
//// main ////////////////////////////
int main(int argc, char* argv[])
{
    // - create interactor node
    Node node("interactor");

    // - publish topic_button_down
    g_btn_pub = node.Advertise<std_msgs::Int8>("topic_button_down", 1);

    // - publish topic_voice
    g_snd_pub = node.Advertise<std_msgs::String>("topic_voice", 1);

    // - publish message
    Rate rate(10);

    // - keyboard listener
    std::string keyboardPath = "/dev/input/event3";
    int tryConnCnt = 0;
    char ret[2];
    struct input_event t;
    int keys_fd = open(keyboardPath.c_str(), O_RDONLY);
    while(keys_fd <= 0)
    {
        LOG_ERROR("[interactor] key board " << keyboardPath <<" open failed");
        if(++tryConnCnt > 10)
        {
            // - play connect failed voice
            std_msgs::String losMsg;
            losMsg.data = std::string("lose_bluetooth");
            g_snd_pub->Publish(losMsg);
            tryConnCnt = 0;
        }
        // - wait a little moment
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        keys_fd = open(keyboardPath.c_str(), O_RDONLY);
    }
    LOG_INFO("[interactor] key board " << keyboardPath <<" opened!");

    // - play connect voice
    std_msgs::String connMsg;
    connMsg.data = std::string("connect_bluetooth");
    g_snd_pub->Publish(connMsg);

    // - notify core
    node.SetParam("start_interactor", true);

    while(Ok())
    {
        read(keys_fd, &t, sizeof(struct input_event));
        if(t.type==1 && t.value == 0)
        {
            LOG_INFO("[interactor] key " << t.code << ", state " << t.value);
            std_msgs::Int8 msg;
            msg.data = t.code;
            g_btn_pub->Publish(msg);
        }
        rate.Sleep();
    }
    close(keys_fd);

    // - notify core
    node.SetParam("start_interactor", false);

    return 0;
}


