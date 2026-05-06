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

private:
    mt19937 randomEngine;
    uniform_int_distribution<int> indexDistribution;
    uniform_int_distribution<int> blockDistribution;

    int getRandomIndex();

    int getRandomBlockValue();

    static bool isValidInGrid(int row, int column, int value, const Board &board);

    static bool isValidInRow(int row, int clueValue, const Board &board);

    static bool isValidInCol(int col, int clueValue, const Board &board);
};


#endif //PDC_FINALPROJECT_BOARDGENERATOR_H
