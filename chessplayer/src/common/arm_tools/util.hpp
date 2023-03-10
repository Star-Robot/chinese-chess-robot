#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <string>
#include <vector>

class Util
{
public:
    static std::string item2Hex(int item);
    static std::string char2Hex(char* data, int inLen);
    static int hex2Int(char hexChar);
    static std::string hex2Str(std::string hex_str);
    static std::vector<std::string> SplitWithStl(const std::string& str, const std::string& pattern);
    static int hexStr2Int(std::string hex_str);
    static std::string int2Hex(const int integer, int length = -1);
};


#endif

