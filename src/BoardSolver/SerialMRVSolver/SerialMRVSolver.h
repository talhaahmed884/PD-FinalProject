#ifndef PDC_FINALPROJECT_SERIALMRVSOLVER_H
#define PDC_FINALPROJECT_SERIALMRVSOLVER_H
#pragma once

#include "../Solver.h"

class SerialMRVSolver : public Solver {
public:
    SerialMRVSolver();

    void solve(Board &board) override;

private:
    static bool solveGrid(Board &board);

    static bool isValid(int row, int col, int value, const Board &board);

    static bool isValidInGrid(int startingRow, int startingCol, int endingRow, int endingCol, int value,
                              const Board &board);

    static bool isValidInRow(int row, int value, const Board &board);

    static bool isValidInCol(int col, int value, const Board &board);

    static int countCandidates(int row, int col, const Board &board);

    // Returns false when no empty cell exists (board is solved).
    // Exits early with the dead-end cell when a cell has 0 candidates (fail fast).
    static bool findMRVCell(const Board &board, int &row, int &col);
};

#endif //PDC_FINALPROJECT_SERIALMRVSOLVER_H
