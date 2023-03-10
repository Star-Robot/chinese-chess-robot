#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <yaml-cpp/yaml.h>

#include "base/node.hpp"
#include "base/publisher.hpp"
#include "base/subscriber.hpp"
#include "message/std_msgs.hpp"

using namespace huleibao;

//////////////////////////////////////
//// funcions decl////////////////////
int InitCamera(int device_id);
void OnScan(std_msgs::Int8::ConstPtr msg);
float SearchExpByBrightness(cv::VideoCapture& cap, float min_bri, float max_bri, bool& success, float init_beg = 0.f);
cv::Mat GrabWithExposure(cv::VideoCapture& cap, float exposure);
bool InitCamExposure(cv::VideoCapture& cap);
float DetectBrightness(cv::Mat input_img);

//////////////////////////////////////
//// global vars//////////////////////
Subscriber<std_msgs::Int8>::Ptr g_sub;
Publisher<std_msgs::Image>::Ptr g_pub;
std::shared_ptr<cv::VideoCapture> g_cam;
int   g_grab_times = 4;
float g_grab_elaps = 0.5;
float g_min_exposure = 80.f;
float g_mid_exposure = 200.f;
float g_max_exposure = 400.f;

//////////////////////////////////////
//// main ////////////////////////////
int main(int argc, char* argv[])
{
    // - create camera node
    Node node("camera");

    // - notify core, wait a little while
    node.SetParam("start_camera", false);

    // - get configs
    std::string cfgFile = "./resource/game_config.yml";
    YAML::Node config = YAML::LoadFile(cfgFile);

    // - parse config .camera
    YAML::Node camCfg = config["game_config"]["camera"];
    g_grab_times = camCfg["grab_times"].as<int>();
    g_grab_elaps = camCfg["grab_interval"].as<float>();
    int camDvId  = camCfg["device_id"].as<int>();
    int camImgH  = camCfg["height"].as<int>();
    int camImgW  = camCfg["width"].as<int>();

    LOG_INFO("[camera] g_grab_times=" << g_grab_times << ", g_grab_elaps=" << g_grab_elaps);
    // - connect to camera
    if(0 != InitCamera(camDvId))
    {
        LOG_ERROR("[camera] camera init failed.");
        return 1;
    }
    // - set resolution
    g_cam->set(cv::CAP_PROP_FRAME_HEIGHT, camImgH);
    g_cam->set(cv::CAP_PROP_FRAME_WIDTH, camImgW);
    g_cam->set(cv::CAP_PROP_SATURATION, 75);
    LOG_INFO("[camera] set size=" << cv::Size(camImgW, camImgH));

    // - publish topic_image
    g_pub = node.Advertise<std_msgs::Image>("topic_image", g_grab_times);

    // - subscrib topic_scan
    g_sub = node.Subscribe<std_msgs::Int8>("topic_scan", 2, OnScan);

    // - notify core
    node.SetParam("start_camera", true);

    // - wait signal
    Spin();

    // - notify core
    node.SetParam("start_camera", false);

    return 0;
}


//////////////////////////////////////
//// function define//////////////////

void OnScan(std_msgs::Int8::ConstPtr msg)
{
    LOG_INFO("[camera] OnScan");
    // - grab some images
	for(int i=0; i<8; ++i) 
    {
        g_cam->grab(); 
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    for (int i = 0; i < g_grab_times; ++i)
    {
        cv::Mat frame;
        (*g_cam) >> frame;
        cv::imwrite("./garb.jpg", frame);
        // - then publish
        std_msgs::Image msg;
        msg.data.image_height = frame.rows;
        msg.data.image_width  = frame.cols;
        msg.data.image_format = ImageData::Format::BGR;
        msg.data.image_data.assign(frame.datastart, frame.dataend);
        LOG_INFO("[camera] img size=" << frame.size() << ", msg size="<<msg.data.image_data.size());
        g_pub->Publish(msg);
    }
}



int InitCamera(int device_id)
{
    g_cam.reset(new cv::VideoCapture(device_id));
    if(g_cam->isOpened())
    {
        cv::Mat frame;
        (*g_cam) >> frame;
        // if(!InitCamExposure(*g_cam)) return 1;
	    return 0;
    }
    LOG_ERROR("[camera] camera connected failed.");
    return 1;
}


cv::Mat GrabWithExposure(cv::VideoCapture& cap, float exposure)
{
    int preExp = cap.get(cv::CAP_PROP_EXPOSURE);
	cv::Mat frame;
	cap.set(cv::CAP_PROP_EXPOSURE, exposure);
	for(int i=0; i<4; ++i) 
    {
        cap.grab(); 
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    int nowExp = cap.get(cv::CAP_PROP_EXPOSURE);
    cap.read(frame);
	return frame;
}


float DetectBrightness(cv::Mat input_img)
{
	cv::Mat src_hsv;
	cv::cvtColor(input_img, src_hsv, cv::COLOR_BGR2HSV);
	std::vector<cv::Mat> hsv_channels;
	cv::split(src_hsv, hsv_channels);
	cv::Mat V = hsv_channels.at(2);
	return cv::mean(V).val[0];
}


float SearchExpByBrightness(cv::VideoCapture& cap, float min_bri, float max_bri, bool& success, float init_beg)
{
    success = false;
    float begExp = init_beg, endExp = 900.f;
    for(int i = 0; i < 10 && begExp < endExp; ++i)
    {
        float midExp = (begExp + endExp) / 2.f;
        cv::Mat frame = GrabWithExposure(cap, midExp); 
        frame = GrabWithExposure(cap, midExp); 
        frame = GrabWithExposure(cap, midExp); 
        frame = GrabWithExposure(cap, midExp); 
        float curBri = DetectBrightness(frame);
        if(curBri > max_bri) endExp = midExp;
        else if(curBri < min_bri) begExp = midExp;
        else { success = true; break;}
    }
    return begExp;
}


bool InitCamExposure(cv::VideoCapture& cap)
{
    float camBaseExposure = 5.0;
    bool initSuccess      = true;
	cv::Mat frame;
    GrabWithExposure(cap, camBaseExposure);
    GrabWithExposure(cap, camBaseExposure);
    LOG_INFO(" - init cam exposure begin...");

    g_min_exposure = SearchExpByBrightness(cap, 50, 60, initSuccess, 10.f);
    g_mid_exposure = SearchExpByBrightness(cap, 90, 100, initSuccess, g_min_exposure + 10.f);
    g_max_exposure = SearchExpByBrightness(cap, 120, 130, initSuccess, g_mid_exposure + 10.f);

    if (true/*initSuccess*/)
    {
        GrabWithExposure(cap, g_min_exposure);
        GrabWithExposure(cap, g_min_exposure);
        LOG_INFO(" - cam exposure exposure init done with min: "<<g_min_exposure<<", mid: "<<g_mid_exposure<<", max: "<<g_max_exposure);
    }
    return true;//initSuccess;
}

