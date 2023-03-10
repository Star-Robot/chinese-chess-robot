#include "util/util.hpp"
#include "chess_manager.hpp"
#include "ucci_engine.hpp"
#include "common/json.hpp"


ChessManager::ChessManager(YAML::Node& chess_cfg)
{
    float gridH     = chess_cfg["chessboard_height"].as<float>();
    float gridW     = chess_cfg["chessboard_width"].as<float>();
    float viewH     = chess_cfg["viewboard_height"].as<float>();
    float viewW     = chess_cfg["viewboard_width"].as<float>();
    grid_rows_      = chess_cfg["board_rows"].as<float>(); // 10
    grid_cols_      = chess_cfg["board_cols"].as<float>(); //  9
    float viewScale = chess_cfg["vis_affined_scale"].as<float>();
    offset_x_       = viewScale * (viewW - gridW) / 2.f;
    offset_y_       = viewScale * (viewH - gridH) / 2.f;
    grid_unit_x_    = viewScale * gridW / (grid_cols_ - 1);
    grid_unit_y_    = viewScale * gridH / (grid_rows_ - 1);

    cnn_cls_to_ucci_ = std::map<std::string, char>
    {
        {"red_ju",      'R'},
        {"red_bing",    'P'},
        {"red_xiang",   'B'},
        {"black_zu",    'p'},
        {"red_pao",     'C'},
        {"red_ma",      'N'},
        {"black_ma",    'n'},
        {"red_shi",     'A'},
        {"red_shuai",   'K'},
        {"black_pao",   'c'},
        {"black_xiang", 'b'},
        {"black_shi",   'a'},
        {"black_jiang", 'k'},
        {"black_ju",    'r'}
    };

    // - default chess formation
    init_formation_ = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR";
    InitPieceMap(init_formation_);
    // - The first UCCI instruction
    // - !Note: must be the red side, that is, the human first hand!
    std::string instruct = std::string("position fen ") + init_formation_ + " w " + "-- 0 1 moves";
    ucci_instructions_.emplace_back(instruct);

    // - aiLevel.
    int aiLevel = 1;
    ucci_engine_ = std::make_shared<UcciEngine>(aiLevel);
}


// - Restore the layout of the pieces from the given game record
void ChessManager::InitPieceMap(const std::string& formation)
{
    int gridX = 0, gridY = 0;
    for(int i = 0; i != formation.size(); ++i)
    {
        if(formation[i] == '/')
        {
            gridX = 0;
            gridY = gridY + 1;
            continue;
        }
        else if(std::isalpha(formation[i]))
        {
            char ucciX = CvtGridXToUcci(gridX);
            char ucciY = CvtGridXToUcci(gridY);
            auto key = std::make_pair(gridX, gridY);
            char chc = formation[i];
            pieces_map_[key] = std::make_shared<Piece>(
                    chc, -1.f, -1.f, gridX, gridY, ucciX, ucciY);
            /// increase x
            gridX = gridX + 1;
        }
        else if(std::isalnum(formation[i]))
        {
            /// increase x
            gridX = gridX + formation[i] - '0';
        }
        else
        {
            /// formation format error
            LOG_FATAL("[situator] formation initial failure!");
            LOG_FATAL("[situator] " << formation);
        }
    }
}


// - clear situator status, prepare for next update
void ChessManager::Prepare()
{
    /// reset all piece to 'LOST'
    for(auto& pr : pieces_map_)
        pr.second->status = 0;
    /// clear unmatched pieces
    newcomers_.clear();
    /// clear unmatched pieces
    lost_.clear();
}


// - match each detection with old pieces's map
void ChessManager::Read(float pred_x, float pred_y, const std::string& pred_c)
{
    /// convert pred-XY to grid-XY
    float predX = (pred_x - offset_x_) / grid_unit_x_;
    float predY = (pred_y - offset_y_) / grid_unit_y_;
    int  gridX  = predX > 0 ? predX + 0.5 : predX - 0.5;
    int  gridY  = predY > 0 ? predY + 0.5 : predY - 0.5;

    /// check grid range
    if(gridX < 0 || gridX > 8 || gridY < 0 || gridY > 9) return;
    
    char ucciX = CvtGridXToUcci(gridX);
    char ucciY = CvtGridXToUcci(gridY);
    auto key   = std::make_pair(gridX, gridY);
    char chc   = cnn_cls_to_ucci_.at(pred_c);
    LOG_INFO("[situator] read: (" << gridX << "," << gridY << "), " << chc);
    /// matching to piece's map
    if(pieces_map_.count(key) && pieces_map_.at(key)->charactor == chc)
    {
        pieces_map_.at(key)->status = 1;
    }
    else
    {
        newcomers_[key] = std::make_shared<Piece>(
            chc, pred_x, pred_y, gridX, gridY, ucciX, ucciY);
    }
}


// - Summarize the difference and judge whether the changes are reasonable
std::string ChessManager::Summary(const std::string& round)
{
    /// the pieces's been matched, won't considered to be newcomer
    for(auto& pr : pieces_map_)
    {
        /// collect the lost
        if(pr.second->status == 0)
            lost_[pr.first] = pr.second;
        else
            newcomers_.erase(pr.first);
    }
    
    /// Summarize the difference
    nlohmann::json difference;
    difference["newcomers"] = {};
    difference["losts"] = {};

    /// search from and to 
    bool moveValid = true;
    if(newcomers_.size() == 1)
    {
        auto nPiece = newcomers_.begin()->second;
        int nCh = int(nPiece->charactor);
        int nGx = nPiece->grid_x;
        int nGy = nPiece->grid_y;
        nlohmann::json nTar = {nCh, nGx, nGy};
        difference["newcomers"].emplace_back(nTar);
        for(auto& pr : lost_)
        {
            int lCh = int(pr.second->charactor);
            int lGx = pr.second->grid_x;
            int lGy = pr.second->grid_y;
            /// two conds: 1)from; 2)to
            if(nCh == lCh)
            {
                /// check avlid move: nPiece→lPiece
                LOG_INFO("check avlid move: nPiece→lPiece");
                moveValid &= NextStepAvalid(pr.second, nPiece->grid_x, nPiece->grid_y);
                moveValid &= !((round == "human") ^ nPiece->IsUpper());
                nlohmann::json tar = {lCh, lGx, lGy};
                difference["losts"].emplace_back(tar);
            }
            else if(nGx == lGx && nGy == lGy)
            {
                nlohmann::json tar = {lCh, lGx, lGy};
                difference["losts"].emplace_back(tar);
            }
            else
            {
                /// detect error may occur!
                LOG_WARN("[situator] detector error occur, lost=(" << lCh << "," << lGx << "," << lGy << ")");
            }
        }
    }
	/// remember that: while newcomers_ is empty, it means game is just openning.
    else if(newcomers_.empty())
    {
		for(auto& pr : lost_)
		{
			int ch = int(pr.second->charactor);
			int gx = pr.second->grid_x;
			int gy = pr.second->grid_y;
			nlohmann::json tar = {ch, gx, gy};
			difference["losts"].emplace_back(tar);
		} 
    }

    /// judge
    bool ncmsValid = newcomers_.size() == 1;
    bool lostValid = difference["losts"].size() >= 1 && difference["losts"].size() <= 2;
    difference["reasonable"] = moveValid && ncmsValid && lostValid;
    return std::string(difference.dump());
}


bool ChessManager::NextStepAvalid(Piece::Ptr piece, int tar_grid_x, int tar_grid_y)
{
    /// begGx,begGy != tarGx,tarGy
    int begGx = piece->grid_x;
    int begGy = piece->grid_y;
    int tarGx = tar_grid_x;
    int tarGy = tar_grid_y;
    LOG_INFO("[situator] NextStepAvalid: begGx="<<begGx << ",begGy=" <<begGy << ",tarGx="<<tarGx << ",tarGy=" <<tarGy )
    /// tarGx,tarGy has to be None or amy
    auto tarKey  = std::make_pair(tarGx, tarGy);
    bool capture = pieces_map_.count(tarKey);
    if(capture)
    {
        auto tarPiece = pieces_map_.at(tarKey);
        bool enemy = (piece->IsUpper()^tarPiece->IsUpper());
        LOG_INFO("[situator] NextStepAvalid: capture="<<capture << ",enemy=" <<enemy << ", targe piece=" << tarPiece->charactor);
        if(!enemy) return false;
    }

    /// check travel distance / out of bounds / hinder
    char chc = piece->charactor;
    LOG_INFO("[situator] NextStepAvalid: source piece="<<chc);
    if(chc == 'K')
    {
        bool stepValid = (std::abs(tarGx-begGx) + std::abs(tarGy-begGy)) == 1;
        bool inbound = 3<=tarGx && tarGx<=5 && 7<=tarGy && tarGy<=9;
        return stepValid && inbound;
    }
    else if(chc == 'k')
    {
        bool stepValid = (std::abs(tarGx-begGx) + std::abs(tarGy-begGy)) == 1;
        bool inbound = 3<=tarGx && tarGx<=5 && 0<=tarGy && tarGy<=2;
        return stepValid && inbound;
    }
    else if(chc == 'A')
    {
        bool stepValid = (std::abs(tarGx-begGx) + std::abs(tarGy-begGy)) == 2;
        bool inbound = 3<=tarGx && tarGx<=5 && 7<=tarGy && tarGy<=9;
        return stepValid && inbound;
    }
    else if(chc == 'a')
    {
        bool stepValid = (std::abs(tarGx-begGx) + std::abs(tarGy-begGy)) == 2;
        bool inbound = 3<=tarGx && tarGx<=5 && 0<=tarGy && tarGy<=2;
        return stepValid && inbound;
    }
    else if(chc == 'B')
    {
        bool stepValid = (std::abs(tarGx-begGx) == 2) && (std::abs(tarGy-begGy) == 2);
        bool inbound = 5<=tarGy && tarGy<=9;
        auto hinKey = std::make_pair((tarGx+begGx)/2,(tarGy+begGy)/2);
        bool noHinder = pieces_map_.count(hinKey) == 0;
        LOG_INFO("[situator] stepValid=" <<int(stepValid) << ", inbound=" << int(inbound) << ", noHinder=" << int(noHinder));
        return stepValid && inbound && noHinder;
    }
    else if(chc == 'b')
    {
        bool stepValid = (std::abs(tarGx-begGx) == 2) && (std::abs(tarGy-begGy) == 2);
        bool inbound = 0<=tarGy && tarGy<=4;
        auto hinKey = std::make_pair((tarGx+begGx)/2,(tarGy+begGy)/2);
        bool noHinder = pieces_map_.count(hinKey) == 0;
        return stepValid && inbound && noHinder;
    }
    else if(chc == 'N' || chc == 'n')
    {
        int dx = std::abs(tarGx-begGx);
        int dy = std::abs(tarGy-begGy);
        bool stepValid = (dx == 1 && dy == 2) || (dx == 2 && dy == 1);
        bool noHinder = true;
        if(dx == 1 && dy == 2)
        {
            auto hinKey = std::make_pair(begGx,(tarGy+begGy)/2);
            noHinder = 0 == pieces_map_.count(hinKey);
        }
        else if(dx == 2 && dy == 1)
        {
            auto hinKey = std::make_pair((tarGx+begGx)/2,begGy);
            noHinder = 0 == pieces_map_.count(hinKey);
        }
        return stepValid && noHinder;
    }
    else if(chc == 'R' || chc == 'r')
    {
        int dx = std::abs(tarGx-begGx);
        int dy = std::abs(tarGy-begGy);
        bool stepValid = (dx == 0 || dy == 0);
        bool noHinder = true;
        /// Lateral movement
        if(dx == 0 && dy > 1)
        {
            int sign = begGy < tarGy ? 1 : -1;
            for(int i = 1; i != dy; ++i)
            {
                auto hinKey = std::make_pair(begGx, begGy + i * sign);
                noHinder &= (0 == pieces_map_.count(hinKey));
            }
        }
        /// Vertical movement
        else if(dy == 0 && dx > 1)
        {
            int sign = begGx < tarGx ? 1 : -1;
            for(int i = 1; i != dx; ++i)
            {
                auto hinKey = std::make_pair(begGx + i * sign, begGy);
                noHinder &= (0 == pieces_map_.count(hinKey));
            }
        }
        return stepValid && noHinder;
    }
    else if(chc == 'C' || chc == 'c')
    {
        int dx = std::abs(tarGx-begGx);
        int dy = std::abs(tarGy-begGy);
        bool stepValid = (dx == 0 || dy == 0);
        int numHinder = 0;
        /// Lateral movement
        if(dx == 0 && dy > 1)
        {
            int sign = begGy < tarGy ? 1 : -1;
            for(int i = 1; i != dy; ++i)
            {
                auto hinKey = std::make_pair(begGx, begGy + i * sign);
                numHinder += (1 == pieces_map_.count(hinKey));
            }
        }
        /// Vertical movement
        else if(dy == 0 && dx > 1)
        {
            int sign = begGx < tarGx ? 1 : -1;
            for(int i = 1; i != dx; ++i)
            {
                auto hinKey = std::make_pair(begGx + i * sign, begGy);
                numHinder += (1 == pieces_map_.count(hinKey));
            }
        }
        bool valid = stepValid && (
            capture && (numHinder == 1) || !capture && (numHinder == 0));
        LOG_INFO("[situator] stepValid=" <<int(stepValid) << ", capture=" << int(capture) << ", numHinder=" << numHinder << ", valid=" << int(valid));
        return valid;
    }
    else if(chc == 'P')
    {
        int dx = std::abs(tarGx-begGx);
        int dy = std::abs(tarGy-begGy);
        bool stepValid = (dx + dy) == 1 && tarGy <= begGy;
        return stepValid;
    }
    else if(chc == 'p')
    {
        int dx = std::abs(tarGx-begGx);
        int dy = std::abs(tarGy-begGy);
        bool stepValid = (dx + dy) == 1 && tarGy >= begGy;
        return stepValid;
    }
}


/// human_move eg: h2e2, return [bestmove h7e7] or [nobestmove]
std::string ChessManager::UcciAnswer(const std::string& human_move)
{
    // - update pieces_map by human_move
    int humGridX0 = CvtUcciXToGrid(human_move[0]);
    int humGridY0 = CvtUcciYToGrid(human_move[1]);
    int humGridX1 = CvtUcciXToGrid(human_move[2]);
    int humGridY1 = CvtUcciYToGrid(human_move[3]);
    auto fromKey = std::make_pair(humGridX0, humGridY0);
    auto toKey = std::make_pair(humGridX1, humGridY1);
    pieces_map_[toKey] = pieces_map_[fromKey];
    pieces_map_.erase(fromKey);
    pieces_map_[toKey]->grid_x = humGridX1;
    pieces_map_[toKey]->grid_y = humGridY1;
    // - construct ucci's instructions
    std::string instrc  = ucci_instructions_.back() + " " + human_move;
    std::pair<std::string,bool> queryAns = ucci_engine_->Query(instrc);
    std::string& ucciAns = queryAns.first;
    bool has_score_mate_1 = queryAns.second; 
    // - extract answer
    // - eleeye(resign=nobestnove), stockfish(resign=bestmove (none))
    bool resign = ucciAns.find("nobestmove") != ucciAns.npos || ucciAns.find("bestmove (none)") != ucciAns.npos;
    bool win = has_score_mate_1 && ucciAns.find("ponder") == ucciAns.npos;
    int gridX0 = -1, gridY0 = -1, gridX1 = -1, gridY1 = -1;
    bool is_mate = false;
    std::string motionType = "move"; 
    if(!resign && ucciAns.find("bestmove") != ucciAns.npos)
    {
        // - update instructions history
        std::string bestmove = ucciAns.substr(ucciAns.find("bestmove") + 9, 4);
        LOG_INFO("[situator] bestmove=" << bestmove);
        ucci_instructions_.emplace_back(instrc + " " + bestmove);
        gridX0 = CvtUcciXToGrid(bestmove[0]);
        gridY0 = CvtUcciYToGrid(bestmove[1]);
        gridX1 = CvtUcciXToGrid(bestmove[2]);
        gridY1 = CvtUcciYToGrid(bestmove[3]);
        // - update pieces_map by robot_move
        fromKey = std::make_pair(gridX0, gridY0);
        toKey = std::make_pair(gridX1, gridY1);
        motionType = pieces_map_.count(toKey) ? "eat" : "move";
        pieces_map_[toKey] = pieces_map_[fromKey];
        pieces_map_.erase(fromKey);
        pieces_map_[toKey]->grid_x = gridX1;
        pieces_map_[toKey]->grid_y = gridY1;
        // - check toKey mate
        is_mate = IsMate();
    }
    nlohmann::json answer =
    {
        {"resign",   resign},
        {"win",      win},
        {"mate",     is_mate},
        {"type",     motionType},
        {"bestmove", { {gridX0, gridY0}, {gridX1, gridY1} } }
    };

    return std::string(answer.dump());
}


// - Check if the kings are attcked 
bool ChessManager::IsMate()
{
    /// find the opponent's king 
    std::vector<Piece::Ptr> kings;
    for(auto& pr : pieces_map_)
    {
        char chc = pr.second->charactor;
        if(chc == 'k' || chc == 'K')
            kings.push_back(pr.second);
    }

    /// check king is attacked.
    for(auto& pr : pieces_map_)
    {
        auto atkPiece = pr.second;
        for(auto kPiece : kings)
        {
            if(atkPiece->EqualTo(kPiece)) continue;
            int  tarGx = kPiece->grid_x;
            int  tarGy = kPiece->grid_y;
            LOG_INFO("[situator] IsMate: attack_piece=" << atkPiece->charactor);
            bool kingAttacked = NextStepAvalid(atkPiece, tarGx, tarGy);
            LOG_INFO("[situator] IsMate: kingAttacked=" << kingAttacked);
            if(kingAttacked) return true;
        }
    }
    return false;
}


// - convert grid-XY to ucci-XY
char ChessManager::CvtGridXToUcci(int grid_x)
{
    return grid_x + 'a';
}


// - convert grid-XY to ucci-XY
char ChessManager::CvtGridYToUcci(int grid_y)
{
    return grid_rows_ - 1 - grid_y + '0';
}


// - convert ucci-XY to grid-XY
int ChessManager::CvtUcciXToGrid(char ucci_x)
{
    return ucci_x - 'a';
}


// - convert ucci-XY to grid-XY
int ChessManager::CvtUcciYToGrid(char ucci_y)
{
    return grid_rows_ - 1 - ucci_y + '0';
}


// - Set Robot skill level
void ChessManager::SetAiLevel(const int ai_level)
{
    ucci_engine_->SetAiLevel(ai_level);
}


