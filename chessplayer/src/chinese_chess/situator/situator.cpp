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
#include "chess_manager.hpp"
#include "common/json.hpp"

using namespace huleibao;

//////////////////////////////////////
//// funcions decl////////////////////
void OnScan(std_msgs::Int8::ConstPtr msg);
void OnGameOpenning(std_msgs::Int8::ConstPtr msg);
void OnSetAiLevel(std_msgs::Int32::ConstPtr msg);
void OnDetection(std_msgs::String::ConstPtr msg);
void OnHumanMove(std_msgs::String::ConstPtr msg);

//////////////////////////////////////
//// global vars//////////////////////
std::shared_ptr<Node> g_node;
Subscriber<std_msgs::String>::Ptr g_sub_detc;
Subscriber<std_msgs::String>::Ptr g_sub_humv;
Subscriber<std_msgs::Int8>::Ptr   g_sub_capt;
Subscriber<std_msgs::Int8>::Ptr   g_sub_open;
Subscriber<std_msgs::Int32>::Ptr  g_sub_ailv;
Publisher<std_msgs::String>::Ptr  g_pub_diff;
Publisher<std_msgs::String>::Ptr  g_pub_arms;
Publisher<std_msgs::String>::Ptr  g_pub_over;
Publisher<std_msgs::String>::Ptr  g_pub_snd;
std::shared_ptr<ChessManager>     g_chess_mnger;
YAML::Node g_chess_cfg;
bool g_robot_win  = false;
bool g_robot_mate = false;
int g_call_times  = 0;
int g_grab_times  = 4;

//////////////////////////////////////
//// main ////////////////////////////
int main(int argc, char* argv[])
{
    // - create situator node
    g_node.reset(new Node("situator"));

    // - notify the game master
    g_node->SetParam("start_situator", false);

    // - get configs
    std::string resrc = "./resource/";
    std::string cfgFile = resrc + "game_config.yml";
    YAML::Node config = YAML::LoadFile(cfgFile);

    // - parse config
    g_chess_cfg = config["game_config"]["chess"];
    g_grab_times = config["game_config"]["camera"]["grab_times"].as<int>();

    // - subscrib topic_detection 
    g_sub_detc = g_node->Subscribe<std_msgs::String>("topic_detection", g_grab_times, OnDetection);

    // - subscrib topic_human_move
    g_sub_humv = g_node->Subscribe<std_msgs::String>("topic_human_move", 1, OnHumanMove);

    // - subscrib topic_scan
    g_sub_capt = g_node->Subscribe<std_msgs::Int8>("topic_scan", 1, OnScan);

    // - subscrib topic_game_opening 
    g_sub_open = g_node->Subscribe<std_msgs::Int8>("topic_game_opening", 1, OnGameOpenning);

    // - subscrib topic_ai_level 
    g_sub_ailv = g_node->Subscribe<std_msgs::Int32>("topic_ai_level", 1, OnSetAiLevel);

    // - publish topic_difference 
    g_pub_diff = g_node->Advertise<std_msgs::String>("topic_difference", 1);

    // - publish topic_arm
    g_pub_arms = g_node->Advertise<std_msgs::String>("topic_arm", 1);

    // - publish topic_gameover
    g_pub_over = g_node->Advertise<std_msgs::String>("topic_gameover", 1);

    // - publish topic_voice
    g_pub_snd = g_node->Advertise<std_msgs::String>("topic_voice", 1);

    // - notify the game master
    g_node->SetParam("start_situator", true);

    // - wait signal
    Spin();

    // - notify the game master
    g_node->SetParam("start_situator", false);

    return 0;
}


//////////////////////////////////////
//// function define//////////////////
void OnScan(std_msgs::Int8::ConstPtr msg)
{
    LOG_INFO("[situator] OnScan");
    g_call_times = 0;
    if(g_chess_mnger == nullptr)
    {
        LOG_FATAL("[situator] ChessManager instance has not been created.");
    }
    // - clear situator status, prepare for next update
    g_chess_mnger->Prepare();
    LOG_INFO("[situator] clear situator status, prepare for next update");
}


void OnGameOpenning(std_msgs::Int8::ConstPtr msg)
{
    LOG_INFO("[situator] OnGameOpenning");
    // - create ChessManager obj
    g_chess_mnger.reset(new ChessManager(g_chess_cfg));
    LOG_INFO("[situator] create ChessManager instance");
}


void OnSetAiLevel(std_msgs::Int32::ConstPtr msg)
{
    LOG_INFO("[situator] OnSetAiLevel");
    if(g_chess_mnger == nullptr)
    {
        LOG_FATAL("[situator] ChessManager instance has not been created.");
    }
    // - set go nodes 
    g_chess_mnger->SetAiLevel(msg->data);
    LOG_INFO("[situator] set AI level " << msg->data);
}


// - Each capture-detecions will call OnDetection N-times
void OnDetection(std_msgs::String::ConstPtr msg)
{
    LOG_INFO("[situator] OnDetection");
    // - increase call times
    ++g_call_times;
    // - if robot win previous, publish it
    if(g_robot_win)
    {
        std_msgs::String endMsg;
        endMsg.data = "win";
        g_pub_over->Publish(endMsg);
        return;
    }
    // - if robot mate previous, play robotmate voice
    if(g_robot_mate)
    {
        std_msgs::String mateMsg;
        mateMsg.data = "game_robotmate";
        g_pub_snd->Publish(mateMsg);
        g_robot_mate = false;
    }
    // - parse detout msg, detout format:
    /// "coords"  : [[x,y],[x,y],...]
    /// "classes" : ["red_ju","black_jiang",...]
    std::string& contents = msg->data;
    nlohmann::json detDict  = nlohmann::json::parse(contents);
    nlohmann::json& coords  = detDict.at("coords");
    nlohmann::json& classes = detDict.at("classes");
    if(coords.size() != classes.size())
    {
        LOG_ERROR("[situator] coords.size != classes.size, vs "
            << coords.size() <<" != " <<  classes.size());
        return;
    }

    // - update situator' status
    int len = coords.size();
    for(int i = 0; i != len; ++i)
    {
        float predX = coords[i][0];
        float predY = coords[i][1];
        std::string predC = classes[i];
        /// if not matched
        g_chess_mnger->Read(predX, predY, predC);
    }

    // - !Note, return if call times not reach grab times
    if(g_call_times < g_grab_times) return;

    LOG_INFO("[situator] g_call_times=" << g_call_times << ", g_grab_times=" << g_grab_times); 
    // - get player round: human or robot
    std::string round = g_node->GetParam<std::string>("game_round","");
    // Summarize the difference and judge whether the changes are reasonable
    std::string summary = std::move(g_chess_mnger->Summary(round));
    LOG_INFO("[situator] summary=" << summary);
    // - publish summary info
    std_msgs::String smryMsg;
    smryMsg.data = std::move(summary);
    g_pub_diff->Publish(smryMsg);
}


// - Obtain the behavior of human players and request AI strategies
void OnHumanMove(std_msgs::String::ConstPtr msg)
{
    LOG_INFO("[situator] OnHumanMove");
    // - parse difference format:
    /// "losts" :    [ [int(chrc),grid_x,grid_y], [int(chrc),grid_x,grid_y],...]
    /// "newcomers": [ [int(chrc),grid_x,grid_y], [int(chrc),grid_x,grid_y],...]
    std::string& contents  = msg->data;
    LOG_INFO("[situator] msg=" << contents);
    nlohmann::json detDict = nlohmann::json::parse(contents);
    nlohmann::json& losts  = detDict.at("losts");
    nlohmann::json& ncoms  = detDict.at("newcomers");
    bool reasonable = detDict.at("reasonable");

    int numLosts = losts.size();
    int numNcoms = ncoms.size();

    LOG_INFO("[situator] losts[0] gx,gy = " << losts[0][1] << ", " << losts[0][2]);
    char ucciX0, ucciY0;
    char ucciX1 = g_chess_mnger->CvtGridXToUcci(ncoms[0][1]);
    char ucciY1 = g_chess_mnger->CvtGridYToUcci(ncoms[0][2]);
    LOG_INFO("[situator] losts[0] x,y = " << ucciX1 << ", " << ucciY1);
    if(losts[0][0] == ncoms[0][0])
    {
        /// losts[0]->numNcoms[0]
        ucciX0 = g_chess_mnger->CvtGridXToUcci(losts[0][1]);
        ucciY0 = g_chess_mnger->CvtGridYToUcci(losts[0][2]);
    }
    else
    {
        /// losts[1]->numNcoms[0]
        ucciX0 = g_chess_mnger->CvtGridXToUcci(losts[1][1]);
        ucciY0 = g_chess_mnger->CvtGridYToUcci(losts[1][2]);
    }
    std::string humanMove = {ucciX0, ucciY0, ucciX1, ucciY1};
    LOG_INFO("[situator] humanMove=" << humanMove);

    // - format,eg:{resign=false, bestmove={{grid_x0,grid_y0}},{grid_x1,grid_y1}}
    std::string answer = g_chess_mnger->UcciAnswer(humanMove);
    LOG_INFO("[situator] answer.json=" << answer);
    nlohmann::json ansDict = nlohmann::json::parse(answer);
    bool resign = ansDict.at("resign");
    bool win    = ansDict.at("win");
    std::vector<std::vector<int>> bestmove = ansDict.at("bestmove");
    // - if AI resign or win, it means game over!
    if(resign)
    {
        std_msgs::String endMsg;
        endMsg.data = "resign";
        g_pub_over->Publish(endMsg);
    }
    // - AI give the bestmove
    else if(bestmove != std::vector<std::vector<int>>({{-1,-1},{-1,-1}}))
    {
        std_msgs::String armMsg;
        // ansDict["type"] = numLosts == 1 ? "move" : "eat";
        armMsg.data = std::move(std::string(ansDict.dump()));
        LOG_INFO("[situator] Publish armMsg=" << armMsg.data);
        g_pub_arms->Publish(armMsg);
    }

    // - Record win or not
    g_robot_win = win;
    // - Record mate or not
    g_robot_mate = ansDict.at("mate");
}
