#ifndef PDC_FINALPROJECT_PUZZLEPROFILER_H
#define PDC_FINALPROJECT_PUZZLEPROFILER_H
#pragma once

#include <string>
#include "../../SudokuBoard/Board/Board.h"
using namespace std;

class PuzzleProfiler {
public:
    static long long countNodes(const Board &board);

    // Reads puzzles from filePath (one per line, 81 chars, '0' = empty).
    // Prints puzzles whose node count falls within [minNodes, maxNodes].
    static void profileFromFile(const string &filePath, long long minNodes, long long maxNodes);

private:
    static bool isValidInGrid(int startingRow, int startingCol, int endingRow, int endingCol, int value,
                              const Board &board);

    static bool isValidInRow(int row, int value, const Board &board);

    static bool isValidInCol(int col, int value, const Board &board);

    static bool isValid(int row, int col, int value, const Board &board);

    static int countCandidates(int row, int col, const Board &board);

    // Returns false when no empty cell exists (board is solved).
    // Exits early with the dead-end cell when a cell has 0 candidates (fail fast).
    static bool findMRVCell(const Board &board, int &row, int &col);

    static bool solveAndCount(Board &board, long long &nodes);

    static Board parseFromLine(string &line);
};

#endif //PDC_FINALPROJECT_PUZZLEPROFILER_H
