#include "tuning.hpp"

namespace Tuning {

    EvalScore PawnValue   = S(100, 100);
    EvalScore KnightValue = S(320, 330);
    EvalScore BishopValue = S(330, 340);
    EvalScore RookValue   = S(500, 520);
    EvalScore QueenValue  = S(950, 1000);

    EvalScore BishopPairBonus       = S(20, 50);
    EvalScore RookOpenFileBonus     = S(40, 20);
    EvalScore RookSemiOpenFileBonus = S(20, 10);
    EvalScore RookOnSeventhBonus    = S(20, 40);
    EvalScore KnightOutpostBonus    = S(30, 20);

    EvalScore IsolatedPawnPenalty   = S(-10, -20);
    EvalScore DoubledPawnPenalty    = S(-10, -20);
    EvalScore BackwardPawnPenalty   = S(-5,  -10);
    EvalScore ConnectedPawnBonus    = S(15, 15);
    EvalScore PhalanxBonus          = S(10, 20);

    int KingSafetyWeight = 90;

    void init() {
    }

}
