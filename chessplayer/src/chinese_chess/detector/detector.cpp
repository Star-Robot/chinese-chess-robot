#include "rknn_api.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <yaml-cpp/yaml.h>

#include "base/node.hpp"
#include "base/publisher.hpp"
#include "base/subscriber.hpp"
#include "message/std_msgs.hpp"

#include "common/json.hpp"

using namespace huleibao;


//////////////////////////////////////
//// funcions decl////////////////////
int InitRKNN(std::string& model_file);
std::vector<unsigned char> FdLoadFile(std::string path);
void OnImage(std_msgs::Image::ConstPtr msg);
std::map<int, std::vector<std::vector<float>>> ProcessSDKOutput(
    rknn_output* outputs, int marg_h, int marg_w, float ratio);
//////////////////////////////////////
//// global vars//////////////////////
Subscriber<std_msgs::Image>::Ptr g_sub;
Publisher<std_msgs::String>::Ptr g_pub;
std::shared_ptr<rknn_context> g_predictor;
rknn_input_output_num g_net_io_num;
std::vector<rknn_tensor_attr> g_net_output_attrs;
std::map<int, std::string> g_cnn_cls_to_label;
int g_grab_times = 4;

//////////////////////////////////////
//// main ////////////////////////////
int main()
{
    // - create detector node
    Node node("detector");

    // - notify the game master
    node.SetParam("start_detector", false);

    // - get configs
    std::string resrc = "./resource/";
    std::string cfgFile = resrc + "game_config.yml";
    YAML::Node config = YAML::LoadFile(cfgFile);

    // - parse config
    YAML::Node detCfg = config["game_config"]["detector"];
    std::string modelFile = resrc + detCfg["net_model"].as<std::string>();
    YAML::Node camCfg = config["game_config"]["camera"];
    g_grab_times = camCfg["grab_times"].as<int>();
    g_cnn_cls_to_label = detCfg["labelMap"].as<std::map<int, std::string>>();

    // - CNN model init
    g_predictor = std::make_shared<rknn_context>();
    int ret = InitRKNN(modelFile);
	if (ret != 0)
	{
        LOG_ERROR("[detector] Predictor Initing failed, please check the option parameters");
        return 1;
    }

    // - subscrib topic_scan
    g_sub = node.Subscribe<std_msgs::Image>("topic_roi", g_grab_times, OnImage);

    // - publish topic_detection
    g_pub = node.Advertise<std_msgs::String>("topic_detection", g_grab_times);

    // - notify the game master
    node.SetParam("start_detector", true);

    // - wait signal
    Spin();

    // - notify the game master
    node.SetParam("start_detector", false);

    return 0;
}

//////////////////////////////////////
//// function define//////////////////

void OnImage(std_msgs::Image::ConstPtr msg)
{
    LOG_INFO("[detector] OnImage");
    // - check empty
    if(msg->data.image_data.empty())
    {
        nlohmann::json detOut;
        detOut["coords"]  = {};
        detOut["classes"] = {};
        /// publish an empty msg
        std_msgs::String detMsg;
        detMsg.data = std::string(detOut.dump());
        LOG_INFO("[detector] No roi found.");
        g_pub->Publish(detMsg);
        return;
    }
    // - decode image data
    int imH = msg->data.image_height;
    int imW = msg->data.image_width;
    std::vector<uint8_t>& imData = msg->data.image_data;
    cv::Mat img(imH, imW, CV_8UC3, imData.data());
    cv::Mat affine = img.clone();
    cv::imwrite("affine.jpg", affine);
    // - detect chess
    /// preprocess
    float netW = 512.f, netH = 512.f;
    float imgH = img.rows, imgW = img.cols;
    float ratio = std::min(netW / imgW, netH / imgH);
    int newH = ratio * imgH;
    int newW = ratio * imgW;
    /// resize image
    cv::resize(img, img, cv::Size(newW, newH));
    /// padding
    int margH = (netH - newH) / 2;
    int margW = (netW - newW) / 2;
    cv::Rect roi(margW, margH, newW, newH);
    cv::Mat netIm = cv::Mat::zeros(netH, netW, CV_8UC3);
    img.copyTo(netIm(roi));
    LOG_INFO(" - input.isContinuous = " << netIm.isContinuous());
    netIm = netIm.clone();
    cv::imwrite("net_input.jpg", netIm);

    /// Set Input Data
    rknn_input inputs[1];
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].size = netIm.cols*netIm.rows*netIm.channels();
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].buf = netIm.data;

    int ret = rknn_inputs_set(*g_predictor, g_net_io_num.n_input, inputs);
    if(ret < 0) 
    {
        LOG_ERROR("rknn_input_set failed!");
        return;
    }
    /// run
    ret = rknn_run(*g_predictor, nullptr);
    if(ret < 0) 
    {
        LOG_ERROR("rknn_run failed!");
        return;
    }

    /// get Output
    rknn_output outputs[g_net_io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < g_net_io_num.n_output; i++) 
    {
        outputs[i].index = i;
        outputs[i].is_prealloc = 0;
        outputs[i].want_float = 1;
    }
    ret = rknn_outputs_get(*g_predictor, g_net_io_num.n_output, outputs, nullptr);
    if(ret < 0) 
    {
        LOG_ERROR("rknn_outputs_get failed!");
        return;
    }

    /// post process
    std::map<int, std::vector<std::vector<float>>> predOut = 
        ProcessSDKOutput(outputs, margH, margW, ratio);

    // - convert to json
    nlohmann::json detOut;
    detOut["coords"]  = {};
    detOut["classes"] = {};
    for(auto& pr : predOut)
    {
        int cls = pr.first;
        for(auto& det : pr.second)
        {
            float xMin = det[0];
            float yMin = det[1];
            float xMax = det[2];
            float yMax = det[3];
            float cx   = (xMin + xMax) / 2.f;
            float cy   = (yMin + yMax) / 2.f;
            detOut["coords"].emplace_back(std::vector<float>{cx, cy});
            detOut["classes"].emplace_back(g_cnn_cls_to_label[cls]);
            //debug
            cv::circle(affine, cv::Point(int(cx), int(cy)), 4, cv::Scalar(255, 0, 0), -1);
            cv::putText(affine, g_cnn_cls_to_label[cls], cv::Point(int(cx), int(cy)), cv::FONT_HERSHEY_COMPLEX, 0.4, cv::Scalar(255, 0, 0), 1, 8, 1);
        }
    }
    cv::imwrite("detout.jpg", affine);
    // - publish detection
    std_msgs::String detMsg;
    detMsg.data = std::string(detOut.dump());
    g_pub->Publish(detMsg);
    LOG_INFO("[detector] OnImage publish detection done.");
}


// std::string FdLoadFile(std::string path)
std::vector<unsigned char> FdLoadFile(std::string path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return {};
    file.seekg(0, file.end);
    int size = file.tellg();
    std::vector<unsigned char> fileContent(size);
    file.seekg(0, file.beg);
    file.read((char*)fileContent.data(), size);
    file.close();
    return fileContent;
}


int InitRKNN(std::string& model_file)
{
    auto model_content = std::move(FdLoadFile(model_file));
    if(model_content.empty())
    {
        LOG_ERROR("[detector] CNN weight load failed, please check the [net_proto] [net_model] in config");
        return 1;
    }
    int ret = rknn_init(g_predictor.get(), model_content.data(), model_content.size(), 0, nullptr);
    if(ret < 0) 
    {
        LOG_ERROR("[detector] rknn_init failed!");
        return 1;
    }
    
    // Get Model Input Output Info
    ret = rknn_query(*g_predictor, RKNN_QUERY_IN_OUT_NUM, &g_net_io_num, sizeof(g_net_io_num));
    if (ret != RKNN_SUCC) 
    {
        LOG_ERROR("[detector] rknn_query input failed!");
        return 1;
    }
    LOG_INFO("[detector] model input num: " << g_net_io_num.n_input << ", output num: " << g_net_io_num.n_output);

    g_net_output_attrs.resize(g_net_io_num.n_output);
    memset(g_net_output_attrs.data(), 0, sizeof(rknn_tensor_attr) * g_net_io_num.n_output);
    for (int i = 0; i < g_net_io_num.n_output; i++) 
    {
        g_net_output_attrs[i].index = i;
        ret = rknn_query(*g_predictor, RKNN_QUERY_OUTPUT_ATTR, &(g_net_output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) 
        {
            LOG_ERROR("[detector] rknn_query output failed!");
            return 1;
        }
    }
    LOG_INFO("[detector] model inited done! ");
    return 0;
}



unsigned char* FdLoadFileV1(const char *filename, int *model_size)
{
    FILE *fp = fopen(filename, "rb");
    if(fp == nullptr) 
    {
        LOG_ERROR("[detector] CNN weight load failed, please check the [net_model] in config");
        return nullptr;
    }
    fseek(fp, 0, SEEK_END);
    int model_len = ftell(fp);
    unsigned char *model = (unsigned char*)malloc(model_len);
    fseek(fp, 0, SEEK_SET);
    if(model_len != fread(model, 1, model_len, fp)) 
    {
        LOG_ERROR("[detector] CNN weight load failed, please check the [net_model] in config");
        free(model);
        return nullptr;
    }
    *model_size = model_len;
    if(fp) fclose(fp);
    return model;
}



std::map<int, std::vector<std::vector<float>>> ProcessSDKOutput(
    rknn_output* outputs, int marg_h, int marg_w, float ratio)
{
    /// save the detect reslut: {cat:[[xMin,yMin,xMax,yMax,score], [], ...]}
    std::map<int, std::vector<std::vector<float>>> predOut;

    if(!outputs)
    {
        LOG_ERROR("[detector] cnn predict output nullptr");
        return predOut;
    }

    std::vector<rknn_tensor_attr>& attr = g_net_output_attrs;
    // - shape: NCHW
    const std::vector<int> hmShape =      //1,14,128,128
        { attr[0].dims[0], attr[0].dims[1], attr[0].dims[2], attr[0].dims[3] };
    const std::vector<int> whShape =      //1,2,128,128
        { attr[1].dims[0], attr[1].dims[1], attr[1].dims[2], attr[1].dims[3] };
    const std::vector<int> regShape =     //1,2,128,128
        { attr[3].dims[0], attr[3].dims[1], attr[3].dims[2], attr[3].dims[3] };

    // - get topk according to picece'categary
    float* hmData = reinterpret_cast<float*>(outputs[0].buf);
    if(hmShape[0]!=1 || hmShape[1] != 14 || hmShape[2] != 128 ||hmShape[3] != 128)
    {
        LOG_ERROR("[detector] hm map shape != [1x14x128x128]");
        return predOut;
    }
    float* whData = reinterpret_cast<float*>(outputs[1].buf);
    if(whShape[0]!=1 || whShape[1] != 2 || whShape[2] != 128 ||whShape[3] != 128)
    {
        LOG_ERROR("[detector] wh map shape != [1x2x128x128]");
        return predOut;
    }
    float* regData = reinterpret_cast<float*>(outputs[3].buf);
    if(regShape[0]!=1 || regShape[1] != 2 || regShape[2] != 128 ||regShape[3] != 128)
    {
        LOG_ERROR("[detector] reg map shape != [1x2x128x128]");
        return predOut;
    }

    const std::map<int, int> topKmapper =
    {
        {1 , 2}, // red_ju
        {2 , 5}, // red_bing
        {3 , 2}, // red_xiang
        {4 , 5}, // black_zu
        {5 , 2}, // red_pao
        {6 , 2}, // red_ma
        {7 , 2}, // black_ma
        {8 , 2}, // red_shi
        {9 , 1}, // red_shuai
        {10, 2}, // black_pao
        {11, 2}, // black_xiang
        {12, 2}, // black_shi
        {13, 1}, // black_jiang
        {14, 2}, // black_ju
    };

    const float scoreThr = 0.2;
    const int kernel     = 7;
    const int pad        = (kernel - 1) / 2;
    const int chnSize    = hmShape[2] * hmShape[3];
    const float netDownS = 4.f;

    float* whWData  = whData  + 0 * chnSize;
    float* whHData  = whData  + 1 * chnSize;
    float* regXData = regData + 0 * chnSize;
    float* regYData = regData + 1 * chnSize;

    // - NMS per channel
    for(int c = 0; c != hmShape[1]; ++c)
    {
        int cat = c + 1;
        int topk = topKmapper.at(cat);
        std::map<float, std::pair<int, int>> cTopk;
        float* cData = hmData + c * chnSize;
        for(int h = 0; h != hmShape[2]; ++h)
        {
            for(int w = 0; w != hmShape[3]; ++w)
            {
                // search range
                int hBeg = std::max(0, h - pad);
                int hEnd = std::min(hmShape[2] - 1, h + pad);
                int wBeg = std::max(0, w - pad);
                int wEnd = std::min(hmShape[3] - 1, w + pad);

                /// search max score
                float maxScore = 0;
                for(int m = hBeg; m != hEnd; ++m)
                {
                    for(int n = wBeg; n != wEnd; ++n)
                    {
                        float score = cData[m * hmShape[3] + n];
                        maxScore = std::max(score, maxScore);
                    }
                }
                /// store it if hit the maxScore and > scoreThr
                float curScore = cData[h * hmShape[3] + w];
                if(maxScore > scoreThr && std::abs(curScore-maxScore) < 1e-5)
                    cTopk[maxScore] = std::make_pair(h, w);
            }
        }
        /// keep only topk predict on this channel
        auto it = cTopk.rbegin();
        for(int i = 0; i != topk && it != cTopk.rend(); ++i, ++it)
        {
            float score = it->first;
            int h = it->second.first;
            int w = it->second.second;
            float predW  = netDownS * whWData[h * whShape[3] + w];
            float predH  = netDownS * whHData[h * whShape[3] + w];
            float predCX = netDownS * (w + regXData[h * regShape[3] + w]);
            float predCY = netDownS * (h + regYData[h * regShape[3] + w]);
            /// restore to raw image size
	        // LOG_INFO("predW="<<predW<<",predH="<<predH<<",predCX="<<predCX<<",predCY="<<predCY);
            predW /= ratio;
            predH /= ratio;
            predCX = (predCX - marg_w) / ratio;
            predCY = (predCY - marg_h) / ratio;
            /// bounding box
            float xMin = predCX - predW / 2.f;
            float yMin = predCY - predH / 2.f;
            float xMax = predCX + predW / 2.f;
            float yMax = predCY + predH / 2.f;
	        // LOG_INFO("ratio="<<ratio<<",marg_w="<<marg_w<<",marg_h="<<marg_h);
	        // LOG_INFO("xMin="<<xMin<<",yMin="<<yMin<<",xMax="<<xMax<<",yMax"<<yMax);
            predOut[cat].emplace_back(std::vector<float>{xMin, yMin, xMax, yMax, score});
        }
    }

    return predOut;
}
