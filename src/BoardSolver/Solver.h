#ifndef PDC_FINALPROJECT_SOLVER_H
#define PDC_FINALPROJECT_SOLVER_H
#pragma once

#include "../SudokuBoard/Board/Board.h"

class Solver {
public:
    virtual ~Solver() = default;

    virtual void solve(Board &board) = 0;
};

#endif //PDC_FINALPROJECT_SOLVER_H
