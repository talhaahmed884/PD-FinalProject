#include "SerialSolver.h"

#include <iostream>
using namespace std;

SerialSolver::SerialSolver() = default;

void SerialSolver::solve(Board &board) {
    solveGrid(board);
}

bool SerialSolver::solveGrid(Board &board) {
    for (int row = 0; row < static_cast<int>(CommonConstants::BoardSize); row++) {
        for (int col = 0; col < static_cast<int>(CommonConstants::BoardSize); col++) {
            if (board.getBoardBlock(row, col).getIsFilled()) {
                continue;
            }

            for (int val = 1; val <= static_cast<int>(CommonConstants::BoardSize); val++) {
                if (!isValid(row, col, val, board)) {
                    continue;
                }

                board.setBoardValue(row, col, val);

                if (solveGrid(board)) {
                    return true;
                }

                board.resetBoardBlock(row, col);
            }

            return false;
        }
    }

    return true;
}

bool SerialSolver::isValid(const int row, const int column, const int value, const Board &board) {
    constexpr int gridSize = static_cast<int>(CommonConstants::GridSize);

    const int startingRow = (row / gridSize) * gridSize;
    const int startingCol = (column / gridSize) * gridSize;
    const int endingRow = startingRow + gridSize;
    const int endingCol = startingCol + gridSize;

    return isValidInGrid(startingRow, startingCol, endingRow, endingCol, value, board) && isValidInRow(
               row, value, board) && isValidInCol(column, value, board);
}

bool SerialSolver::isValidInGrid(const int startingRow, const int startingCol, const int endingRow, const int endingCol,
                                 const int value, const Board &board) {
    for (int row = startingRow; row < endingRow; row++) {
        for (int col = startingCol; col < endingCol; col++) {
            if (board.getBoardValue(row, col) == value) {
                return false;
            }
        }
    }
    return true;
}

bool SerialSolver::isValidInRow(const int row, const int value, const Board &board) {
    for (int col = 0; col < static_cast<int>(CommonConstants::BoardSize); col++) {
        if (board.getBoardValue(row, col) == value) {
            return false;
        }
    }
    return true;
}

bool SerialSolver::isValidInCol(const int col, const int value, const Board &board) {
    for (int row = 0; row < static_cast<int>(CommonConstants::BoardSize); row++) {
        if (board.getBoardValue(row, col) == value) {
            return false;
        }
    }
    return true;
}
