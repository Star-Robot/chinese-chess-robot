#include "util.hpp"


std::string Util::item2Hex(int item)
{
    std::string message;
    if (item >= 0 && item <= 9)
    {
        message = std::to_string(item);
    }
    else if(item == 10) message = "a";
    else if(item == 11) message = "b";
    else if(item == 12) message = "c";
    else if(item == 13) message = "d";
    else if(item == 14) message = "e";
    else if(item == 15) message = "f";
    return message;
}

std::string Util::int2Hex(const int integer, int length)
{
    std::vector<std::string> itemsTmp;
    int divisor = integer;
    while(divisor>=16)
    {
        itemsTmp.emplace_back(item2Hex(divisor%16));
        divisor = divisor/16;
    }
    itemsTmp.emplace_back(item2Hex(divisor%16));
    std::vector<std::string> items;
    for (int i = int(itemsTmp.size())-1;i >= 0;i--)
        items.emplace_back(itemsTmp[i]);
    
    // 保证数据的长度
    if (length != -1 && items.size() > length)
    {
        std::vector<std::string> tmp(items.end() - length, items.end());
        items.swap(tmp);
    }
    else if (items.size() < length)
    {
        while(items.size() < length)
            items.insert(items.begin(), {"0"});
    }

    // 将十六进制vector数据转为中间带空格的字符串
    std::string result;
    int cnt = 0;
    if (int(items.size()) % 2 != 0)
    {
        result += "0";
        cnt ++;
    }
    for (int i = 0;i <int(items.size());i++)
    {
        result += items[i];
        cnt ++;
        if ((cnt % 2) == 0 && i != 0) result += " ";
    }
    return result;
}

std::string Util::char2Hex(char* data, int inLen)
{
    std::string message;
    for (int i = 0;i < inLen;++i)
    {
        int elemInt = data[i];
        int elemIntHigh = elemInt / 16;
        int elemIntLow = elemInt % 16;
        message += item2Hex(elemIntHigh);
        message += item2Hex(elemIntLow);
        message += " ";
    }
    return message;
}

int Util::hex2Int(char hexChar)
{
    int message;
    if (hexChar >= '0' && hexChar <= '9')
    {
        message = hexChar - '0';
    }
    else if(hexChar == 'a' || hexChar == 'A') message = 10;
    else if(hexChar == 'b' || hexChar == 'B') message = 11;
    else if(hexChar == 'c' || hexChar == 'C') message = 12;
    else if(hexChar == 'd' || hexChar == 'D') message = 13;
    else if(hexChar == 'e' || hexChar == 'E') message = 14;
    else if(hexChar == 'f' || hexChar == 'F') message = 15;
    return message;
}

std::string Util::hex2Str(std::string hex_str)
{
    std::string message;
    int size = hex_str.size();
    // std::cout<<"hex_str: "<<hex_str<<std::endl;
    // std::cout<<"size: "<<size<<std::endl;
    char* data = (char*)hex_str.c_str();
    for (int i = 0;i < size;)
    {
        if (data[i] == ' ')
        {
            i += 1;
            continue;
        }
        int high = hex2Int(data[i]);
        int low = hex2Int(data[i+1]);
        char item = high*16+low;
        message += item;
        i += 2;
    }
    return message;
}

int Util::hexStr2Int(std::string hex_str)
{
    std::string message;
    int size = hex_str.size();
    // std::cout<<"hex_str: "<<hex_str<<std::endl;
    // std::cout<<"size: "<<size<<std::endl;
    int result = 0;
    char* data = (char*)hex_str.c_str();
    for (int i = 0;i < size;)
    {
        if (data[i] == ' ')
        {
            i += 1;
            continue;
        }
        result = result*16+hex2Int(data[i]);
        i += 1;
    }
    return result;
}

/**
 @function: 以一定的字符串pattern拆分字符串str。
 @param str: 原始长字符串
 @param pattern: 拆分的分割字符或字符串
 @return: 拆分后的字符串组。
 */
std::vector<std::string> Util::SplitWithStl(const std::string& str, const std::string& pattern)
{
    std::vector<std::string> resVec;

    if ("" == str)
    {
        return resVec;
    }
    //方便截取最后一段数据
    std::string strs = str + pattern;

    size_t pos = strs.find(pattern);
    size_t size = strs.size();

    while (pos != std::string::npos)
    {
        std::string x = strs.substr(0, pos);
        resVec.push_back(x);
        strs = strs.substr(pos + 1, size);
        pos = strs.find(pattern);
    }

    return resVec;
}

