#include "BoardGenerator.h"
#include "../SudokuBoard/CommonConstants.h"

BoardGenerator::BoardGenerator() : randomEngine(random_device{}()), indexDistribution(0, 8),
                                   blockDistribution(1, 9) {
}

void BoardGenerator::generateBoard(Board &board) {
    int clusesAdded = 0;

    while (clusesAdded < getNoOfClues()) {
        const int randomRowIndex = getRandomIndex();
        const int randomColumnIndex = getRandomIndex();

        const int randomClueValue = getRandomBlockValue();

        if (board.getBoardBlock(randomRowIndex, randomColumnIndex).getIsFilled()) {
            continue;
        }

        if (!isValidInRow(randomRowIndex, randomClueValue, board)) {
            continue;
        }

        if (!isValidInCol(randomColumnIndex, randomClueValue, board)) {
            continue;
        }

        board.setBoardValue(randomRowIndex, randomColumnIndex, randomClueValue);
        clusesAdded++;
    }
}

bool BoardGenerator::isValidInRow(const int row, const int clueValue, const Board &board) {
    for (int col = 0; col < static_cast<int>(CommonConstants::BoardSize); col++) {
        if (board.getBoardValue(row, col) == clueValue) {
            return false;
        }
    }
    return true;
}

bool BoardGenerator::isValidInCol(const int col, const int clueValue, const Board &board) {
    for (int row = 0; row < static_cast<int>(CommonConstants::BoardSize); row++) {
        if (board.getBoardValue(row, col) == clueValue) {
            return false;
        }
    }
    return true;
}

int BoardGenerator::getNoOfClues() {
    return static_cast<int>(CommonConstants::BoardClues);
}

int BoardGenerator::getRandomIndex() {
    return indexDistribution(randomEngine);
}

int BoardGenerator::getRandomBlockValue() {
    return blockDistribution(randomEngine);
}
