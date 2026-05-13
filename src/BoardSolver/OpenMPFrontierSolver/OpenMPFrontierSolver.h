#ifndef PDC_FINALPROJECT_OPENMPFRONTIERSOLVER_H
#define PDC_FINALPROJECT_OPENMPFRONTIERSOLVER_H
#pragma once

#include "../Solver.h"

#include <atomic>
#include <vector>
using namespace std;

class OpenMPFrontierSolver : public Solver {
public:
    explicit OpenMPFrontierSolver(int maxThreads = 0);

    void solve(Board &board) override;

private:
    static void buildFrontier(const Board &board, vector<Board> &frontier, int depth, int maxDepth);

    static bool solveGridSerial(Board &board, atomic<bool> &solved);

    static bool findMRVCell(const Board &board, int &row, int &col);

    static vector<int> getValidValues(int row, int col, const Board &board);

    static int countCandidates(int row, int col, const Board &board);

    static bool isValid(int row, int col, int value, const Board &board);

    static bool isValidInGrid(int startingRow, int startingCol, int endingRow, int endingCol, int value,
                              const Board &board);

    static bool isValidInRow(int row, int value, const Board &board);

    static bool isValidInCol(int col, int value, const Board &board);

    int maxThreads;
};

#endif //PDC_FINALPROJECT_OPENMPFRONTIERSOLVER_H
