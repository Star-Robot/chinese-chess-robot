#include "util/util.hpp"


namespace huleibao
{

Util::Util()
{
}


Util::~Util()
{
}


int64_t Util::GetTimeStamp()
{
    //获取当前时间点
    std::chrono::time_point<std::chrono::system_clock,std::chrono::microseconds> tp
        = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    //计算距离1970-1-1,00:00的时间长度
    std::time_t timestamp =  tp.time_since_epoch().count();
    return uint64_t(timestamp);
}


// - In-place refresh display progress
void Util::ShowProgressBar(int cnt, int total, int interval)
{
    using namespace std;
    using namespace chrono;
	if(cnt != total && cnt % interval != 0) return;
	static auto pre = system_clock::now();
	auto now = system_clock::now();

	auto duration = duration_cast<microseconds>(now - pre);
	double cost = double(duration.count()) * microseconds::period::num / microseconds::period::den + 0.0000001;
	pre = now;
	int speed = interval / cost;

	float ratio = float(cnt) / total;
	string bar(ratio * 100, '#');
	const char *lable = "|/-\\";
	printf("[%-100s][%d%%, %d/%d, %d/s] [%c]\r", bar.c_str(), int(ratio * 100), cnt, total, speed, lable[cnt/interval%4]);
	fflush(stdout);
	if(cnt == total) cout << endl;
}


// - String is split into arrays by the specified separator
void Util::Split(const std::string& str, std::vector<std::string>& intArr, char c)
{
    std::string elem;
    for (size_t i = 0; i <= str.size(); ++i)
    {
        if (i == str.size() && !elem.empty())
            intArr.emplace_back(elem);
        else if (str[i] == c)
        {
            if (!elem.empty())
                intArr.emplace_back(elem);
            elem.clear();
        }
        else
            elem += str[i];
    }
}


// - Replace the specified content in the string
std::string Util::ReplaceAllDistinct(const std::string& str, const std::string& old_value, const std::string& new_value)
{
	std::string rurl = str;
	for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length())
	{
		if ((pos = rurl.find(old_value, pos)) != std::string::npos)
		{
			rurl.replace(pos, old_value.length(), new_value);
		}
		else
		{ break; }
	}
	return rurl;
}


std::string& Util::Trim(std::string &s)
{
    if (s.empty()) return s;
    s.erase(0,s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    s.erase(0,s.find_first_not_of("\t"));
    s.erase(s.find_last_not_of("\t") + 1);
    return s;
}


////////////////////
//create instance //
////////////////////
static Util g_util;


} // namespace huleibao
