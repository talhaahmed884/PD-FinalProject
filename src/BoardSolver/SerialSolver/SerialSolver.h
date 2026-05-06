#ifndef PDC_FINALPROJECT_SERIALSOLVER_H
#define PDC_FINALPROJECT_SERIALSOLVER_H
#pragma once

#include "../Solver.h"

class SerialSolver : public Solver {
public:
    SerialSolver();

    void solve(Board &board) override;

private:
    static bool solveGrid(Board &board);

    static bool isValid(int row, int column, int value, const Board &board);

    static bool isValidInGrid(int startingRow, int startingCol, int endingRow, int endingCol, int value,
                              const Board &board);

    static bool isValidInRow(int row, int value, const Board &board);

    static bool isValidInCol(int col, int value, const Board &board);
};


#endif //PDC_FINALPROJECT_SERIALSOLVER_H
