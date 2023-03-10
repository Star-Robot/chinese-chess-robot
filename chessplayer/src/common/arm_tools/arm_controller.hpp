#ifndef arm_controller_hpp__
#define arm_controller_hpp__

#include <memory>
#include <vector>

class CommandManager;
class ArmController{
public:
    ArmController(bool enable_force = true);

    std::vector<int> CalVel(float dst_angle1, float dst_angle2);

    void Reset();

    void ResetHand();

    void DriveByAbsAngle(int arm_id, float angle, int speed = 40, bool is_SCS=false);

    void DriveByXY(float x, float y);

    void TakePiece();

    void DropPiece();

    void Check(int arm_id, float arm_angle, bool is_SCS=false);

    std::pair<float, float> ReadArmXY();

private:
    std::shared_ptr<CommandManager> mpComManager_;

}; // ArmController


#endif // arm_controller_hpp__

