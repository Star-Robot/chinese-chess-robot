#ifndef chess_manager_hpp__
#define chess_manager_hpp__


#include <map>
#include <memory>
#include <string>
#include <yaml-cpp/yaml.h>


struct Piece
{
    typedef std::shared_ptr<Piece> Ptr;
    char  charactor = '\0';
    float pred_x    = -1.f;
    float pred_y    = -1.f;
    int   grid_x    = -1;
    int   grid_y    = -1;
    char  ucci_x    = '\0';
    char  ucci_y    = '\0';
    // - [0,1], lost, found
    uint8_t status  = 0;

    Piece(char chc,
          float predX, float predY,
          float gridX, float gridY,
          float ucciX, float ucciY)
    {
        charactor = chc;
        pred_x = predX;
        pred_y = predY;
        grid_x = gridX;
        grid_y = gridY;
        ucci_x = ucciX;
        ucci_y = ucciY;
    }

    bool EqualTo(Ptr other)
    {
        bool equl = charactor == other->charactor &&
                    pred_x == other->pred_x &&
                    pred_y == other->pred_y &&
                    grid_x == other->grid_x &&
                    grid_y == other->grid_y &&
                    ucci_x == other->ucci_x &&
                    ucci_y == other->ucci_y;
        return equl;
    }

    bool IsUpper()
    { return std::isupper(charactor); }

    bool IsLower()
    { return std::islower(charactor); }

};


class UcciEngine;
class ChessManager
{
public:
    ChessManager(YAML::Node& chess_cfg);

    // - Restore the layout of the pieces from the given game record
    void InitPieceMap(const std::string& formation);

    // - clear situator status, prepare for next update
    void Prepare();

    // - match each detection with old pieces's map
    void Read(float pred_x, float pred_y, const std::string& pred_c);

    // - Summarize the difference and judge whether the changes are reasonable
    std::string Summary(const std::string& round = "");

    // - Read in the actions of human players, UCCI will give countermeasures
    std::string UcciAnswer(const std::string& human_move);

    // - Check if the kings are attcked 
    bool IsMate();

    // - According to situator, judge whether the specified piece can reach the target.
    bool NextStepAvalid(Piece::Ptr piece, int tar_grid_x, int tar_grid_y);

    // - convert grid-XY to ucci-XY
    char CvtGridXToUcci(int grid_x);

    // - convert grid-XY to ucci-XY
    char CvtGridYToUcci(int grid_y);

    // - convert ucci-XY to grid-XY
    int CvtUcciXToGrid(char ucci_x);

    // - convert ucci-XY to grid-XY
    int CvtUcciYToGrid(char ucci_y);

    // - Set Robot skill level
    void SetAiLevel(const int ai_level);

public:
    /// !Note, key: (gridX,gridY)
    std::map<std::pair<int, int>, Piece::Ptr> pieces_map_;
    int   grid_cols_   = 9;
    int   grid_rows_   = 10;
    float offset_x_    = 0.f;
    float offset_y_    = 0.f;
    float grid_unit_x_ = 0.f;
    float grid_unit_y_ = 0.f;
    /// CNN detector, but not match
    std::map<std::pair<int, int>, Piece::Ptr> newcomers_;
    /// someone in pieces_map but has never been matched
    std::map<std::pair<int, int>, Piece::Ptr> lost_;
    /// cnn predict label map-to ucci label
    std::map<std::string, char> cnn_cls_to_ucci_;
    /// default chess formation
    std::string init_formation_;
    /// ucci instruction history
    std::vector<std::string> ucci_instructions_;
    /// UcciEngine ptr 
    std::shared_ptr<UcciEngine> ucci_engine_;
};


#endif//chess_manager_hpp_
