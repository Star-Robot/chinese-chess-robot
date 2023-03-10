#include "util/util.hpp"
#include "common/timer.hpp"
#include "ucci_engine.hpp"


UcciEngine::UcciEngine(const int ai_level)
{
    const char* engineFile = "resource/stockfish";
    const char* nnueFile = "resource/weights/xiangqi-83f16c17fe26.nnue";
    char cwdBuff[256];
    getcwd(cwdBuff, 256);
    std::string currentWorkDir(cwdBuff);

    LOG_INFO("[situator] Current working directory: " << currentWorkDir);
    std::string enginePath = currentWorkDir + "/" + engineFile;
    std::string nnuePath = currentWorkDir + "/" + engineFile;
    LOG_INFO("[situator] Ucci Engine open " << enginePath);
    LOG_INFO("[situator] Ucci nnue path " << nnuePath);
    // - init pipe with eigen
    pipe_.Open(enginePath.c_str());

    bool has_score_mate_1 = false; 
    std::vector<std::string> empty;
    std::vector<std::string> keywords = {"ucciok"};
    std::string ucciAns = WriteAndRead("ucci", keywords, has_score_mate_1);
    if(ucciAns.empty())
        LOG_FATAL("[situator] Ucci Engine start failed!");
    // - see
    WriteAndRead("uccinewgame", empty, has_score_mate_1);
    WriteAndRead("setoption UCI_LimitStrength true", empty, has_score_mate_1);
    WriteAndRead("setoption UCI_Elo 1000", empty, has_score_mate_1);
    WriteAndRead("setoption Use_NNUE true", empty, has_score_mate_1);
    std::string setEvalFile = std::string("setoption EvalFile ") + nnuePath;
    WriteAndRead(setEvalFile, empty, has_score_mate_1);

    // - use go noeds xxx; 
    ai_level_ = ai_level;
}


// - Set UciELO to change Robot skill level
void UcciEngine::SetAiLevel(const int ai_level)
{
    // - set go nodes  
    ai_level_ = ai_level;
}



std::pair<std::string,bool> UcciEngine::Query(const std::string& fen_str)
{
    std::vector<std::string> moveKeywords = {"bestmove", "nobestmove"};

    // - A table to set AI skill level
    std::map<int, std::pair<int, int> > keyToEloAndNode = {
        {1, { 500,  500     }},
        {2, { 1000, 7766    }},
        {3, { 1600, 279936  }},
        {4, { 2400, 1679616 }},
        {5, { 2600, 1679616 }},
        {6, { 2800, 1679616 }}
    };

    const int elo = keyToEloAndNode[ai_level_].first;
    const int nodes = keyToEloAndNode[ai_level_].second;

    // - set instruction 
    std::string instruction = fen_str + "\nsetoption UCI_LimitStrength true\nsetoption UCI_Elo " 
        + std::to_string(elo) + "\ngo nodes " + std::to_string(nodes) + "\n";
    LOG_ERROR("[situator] instruction: \n" << instruction);
    bool has_score_mate_1 = false;
    std::string moveAns = WriteAndRead(instruction, moveKeywords, has_score_mate_1);
    LOG_ERROR("[situator] answer: " << moveAns << ", has_score_mate_1: " << has_score_mate_1);
    if(moveAns.empty())
    {
        LOG_ERROR("[situator] Error: " << instruction);
        LOG_FATAL("[situator] Ucci Engine has no answer!");
    }
    return std::make_pair(moveAns, has_score_mate_1);
}


std::string UcciEngine::WriteAndRead(
    const std::string& instruction,
    const std::vector<std::string>& keywords,
    bool& has_score_mate_1)
{
    std::string lineStr;
    pipe_.LineOutput(instruction.c_str());
    char buffer[LINE_INPUT_MAX_CHAR];
    bool catched = keywords.empty();

    Timer timer;
    while(!catched && timer.Elapsed() < 1e9)
    {
        memset(buffer, '\0', sizeof(buffer));
        if(pipe_.LineInput(buffer))
        {
            lineStr = std::string(buffer);
            if(lineStr.empty()) continue;
            for(auto& kw : keywords)
                catched |= lineStr.find(kw) != lineStr.npos;
            has_score_mate_1 |= lineStr.find("score mate 1") != lineStr.npos;
        }
    }
    return catched ? lineStr : "";
}

