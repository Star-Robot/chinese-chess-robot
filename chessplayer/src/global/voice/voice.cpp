#include <vector>
#include <thread>
#include <chrono>
#include <yaml-cpp/yaml.h>

#include "base/node.hpp"
#include "base/publisher.hpp"
#include "base/subscriber.hpp"
#include "message/std_msgs.hpp"

#include "decoder/ffmpeg_decoder.h"
#include "decoder/pcm_reader.h"
#include "player/alsa_player.h"
#include "player/pcm_dumper.h"

using namespace huleibao;

//////////////////////////////////////
//// funcions decl////////////////////
void OnPlay(std_msgs::String::ConstPtr msg);

//////////////////////////////////////
//// global vars//////////////////////
Subscriber<std_msgs::String>::Ptr   g_sub;
std::map<std::string, std::string>  g_voice_mapper;
std::string                         g_snd_dev_name;
std::shared_ptr<FFmpegDecoder>      g_snd_decoder;
std::shared_ptr<AlsaPlayer>         g_snd_player;


int main(int argc, char* argv[])
{
    // - create voice node
    Node node("voice");

    // - get configs
    std::string resrc = "./resource/";
    std::string cfgFile = resrc + "game_config.yml";
    YAML::Node config;
    config = YAML::LoadFile(cfgFile);

    // - parse config .voice
    YAML::Node voiceCfg = config["game_config"]["voice"]["sound_name_to_file"];
    for(YAML::const_iterator viter = voiceCfg.begin(); viter != voiceCfg.end(); ++viter)
    {
        std::string vName = viter->first.as<std::string>();
        g_voice_mapper[vName] = resrc + viter->second.as<std::string>(); 
    }

    g_snd_dev_name = config["game_config"]["voice"]["sound_device_name"].as<std::string>();

    // - init sound decoder and player
    g_snd_decoder.reset(new FFmpegDecoder(2, AV_SAMPLE_FMT_S16, 44100));
    g_snd_player.reset(new AlsaPlayer(2, SND_PCM_FORMAT_S16_LE, 44100, g_snd_dev_name.c_str())); 
    g_snd_player->setDecoder(g_snd_decoder.get());

    // - play power_on voice
    std::string powerOnFile = g_voice_mapper["power_on"];
    g_snd_decoder->openFile(powerOnFile.c_str());
    g_snd_player->play();                                                        
    g_snd_decoder->release();

    // - subscrib topic_play
    g_sub = node.Subscribe<std_msgs::String>("topic_voice", 10, OnPlay);

    // - notify core
    node.SetParam("start_voice", true);

    // - wait signal
    Spin();

    // - notify core
    node.SetParam("start_voice", false);

    return 0;

}


//////////////////////////////////////
//// function define//////////////////

void OnPlay(std_msgs::String::ConstPtr msg)
{
    LOG_INFO("[voice] OnPlay " << msg->data);
    if(g_voice_mapper.count(msg->data) == 0)
    {
        LOG_ERROR("[voice] invalid voice name!");
        return;
    }

    std::string voiceFile = g_voice_mapper[msg->data];
    g_snd_decoder->openFile(voiceFile.c_str());
    g_snd_player->play();                                                        
    g_snd_decoder->release();
}
