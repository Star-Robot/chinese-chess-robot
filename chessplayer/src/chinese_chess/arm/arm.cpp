#include <fstream>
#include <yaml-cpp/yaml.h>
#include <opencv2/opencv.hpp>

#include "base/node.hpp"
#include "base/publisher.hpp"
#include "base/subscriber.hpp"
#include "message/std_msgs.hpp"
#include "common/json.hpp"
#include "common/arm_tools/arm_controller.hpp"

using namespace huleibao;

//////////////////////////////////////
//// funcions decl////////////////////
int InitArmGridTransform(std::string& calib_file);
void OnArm(std_msgs::String::ConstPtr msg);
void ActMove(std::vector<float>& begArmXY, std::vector<float>& endArmXY);
std::vector<int> GetNextRecycleGridXY();
std::vector<float> ConvertGridToArmCoord(int grid_x, int grid_y);

//////////////////////////////////////
//// global vars//////////////////////
Publisher<std_msgs::Int8>::Ptr      g_pub_capt;
Publisher<std_msgs::String>::Ptr    g_pub_snd;
Subscriber<std_msgs::String>::Ptr   g_sub_armc;
std::shared_ptr<ArmController>      g_arm_ctrl;
std::vector<std::vector<int> >      g_recycle_grid_xys;
std::vector<std::vector<float> >    g_homo_mat;

//////////////////////////////////////
//// main ////////////////////////////
int main(int argc, char* argv[])
{
    // - create arm node
    Node node("arm");
    
    // - notify core
    node.SetParam("start_arm", false);

    // - creat arm ctrl
    LOG_INFO("ArmController init ...");
    g_arm_ctrl.reset(new ArmController);
    LOG_INFO("ArmController init done.");

    // - init robot's recycle coords(in Grid format)
    g_recycle_grid_xys.assign({
        {-1,1},{-2,1},
        {-1,2},{-2,2},
        {-1,3},{-2,3},
        {-1,4},{-2,4},
        {-1,5},{-2,5},
        {-1,6},{-2,6},
        {-1,7},{-2,7},
        {-1,8},{-2,8},
    });
   
    // - subscribe topic_arm
    g_sub_armc = node.Subscribe<std_msgs::String>("topic_arm", 1, OnArm);

    // - publish topic_scan
    g_pub_capt = node.Advertise<std_msgs::Int8>("topic_scan", 1);

    // - publish topic_voice
    g_pub_snd = node.Advertise<std_msgs::String>("topic_voice", 1);
 
    // - get configs
    std::string resrc = "./resource/";
    std::string cfgFile = resrc + "game_config.yml";
    YAML::Node config = YAML::LoadFile(cfgFile);
    std::string calibFile = resrc + config["game_config"]["arm"]["calib"].as<std::string>();
    // - init arm's xy transform
    int ret = InitArmGridTransform(calibFile);
    if(ret != 0) return -1;

    // - notify core
    node.SetParam("start_arm", true);

    // - wait signal
    Spin();

    // - notify core
    node.SetParam("start_arm", false);

    return 0;
}


//////////////////////////////////////
//// function define//////////////////

int InitArmGridTransform(std::string& calib_file)
{
    typedef std::pair<float, float> XY;
    try
    {
        YAML::Node n = YAML::LoadFile(calib_file);
        // - load calib arm xy[↖↗↙↘]顺序
        XY tl = n["top_left"].as<XY>();
        XY tr = n["top_right"].as<XY>();
        XY bl = n["bottom_left"].as<XY>();
        XY br = n["bottom_right"].as<XY>();
        // - prepare dest xys
        std::vector<cv::Point2f> armXYs = 
        {
            {tl.first, tl.second},
            {tr.first, tr.second},
            {bl.first, bl.second},
            {br.first, br.second}
        }; 
        // - prepare source xys
        std::vector<cv::Point2f> gridXYs = 
        {
            {0.f, 0.f},
            {8.f, 0.f},
            {0.f, 9.f},
            {8.f, 9.f}
        };
        // - init transform from grid to arm coords
        cv::Mat homo = cv::findHomography(gridXYs, armXYs, cv::RANSAC); 
        std::vector<float> homoVec =  std::vector<float>(homo.reshape(1, 1));
        g_homo_mat.assign(
        {
            { homoVec[0], homoVec[1], homoVec[2] },
            { homoVec[3], homoVec[4], homoVec[5] },
            { homoVec[6], homoVec[7], homoVec[8] }
        });
        LOG_INFO("[arm_tester] init transform from grid to arm done.");
        // - play init voice
        // std_msgs::String sucMsg;
        // sucMsg.data = std::string("arm_tester_load_success");
        // g_pub_snd->Publish(sucMsg);
    }
    catch(...)
    {
        // - play init voice
        std_msgs::String initMsg;
        initMsg.data = std::string("arm_tester_load_failed");
        g_pub_snd->Publish(initMsg);
        return 1;
    }
    return 0;
}


void OnArm(std_msgs::String::ConstPtr msg)
{
    LOG_INFO("[Arm] OnArm");
    // - parse difference format:
    /// "type":      "move" or "eat"
    /// "bestmove":  [[grid_x,grid_y],[grid_x,grid_y]]
    /// "resign":    true or false 
    std::string& contents  = msg->data;
    LOG_INFO("[Arm] msg=" << contents);
    nlohmann::json detDict = nlohmann::json::parse(contents);
    bool resign  = detDict.at("resign");
    std::vector<std::vector<int>> bestmove = detDict.at("bestmove");
    std::string moveType = detDict.at("type");
    // - play move voice
    std_msgs::String moveMsg;
    moveMsg.data = std::string("game_move");
    g_pub_snd->Publish(moveMsg);
    // - drive robot-arm to take action
    std::vector<float> armBegXY = ConvertGridToArmCoord(bestmove[0][0],bestmove[0][1]);
    std::vector<float> armEndXY = ConvertGridToArmCoord(bestmove[1][0],bestmove[1][1]);
    if(moveType == "eat")
    {
        std::vector<int> rycGridXY = GetNextRecycleGridXY();
        std::vector<float> rycArmXY = ConvertGridToArmCoord(rycGridXY[0], rycGridXY[1]);
        ActMove(armEndXY, rycArmXY);
        g_arm_ctrl->ResetHand();
    }
    ActMove(armBegXY, armEndXY);

    // - Reset arm to init position
    g_arm_ctrl->Reset();

    LOG_INFO("[Arm] Robot arm action completed & publish topic_scan");
    // - Notify camera to capture image, to check if arm move as expected ...
    std_msgs::Int8 captMsg;
    g_pub_capt->Publish(captMsg);
    g_arm_ctrl->ResetHand();
}


void ActMove(std::vector<float>& begArmXY, std::vector<float>& endArmXY)
{
    LOG_INFO("[Arm] Robot arm action DriveByXY: " << begArmXY[0] << ", " << begArmXY[1]);
    g_arm_ctrl->DriveByXY(begArmXY[0], begArmXY[1]);
    // - Reset arm's hand to init position
    // g_arm_ctrl->ResetHand();
    LOG_INFO("[Arm] Robot arm TakePiece ...");
    g_arm_ctrl->TakePiece();
    LOG_INFO("[Arm] Robot arm action DriveByXY: " << endArmXY[0] << ", " << endArmXY[1]);
    g_arm_ctrl->DriveByXY(endArmXY[0], endArmXY[1]);
    LOG_INFO("[Arm] Robot arm DropPiece ...");
    g_arm_ctrl->DropPiece();
}


std::vector<int> GetNextRecycleGridXY()
{
    if(g_recycle_grid_xys.empty())
    {
        LOG_ERROR("[Arm] Failure found: No recycle position left!");
    }

    std::vector<int> rycXY =  g_recycle_grid_xys.back();
    g_recycle_grid_xys.pop_back();
    return rycXY; 
}


std::vector<float> ConvertGridToArmCoord(int grid_x, int grid_y)
{
    float xx = g_homo_mat[0][0] * grid_x + g_homo_mat[0][1] * grid_y + g_homo_mat[0][2];
    float yy = g_homo_mat[1][0] * grid_x + g_homo_mat[1][1] * grid_y + g_homo_mat[1][2];
    float zz = g_homo_mat[2][0] * grid_x + g_homo_mat[2][1] * grid_y + g_homo_mat[2][2];
    return { xx/zz, yy/zz };
}

