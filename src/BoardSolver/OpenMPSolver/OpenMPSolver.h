#ifndef PDC_FINALPROJECT_OPENMPSOLVER_H
#define PDC_FINALPROJECT_OPENMPSOLVER_H
#pragma once

#include "../Solver.h"
#include <atomic>

class OpenMPSolver : public Solver {
public:
    explicit OpenMPSolver(int maxThreads = 0);

    void solve(Board &board) override;

private:
    static bool solveGridParallel(Board &board, std::atomic<bool> &solved, int depth);

    static bool solveGridSerial(Board &board, std::atomic<bool> &solved);

    static bool isValid(int row, int column, int value, const Board &board);

    static bool isValidInGrid(int startingRow, int startingCol, int endingRow, int endingCol, int value,
                              const Board &board);

    static bool isValidInRow(int row, int value, const Board &board);

    static bool isValidInCol(int col, int value, const Board &board);

    static int countCandidates(int row, int col, const Board &board);

    // Returns false when no empty cell exists (board is solved).
    // Exits early with the dead-end cell when a cell has 0 candidates (fail fast).
    static bool findMRVCell(const Board &board, int &row, int &col);

    int maxThreads;
};

#endif // PDC_FINALPROJECT_OPENMPSOLVER_H
