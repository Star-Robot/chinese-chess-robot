#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <yaml-cpp/yaml.h>

#include "base/node.hpp"
#include "base/publisher.hpp"
#include "base/subscriber.hpp"
#include "message/std_msgs.hpp"

using namespace huleibao;

//////////////////////////////////////
//// funcions decl////////////////////
void OnImage(std_msgs::Image::ConstPtr msg);

//////////////////////////////////////
//// global vars//////////////////////
Subscriber<std_msgs::Image>::Ptr g_sub;
Publisher<std_msgs::Image>::Ptr g_pub;
int   g_grab_times      = 4;
float g_grab_elaps      = 0.5;
float g_viewboard_h     = 0;
float g_viewboard_w     = 0;
float g_markerboard_h   = 0;
float g_markerboard_w   = 0;
float visScale          = 0;

cv::Size g_vis_size(0, 0);
std::vector<cv::Point2f> g_dest_points;

cv::Mat g_camera_k;
cv::Mat g_camera_d;
cv::Mat g_homography;

//////////////////////////////////////
//// main ////////////////////////////
int main(int argc, char* argv[])
{
    // - create calibrator node
    Node node("calibrator");

    // - notify the game master
    node.SetParam("start_calibrator", false);

    // - get configs
    std::string cfgFile = "./resource/game_config.yml";
    YAML::Node config = YAML::LoadFile(cfgFile);

    // - parse config
    YAML::Node camCfg = config["game_config"]["camera"];
    YAML::Node chessCfg = config["game_config"]["chess"];
    g_grab_times = camCfg["grab_times"].as<int>();
    std::vector<double> intrinsic = camCfg["intrinsic"].as<std::vector<double>>();
    std::vector<double> distortion = camCfg["distortion"].as<std::vector<double>>();

    g_viewboard_h = chessCfg["viewboard_height"].as<float>();
    g_viewboard_w = chessCfg["viewboard_width"].as<float>();
    g_markerboard_h = chessCfg["markerboard_height"].as<float>();
    g_markerboard_w = chessCfg["markerboard_width"].as<float>();
    visScale = chessCfg["vis_affined_scale"].as<float>();

    float offX = (g_viewboard_w - g_markerboard_w) / 2.0;
    float offY = (g_viewboard_h - g_markerboard_h) / 2.0;

    // - MarkerID必须按4,1,3,2对应上,即[↖↗↙↘]->偏移放大
    g_dest_points = std::vector<cv::Point2f>
    {
        { visScale*(0 + offX), visScale*(0 + offY)},
        { visScale*(g_markerboard_w + offX), visScale * (0 + offY) },
        { visScale*(0 + offX), visScale * (g_markerboard_h + offY) },
        { visScale*(g_markerboard_w + offX), visScale * (g_markerboard_h + offY) }
    };

    // - camera K, D
    g_camera_k = cv::Mat(3, 3, CV_64FC1, intrinsic.data());
    g_camera_d = cv::Mat(5, 1, CV_64FC1, distortion.data());

    // - view image size, will input to net
    g_vis_size = cv::Size(g_viewboard_w * visScale, g_viewboard_h * visScale);

    // - publish topic_image
    g_pub = node.Advertise<std_msgs::Image>("topic_roi", g_grab_times);

    // - subscrib topic_scan
    g_sub = node.Subscribe<std_msgs::Image>("topic_image", g_grab_times, OnImage);

    // - notify the game master
    node.SetParam("start_calibrator", true);

    // - wait signal
    Spin();

    // - notify the game master
    node.SetParam("start_calibrator", false);

    return 0;
}


//////////////////////////////////////
//// function define//////////////////

void OnImage(std_msgs::Image::ConstPtr msg)
{
    LOG_INFO("[calibrator] OnImage");
    // - decode image data and undistort
    int imH = msg->data.image_height;
    int imW = msg->data.image_width;
    std::vector<uint8_t>& imData = msg->data.image_data;
    cv::Mat img(imH, imW, CV_8UC3, imData.data()), undistImg;
    cv::imwrite("origin.jpg", img);
    // cv::undistort(img, undistImg, g_camera_k, g_camera_d, g_camera_k);
    // cv::imwrite("undistort.jpg", undistImg);
    undistImg = img;
    // - then detect aruco
    std::vector<int> markerIds;
    std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
    cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
    cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
    cv::aruco::detectMarkers(undistImg, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);
    cv::Mat outputImage = undistImg.clone();
    cv::aruco::drawDetectedMarkers(outputImage, markerCorners, markerIds);
    cv::imwrite("draw_marker.jpg", outputImage);
    LOG_INFO("[calibrator] detected marker num=" << markerIds.size());
    // 4 marker detected

    /*
    * Marker postion as:
    * 4---1
      |   |
    * 3---2
    */
    if (markerIds.size() != 4 && g_homography.empty())
    {
        std_msgs::Image imMsg;
        g_pub->Publish(imMsg);
	    return;
    }
    else if(g_homography.empty())
    {
        std::map<int, cv::Point2f> mid2center;
        for (int i = 0; i != 4; ++i)
        {
            // - caculate corner centor
            std::vector<cv::Point2f> cors = markerCorners[i];
            float cx = 0.f, cy = 0.f;
            for (int j = 0; j != 4; j++)
            {
                cx += cors[j].x;
                cy += cors[j].y;
            }
            cx /= 4.f, cy /= 4.f;

            int mId = markerIds[i];
            mid2center[mId] = cv::Point2f(cx, cy);
        }
        std::vector<cv::Point2f> sorcPoints =
        {
            mid2center[4],
            mid2center[1],
            mid2center[3],
            mid2center[2]
        };
        // - then do calibrate
        cv::Mat homo = cv::findHomography(sorcPoints, g_dest_points);
        g_homography = homo.clone();
    }
    cv::Mat affImg;
    cv::warpPerspective(undistImg, affImg, g_homography, g_vis_size);
    // - at latest, publish this affImg
    std_msgs::Image imMsg;
    imMsg.data.image_height = affImg.rows;
    imMsg.data.image_width  = affImg.cols;
    imMsg.data.image_format = ImageData::Format::BGR;
    imMsg.data.image_data.assign(affImg.datastart, affImg.dataend);
    g_pub->Publish(imMsg);
}
