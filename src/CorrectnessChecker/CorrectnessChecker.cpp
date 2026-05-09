#include "CorrectnessChecker.h"

#include <iostream>
using namespace std;

#include "../SudokuBoard/CommonConstants.h"

CorrectnessChecker::CorrectnessChecker() = default;

bool CorrectnessChecker::check(const Board &board) {
    for (int a = 0; a < static_cast<int>(CommonConstants::BoardSize); a++) {
        if (!isRowValid(a, board) || !isColValid(a, board)) {
            return false;
        }
    }

    for (int a = 0; a < static_cast<int>(CommonConstants::BoardSize); a += 3) {
        for (int b = 0; b < static_cast<int>(CommonConstants::BoardSize); b += 3) {
            if (!isGridValid(a, b, a + 3, b + 3, board)) {
                return false;
            }
        }
    }

    return true;
}

bool CorrectnessChecker::isRowValid(const int row, const Board &board) {
    bool rowSet[10] = {};
    for (int a = 0; a < static_cast<int>(CommonConstants::BoardSize); a++) {
        if (!board.getBoardBlock(row, a).getIsFilled()) {
            return false;
        }

        const int value = board.getBoardValue(row, a);
        if (value < 1 || value > static_cast<int>(CommonConstants::BoardSize)) {
            return false;
        }
        if (rowSet[value]) {
            return false;
        }
        rowSet[value] = true;
    }

    for (int a = 1; a < 10; a++) {
        if (!rowSet[a]) {
            return false;
        }
    }

    return true;
}

bool CorrectnessChecker::isColValid(const int col, const Board &board) {
    bool colSet[10] = {};
    for (int a = 0; a < static_cast<int>(CommonConstants::BoardSize); a++) {
        if (!board.getBoardBlock(a, col).getIsFilled()) {
            return false;
        }

        const int value = board.getBoardValue(a, col);
        if (value < 1 || value > static_cast<int>(CommonConstants::BoardSize)) {
            return false;
        }
        if (colSet[value]) {
            return false;
        }
        colSet[value] = true;
    }

    for (int a = 1; a < 10; a++) {
        if (!colSet[a]) {
            return false;
        }
    }

    return true;
}

bool CorrectnessChecker::isGridValid(const int startingRow, const int startingCol, const int endingRow,
                                     const int endingCol, const Board &board) {
    bool gridSet[10] = {};

    for (int a = startingRow; a < endingRow; a++) {
        for (int b = startingCol; b < endingCol; b++) {
            if (!board.getBoardBlock(a, b).getIsFilled()) {
                return false;
            }

            const int value = board.getBoardValue(a, b);
            if (value < 1 || value > static_cast<int>(CommonConstants::BoardSize)) {
                return false;
            }
            if (gridSet[value]) {
                return false;
            }
            gridSet[value] = true;
        }
    }

    for (int a = 1; a < 10; a++) {
        if (!gridSet[a]) {
            return false;
        }
    }

    return true;
}
