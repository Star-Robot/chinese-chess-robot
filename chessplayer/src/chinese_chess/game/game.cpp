#include <iostream>
#include <exception>
#include <vector>
#include <thread>
#include <memory>
#include <chrono>
#include <yaml-cpp/yaml.h>

#include "base/node.hpp"
#include "base/publisher.hpp"
#include "base/subscriber.hpp"
#include "message/std_msgs.hpp"
#include "common/json.hpp"

using namespace huleibao;

//////////////////////////////////////
//// funcions decl////////////////////
void OnButtonDown(std_msgs::Int8::ConstPtr msg);
void OnDifference(std_msgs::String::ConstPtr msg);
void OnGameover(std_msgs::String::ConstPtr msg);

//////////////////////////////////////
//// global vars//////////////////////
std::shared_ptr<Node> g_node;
Subscriber<std_msgs::Int8>::Ptr   g_sub_btn;
Subscriber<std_msgs::String>::Ptr g_sub_dif;
Subscriber<std_msgs::String>::Ptr g_sub_end;
Publisher<std_msgs::Int8>::Ptr    g_pub_capt;
Publisher<std_msgs::Int8>::Ptr    g_pub_open;
Publisher<std_msgs::Int32>::Ptr   g_pub_ndnm;
Publisher<std_msgs::String>::Ptr  g_pub_humv;
Publisher<std_msgs::String>::Ptr  g_pub_snd;

std::map<char,std::string> g_ucci_to_rearrange
{
    {'R',      "game_rearrange_red_ju"},
    {'P',    "game_rearrange_red_bing"},
    {'B',   "game_rearrange_red_xiang"},
    {'p',    "game_rearrange_black_zu"},
    {'C',     "game_rearrange_red_pao"},
    {'N',      "game_rearrange_red_ma"},
    {'n',    "game_rearrange_black_ma"},
    {'A',     "game_rearrange_red_shi"},
    {'K',   "game_rearrange_red_shuai"},
    {'c',   "game_rearrange_black_pao"},
    {'b', "game_rearrange_black_xiang"},
    {'a',   "game_rearrange_black_shi"},
    {'k', "game_rearrange_black_jiang"},
    {'r',    "game_rearrange_black_ju"}
};


//////////////////////////////////////
//// main ////////////////////////////
int main(int argc, char* argv[])
{
    // - create game node
    g_node.reset(new Node("game"));

    std::string cfgFile = "./resource/game_config.yml";
    YAML::Node config = YAML::LoadFile(cfgFile);

    // - parse config
    YAML::Node mebCfg = config["game_config"]["member"];

    // - set status: starting
    g_node->SetParam("game_status", "starting");
    // - lock game, ignore interactor
    g_node->SetParam("game_lock", true);

    // - publish topic_voice
    g_pub_snd = g_node->Advertise<std_msgs::String>("topic_voice", 1);

    // - play starting voice & level select voice
    std_msgs::String startMsg;
    startMsg.data = "game_starting";
    g_pub_snd->Publish(startMsg);

    LOG_INFO("[game] checking node's starting ...");
    bool ready = false;
    while(!ready)
    {
        ready = true;
        std::string prefix = "start_";
        // - check everyone's status
        for(auto memNm : mebCfg)
        {
            bool memStart = g_node->GetParam<bool>(prefix + memNm.as<std::string>(), false);
            LOG_INFO(prefix + memNm.as<std::string>() << " ? " << int(memStart));
            ready &= memStart;
        }
        // - wait a little moment
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOG_INFO("[game] main nodes start done");

    // - publish topic_scan
    g_pub_capt = g_node->Advertise<std_msgs::Int8>("topic_scan", 2);

    // - publish topic_human_move 
    g_pub_humv = g_node->Advertise<std_msgs::String>("topic_human_move", 1);

    // - subscrib topic_button_down
    g_sub_btn = g_node->Subscribe<std_msgs::Int8>("topic_button_down", 1, OnButtonDown);

    // - subscrib topic_difference
    g_sub_dif = g_node->Subscribe<std_msgs::String>("topic_difference", 1, OnDifference);

    // - subscrib topic_gameover
    g_sub_end = g_node->Subscribe<std_msgs::String>("topic_gameover", 1, OnGameover);

    // - publish topic_ai_level
    g_pub_ndnm = g_node->Advertise<std_msgs::Int32>("topic_ai_level", 1);

    LOG_INFO("[game] every node started, unlock the game");

    // - publish topic_game_opening
    g_pub_open = g_node->Advertise<std_msgs::Int8>("topic_game_opening", 1);
    std_msgs::Int8 openMsg;
    g_pub_open->Publish(openMsg);

    // - set status: opening
    g_node->SetParam("game_status", "opening");

    // - unlock game, waiting for interactor
    g_node->SetParam("game_lock", false);

    // - play fighting voice
    std_msgs::String fightMsg;
    fightMsg.data = "game_fighting";
    g_pub_snd->Publish(fightMsg);

    // - wait signal
    Spin();

    return 0;
}


//////////////////////////////////////
//// function define//////////////////

void OnButtonDown(std_msgs::Int8::ConstPtr msg)
{
    // - check game lock
    bool bGameLocked = g_node->GetParam<bool>("game_lock", true);
    LOG_INFO("[game] game_locked ? " << int(bGameLocked));
    if(bGameLocked) 
    {
        // - play locking voice
        std_msgs::String lockMsg;
        lockMsg.data = "game_locking";
        g_pub_snd->Publish(lockMsg);
        return;
    }
    int keyCode = msg->data;
    /* 
    * 79(1)
    * 80(2)
    * 81(3)
    * 75(4)
    * 76(5)
    * 77(6)
    * 96(ENTER)
    */
    // - waitting for nums above
    if( keyCode == 79 || 
        keyCode == 80 || 
        keyCode == 81 || 
        keyCode == 75 || 
        keyCode == 76 || 
        keyCode == 77)
    {
        // - To modify AI skill level
        std::map<int, int> keyToLv = {
            {79, 1},
            {80, 2},
            {81, 3},
            {75, 4},
            {76, 5},
            {77, 6}
        };
        std::map<int, std::string> key2LvVoice = {
            {79, "game_beginner_level"},
            {80, "game_amateur_level"},
            {81, "game_professional_level"},
            {75, "game_master_level"},
            {76, "game_supermaster_level"},
            {77, "game_godlike_level"}
        };
        // - publish to UCCI-Engine set ai-level 
        std_msgs::Int32 lvMsg;
        lvMsg.data = keyToLv[keyCode];
        g_pub_ndnm->Publish(lvMsg);

        // - publish level voice
        std_msgs::String levelMsg;
        levelMsg.data = key2LvVoice[keyCode];
        g_pub_snd->Publish(levelMsg);
        return; 
    }

    // - then waitting for 'Enter' only
    if(keyCode != 96) return;

    // - then lock the game
    g_node->SetParam("game_lock", true);
    // - Notify camera to capture image, activate detector, chess ...
    LOG_INFO("[game] publish topic_scan");
    g_pub_capt->Publish(*msg);

    // - play scanning voice
    std_msgs::String scanMsg;
    scanMsg.data = "game_scanning";
    g_pub_snd->Publish(scanMsg);
}


void OnDifference(std_msgs::String::ConstPtr msg)
{
    LOG_INFO("[game] OnDifference");
    // - check game status
    std::string gameStatus = g_node->GetParam<std::string>("game_status", "starting");
    LOG_INFO("[game] game_status= " << gameStatus);

    // - parse difference format:
    /// "losts" :    [ [int(chrc),grid_x,grid_y], [int(chrc),grid_x,grid_y],...]
    /// "newcomers": [ [int(chrc),grid_x,grid_y], [int(chrc),grid_x,grid_y],...]
    std::string& contents  = msg->data;
    nlohmann::json detDict = nlohmann::json::parse(contents);
    nlohmann::json& losts  = detDict.at("losts");
    nlohmann::json& ncoms  = detDict.at("newcomers");
    bool reasonable = detDict.at("reasonable");

    int numLosts = losts.size();
    int numNcoms = ncoms.size();
    if(gameStatus == "opening")
    {
        if(numLosts == 0 && numNcoms == 0)
        {
            /// Status meets opening conditions, let's start the game
            g_node->SetParam("game_status", "fighting");
            g_node->SetParam("game_round", "human");
            g_node->SetParam("game_lock", false);
            LOG_INFO("[game] Let's start the game now, round: human!");
            LOG_INFO("[game] Now, human make the move first please.");
            // - play hunman_round voice
            std_msgs::String selvMsg;
            selvMsg.data = "game_select_level";
            g_pub_snd->Publish(selvMsg);
        }
        else
        {
            g_node->SetParam("game_lock", false);
            LOG_ERROR("[game] The chess pieces are not arranged properly, please rearrange them!");
            // - get all pieces not arranged 
            std::set<int> piece;
            for(auto& l : losts) piece.insert(int(l[0]));
            for(auto& n : ncoms) piece.insert(int(n[0]));
            for(int uid : piece)
            {
                // - play game_rearrange_x voice
                std_msgs::String rearrangeXMsg;
                rearrangeXMsg.data = g_ucci_to_rearrange[char(uid)];
                g_pub_snd->Publish(rearrangeXMsg);
            }
            // - play game_rearrange voice
            std_msgs::String rearrangeMsg;
            rearrangeMsg.data = "game_rearrange";
            g_pub_snd->Publish(rearrangeMsg);
        }
    }
    else if(gameStatus == "fighting")
    {
        std::string player = g_node->GetParam<std::string>("game_round", "human");
        if(player == "human")
        {
            if(numLosts == 0 && numNcoms == 0)
            {
                LOG_WARN("[game] No action! Human make the move now.");
                g_node->SetParam("game_lock", false);
                // - play hunman_round voice
                std_msgs::String humanRoundMsg;
                humanRoundMsg.data = "game_human_round";
                g_pub_snd->Publish(humanRoundMsg);
            }
            else if(!reasonable)
            {
                LOG_ERROR("[game] The action violated the rules! Human remake the move.");
                g_node->SetParam("game_lock", false);
                // - play hunman_illgal voice
                std_msgs::String humanIllMsg;
                humanIllMsg.data = "game_human_illegal";
                g_pub_snd->Publish(humanIllMsg);
            }
            else
            {
                /// The human has moved, now it’s the robot’s turn
                LOG_INFO("[game] Now, it's the robot's turn");
                g_node->SetParam("game_round", "robot");
                g_pub_humv->Publish(*msg);
            }
        }
        else if(player == "robot")
        {
            if(numLosts == 0 && numNcoms == 0)
            {
                /// The robot has moved, now it’s the human’s turn
                LOG_INFO("[game] Robot moved done! Now it's the human's turn");
                g_node->SetParam("game_lock", false);
                g_node->SetParam("game_round", "human");
                // - play hunman_round voice
                std_msgs::String humanRoundMsg;
                humanRoundMsg.data = "game_human_round";
                g_pub_snd->Publish(humanRoundMsg);
            }
            else
            {
                LOG_ERROR("[game] Failure found: Robot did not act as planned.");
                LOG_ERROR("[game] If this message is printed, it means that the machine is not working well.");
                LOG_ERROR("[game] Please contact the maintenance personnel.");
                // - play system_failure voice
                std_msgs::String sysFailMsg;
                sysFailMsg.data = "game_system_failure";
                g_pub_snd->Publish(sysFailMsg);

                // - publish game opening
                std_msgs::Int8 openMsg;
                g_pub_open->Publish(openMsg);

                // - unlock game, waiting for interactor
                g_node->SetParam("game_lock", false);
                // - set status: opening
                g_node->SetParam("game_status", "opening");
                // - play game_rearrange voice
                std_msgs::String rearrangeMsg;
                rearrangeMsg.data = "game_rearrange";
                g_pub_snd->Publish(rearrangeMsg);
            }
        }
    }
    else
    {
        LOG_ERROR("[game] Failure found: Robot did not act as planned.");
        LOG_ERROR("[game] If this message is printed, it means that the machine is not working well.");
        LOG_ERROR("[game] Please contact the maintenance personnel.");
        // - play system_failure voice
        std_msgs::String sysFailMsg;
        sysFailMsg.data = "game_system_failure";
        g_pub_snd->Publish(sysFailMsg);

        // - publish game opening
        std_msgs::Int8 openMsg;
        g_pub_open->Publish(openMsg);

        // - unlock game, waiting for interactor
        g_node->SetParam("game_lock", false);
        // - set status: opening
        g_node->SetParam("game_status", "opening");
        // - play game_rearrange voice
        std_msgs::String rearrangeMsg;
        rearrangeMsg.data = "game_rearrange";
        g_pub_snd->Publish(rearrangeMsg);
    }
}


void OnGameover(std_msgs::String::ConstPtr msg)
{
    std::string& contents = msg->data;
    // - play game finished voice
    std_msgs::String gameOverMsg;
    if(contents == "resign")
    {
        gameOverMsg.data = "game_human_win";
        LOG_WARN("[game] Congratulations, you have won!");
    }
    else if(contents == "win")
    {
        gameOverMsg.data = "game_robot_win";
        LOG_WARN("[game] Sorry, you lost this game!");
    }
    else
    {
        LOG_FATAL("[game] Something unexpected happened!");
    }
    g_pub_snd->Publish(gameOverMsg);

    LOG_WARN("[game] Game Over!");
    // - set status: starting
    g_node->SetParam("game_status", "starting");
}
