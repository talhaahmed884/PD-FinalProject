#ifndef PDC_FINALPROJECT_BOARDGENERATOR_H
#define PDC_FINALPROJECT_BOARDGENERATOR_H
#pragma once

#include <random>

#include "../SudokuBoard/Board/Board.h"
using namespace std;

class BoardGenerator {
public:
    BoardGenerator();

    void generateBoard(Board &board);

    static int getNoOfClues();

    static vector<Board> loadProblems(int count, int difficultyLevel);

private:
    mt19937 randomEngine;

    bool generateFullBoard(Board &board);

    static void countSolutions(Board &board, int &count);

    static bool isValidInGrid(int row, int column, int value, const Board &board);

    static bool isValidInRow(int row, int clueValue, const Board &board);

    static bool isValidInCol(int col, int clueValue, const Board &board);
};


#endif //PDC_FINALPROJECT_BOARDGENERATOR_H
