#ifndef CONFIG_H
#define CONFIG_H

#include <termios.h>
#include <vector>
#include <string>
#include <map>



class Config
{
public:
    static const int uart_host_name;

    static const speed_t serial_freq;
    static const int serial_name;

    static const int   upper_arm_id;
    static const float upper_arm_len;
    static const float upper_arm_init_angle;
    static const std::vector<float> upper_arm_angle_range;

    static const int   lower_arm_id;
    static const float lower_arm_len;
    static const float lower_arm_init_angle;

    static const int min_arm_omiga;
    static const int max_arm_omiga;

    static const int   hand_id;
    static const int   hand_omiga;
    static const float hand_len;
    static const float hand_init_angle;
    static const float hand_take_angle;
    static const float hand_drop_angle;

    static const float wrist_len;
    static const float wrist_angle;
};

#endif

