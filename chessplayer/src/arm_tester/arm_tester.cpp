#include <fstream>
#include <yaml-cpp/yaml.h>
#include <opencv2/opencv.hpp>

#include "common/timer.hpp"
#include "base/node.hpp"
#include "base/publisher.hpp"
#include "base/subscriber.hpp"
#include "message/std_msgs.hpp"
#include "common/arm_tools/arm_controller.hpp"

using namespace huleibao;

//////////////////////////////////////
//// funcions decl////////////////////
void InitArmGridTransform(std::string& calib_file);
void OnButtonDown(std_msgs::Int8::ConstPtr msg);
void Profile(int key_code);
typedef std::pair<float, float> XY;
XY ConvertGridToArmCoord(int grid_x, int grid_y);

//////////////////////////////////////
//// global vars//////////////////////
std::vector<XY>                     g_arm_xys;
Subscriber<std_msgs::Int8>::Ptr     g_sub;
Publisher<std_msgs::String>::Ptr    g_pub;
std::shared_ptr<ArmController>      g_arm_ctrl;
std::vector<std::vector<float> >    g_homo_mat;
std::vector<std::vector<XY> >       g_grid_xys;
XY                                  g_pre_grid_xy;
XY                                  g_grid_tl_xy;
XY                                  g_grid_br_xy;

//////////////////////////////////////
//// main ////////////////////////////
int main(int argc, char* argv[])
{
    // - create arm_tester node
    Node node("arm_tester");
    
    // - creat arm ctrl
    g_arm_ctrl.reset(new ArmController);

    // - publish topic_voice
    g_pub = node.Advertise<std_msgs::String>("topic_voice", 1);

    // - subscrib topic_button_down
    g_sub = node.Subscribe<std_msgs::Int8>("topic_button_down", 1, OnButtonDown);

    // - notify core
    node.SetParam("start_arm_tester", true);

    // - get configs
    std::string resrc = "./resource/";
    std::string cfgFile = resrc + "game_config.yml";
    YAML::Node config = YAML::LoadFile(cfgFile);
    std::string calibFile = resrc + config["game_config"]["arm"]["calib"].as<std::string>();
    // - init arm's xy transform
    InitArmGridTransform(calibFile);

    // - wait signal
    Spin();

    // - notify core
    node.SetParam("start_arm_tester", false);

    return 0;
}


//////////////////////////////////////
//// function define//////////////////
void InitArmGridTransform(std::string& calib_file)
{
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
        std_msgs::String initMsg;
        initMsg.data = std::string("arm_tester_load_success");
        g_pub->Publish(initMsg);

        // - init all grid-xy for test
        const int row = 10;
        const int col = 9;
        for(int r = 0; r < row; ++r)
        {
            g_grid_xys.push_back({});
            for(int c = -2; c < col + 2; ++c) 
                g_grid_xys.back().push_back({c, r});
        }
        g_pre_grid_xy = std::make_pair(0.f, 0.f);
        g_grid_tl_xy  = std::make_pair(-2.f, 0.f);
        g_grid_br_xy  = std::make_pair(10.f, 9.f);
    }
    catch(...)
    {
        // - play init voice
        std_msgs::String initMsg;
        initMsg.data = std::string("arm_tester_load_failed");
        g_pub->Publish(initMsg);
    }
}


void OnButtonDown(std_msgs::Int8::ConstPtr msg)
{
    LOG_INFO("[arm_tester] OnButtonDown: " << int(msg->data));
    int keyCode = msg->data;
    
    Profile(keyCode);
    /* 
    *   8 
    * 4   6
    *   2
    * 72(8)
    * 77(6)
    * 75(4)
    * 80(2)
    * 96(ENTER)
    */
    // - waitting for nums above only
    if( keyCode != 72 && 
        keyCode != 77 && 
        keyCode != 75 && 
        keyCode != 80 && 
        keyCode != 96 
    ) return;

    if(g_homo_mat.empty())
    {
        LOG_ERROR("[arm_tester] Failure found: arm hasn't been calibrated.");
        // - play arm uninit voice
        std_msgs::String initMsg;
        initMsg.data = std::string("arm_tester_load_failed");
        g_pub->Publish(initMsg);
        return;
    }

    static bool take = true;
    // - perfrom take and drop action 
    if(keyCode == 96)
    {
        if(take)
        {
            LOG_INFO("[arm_tester] perform Take action ...");
            g_arm_ctrl->TakePiece();
        }
        else
        {
            LOG_INFO("[arm_tester] perform Drop action ...");
            g_arm_ctrl->DropPiece();
        }
        LOG_INFO("[arm_tester] perform Hand reset action ...");
        // g_arm_ctrl->ResetHand();
        take = !take;
        return;
    }

    // - perform one by one grid move action 
    int curGX = g_pre_grid_xy.first;
    int curGY = g_pre_grid_xy.second;
    if(keyCode == 72)       --curGY;
    else if(keyCode == 77)  ++curGX;
    else if(keyCode == 75)  --curGX;
    else if(keyCode == 80)  ++curGY;

    // - 越界检查
    LOG_WARN("[arm_tester] go out of bound check ...");
    if( curGX < g_grid_tl_xy.first ||
        curGX > g_grid_br_xy.first ||
        curGY < g_grid_tl_xy.second ||
        curGY > g_grid_br_xy.second )
    {
        // - play outofbound voice
        std_msgs::String outMsg;
        outMsg.data = std::string("arm_tester_outofbound");
        g_pub->Publish(outMsg);
        LOG_WARN("[arm_tester] go out of bound");
        return;
    }
    // - play arm_action voice
    std_msgs::String actMsg;
    actMsg.data = std::string("arm_tester_action");
    g_pub->Publish(actMsg);
    // - drive robot-arm to take action
    XY armXY = ConvertGridToArmCoord(curGX, curGY);
    LOG_INFO("[arm_tester] grid(" << curGX << "," << curGY 
        << ", arm action(" << armXY.first << "," << armXY.second << ")");
    g_arm_ctrl->DriveByXY(armXY.first, armXY.second);
    g_pre_grid_xy.first = curGX;
    g_pre_grid_xy.second = curGY;
}


XY ConvertGridToArmCoord(int grid_x, int grid_y)
{
    float xx = g_homo_mat[0][0] * grid_x + g_homo_mat[0][1] * grid_y + g_homo_mat[0][2];
    float yy = g_homo_mat[1][0] * grid_x + g_homo_mat[1][1] * grid_y + g_homo_mat[1][2];
    float zz = g_homo_mat[2][0] * grid_x + g_homo_mat[2][1] * grid_y + g_homo_mat[2][2];
    return { xx/zz, yy/zz };
}



/**
性能测试
*/
void Profile(int key_code)
{
    
    // - waitting for nums above only
    if( key_code != 79 &&
        key_code != 82 &&
        key_code != 83
    ) return;

    XY tl = std::make_pair(0.f, 0.f);;
    XY tr = std::make_pair(8.f, 0.f);;
    XY br = std::make_pair(8.f, 9.f);;
    XY bl = std::make_pair(0.f, 9.f);;

    //// 1st. reset arm to tl
    // - play arm_action voice
    std_msgs::String actMsg;
    actMsg.data = std::string("arm_tester_action");
    g_pub->Publish(actMsg);

    /* 
    * 82, then go loop: 
    *   tl  ->  tr  ->  br   ->  bl   -> tl , grid(x,y)
    * (0,0)    (8,0)   (8,9)    (0,9)   (0,0)
    * 
    */
    if(key_code == 82) // '0'
    {
        Timer timer;
        for(int i = 0; i < 20; ++i)
        {
            // - drive robot-arm to take action
            XY armTlXY = ConvertGridToArmCoord(tl.first, tl.second);
            LOG_INFO("[arm_tester] grid(" << tl.first << "," << tl.second 
                << ", arm action(" << armTlXY.first << "," << armTlXY.second << ")");
            g_arm_ctrl->DriveByXY(armTlXY.first, armTlXY.second);
            LOG_INFO("[arm_tester/Profile] xx->tl action cost(ms) " << timer.Elapsed(true));

            // - drive robot-arm to take action
            XY armTrXY = ConvertGridToArmCoord(tr.first, tr.second);
            LOG_INFO("[arm_tester] grid(" << tr.first << "," << tr.second 
                << ", arm action(" << armTrXY.first << "," << armTrXY.second << ")");
            g_arm_ctrl->DriveByXY(armTrXY.first, armTrXY.second);
            LOG_INFO("[arm_tester/Profile] tl->tr action cost(ms) " << timer.Elapsed(true));

            // - drive robot-arm to take action
            XY armBrXY = ConvertGridToArmCoord(br.first, br.second);
            LOG_INFO("[arm_tester] grid(" << br.first << "," << br.second 
                << ", arm action(" << armBrXY.first << "," << armBrXY.second << ")");
            g_arm_ctrl->DriveByXY(armBrXY.first, armBrXY.second);
            LOG_INFO("[arm_tester/Profile] tr->br action cost(ms) " << timer.Elapsed(true));

            // - drive robot-arm to take action
            XY armBlXY = ConvertGridToArmCoord(bl.first, bl.second);
            LOG_INFO("[arm_tester] grid(" << bl.first << "," << bl.second 
                << ", arm action(" << armBlXY.first << "," << armBlXY.second << ")");
            g_arm_ctrl->DriveByXY(armBlXY.first, armBlXY.second);
            LOG_INFO("[arm_tester/Profile] bl->br action cost(ms) " << timer.Elapsed(true));

            g_pre_grid_xy = bl;
        }
    }
    else if(key_code == 79) // '1'
    {
        // - drive robot-arm to take action
        XY armTlXY = ConvertGridToArmCoord(tl.first, tl.second);
        LOG_INFO("[arm_tester] grid(" << tl.first << "," << tl.second 
            << ", arm action(" << armTlXY.first << "," << armTlXY.second << ")");
        g_arm_ctrl->DriveByXY(armTlXY.first, armTlXY.second);
        
        Timer timer;
        for(int i = 0; i < 1; ++i)
        {
            int xMin = tl.first;
            int yMin = tl.second;
            int xMax = br.first;
            int yMax = br.second;

            int r = yMin;
            while(true)
            {
                for(int c = xMin; c <= xMax + 1; ++c)
                {
                    // - drive robot-arm to take action
                    XY armXY = ConvertGridToArmCoord(c, r);
                    LOG_INFO("[arm_tester] grid(" << c << "," << r
                        << ", arm action(" << armXY.first << "," << armXY.second << ")");
                    g_arm_ctrl->DriveByXY(armXY.first, armXY.second);
                    LOG_INFO("[arm_tester/Profile] step action cost(ms) " << timer.Elapsed(true));
                    g_pre_grid_xy = std::make_pair(c, r);
                }
                if(++r > yMax) break;
                for(int c = xMax; c >= xMin - 1; --c)
                {
                    // - drive robot-arm to take action
                    XY armXY = ConvertGridToArmCoord(c, r);
                    LOG_INFO("[arm_tester] grid(" << c << "," << r
                        << ", arm action(" << armXY.first << "," << armXY.second << ")");
                    g_arm_ctrl->DriveByXY(armXY.first, armXY.second);
                    LOG_INFO("[arm_tester/Profile] step action cost(ms) " << timer.Elapsed(true));
                    g_pre_grid_xy = std::make_pair(c, r);
                }
                if(++r > yMax) break;
            }
        }
    }
    else if(key_code == 83) // '.'
    {
        static bool take = true;
        Timer timer;
        for(int i = 0; i < 10; ++i)
        {
            g_arm_ctrl->TakePiece();
            LOG_INFO("[arm_tester/Profile] Take action cost(ms) " << timer.Elapsed(true));
        
            g_arm_ctrl->ResetHand();
            LOG_INFO("[arm_tester/Profile] ResetHand   cost(ms) " << timer.Elapsed(true));

            g_arm_ctrl->DropPiece();
            LOG_INFO("[arm_tester/Profile] Drop action cost(ms) " << timer.Elapsed(true));
        }
    }

}