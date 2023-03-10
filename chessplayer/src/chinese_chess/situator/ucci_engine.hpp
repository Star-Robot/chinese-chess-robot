#ifndef ucci_engine_hpp__
#define ucci_engine_hpp__

#include <string>
#include <vector>
#include "pipe.hpp"

class UcciEngine
{
public:
    UcciEngine(const int ai_level = 1);

    // - Set UciELO to change Robot skill level
    void SetAiLevel(const int ai_level);

    // - The human command tells the UCCI engine and waits for its feedback.
    std::pair<std::string,bool> Query(const std::string& fen_str);

    // - Parse the output of the pipeline and grab the line with keywords
    std::string WriteAndRead(
        const std::string& instruction,
        const std::vector<std::string>& keywords,
        bool& has_score_mate_1);

private:
    PipeStruct pipe_;
    int ai_level_;
};

#endif//ucci_engine_hpp__

