#include "SerialMRVSolver.h"

SerialMRVSolver::SerialMRVSolver() = default;

void SerialMRVSolver::solve(Board &board) {
    solveGrid(board);
}

bool SerialMRVSolver::solveGrid(Board &board) {
    int row = -1;
    int col = -1;

    if (!findMRVCell(board, row, col)) {
        return true;
    }

    for (int value = 1; value <= static_cast<int>(CommonConstants::BoardSize); value++) {
        if (!isValid(row, col, value, board)) {
            continue;
        }

        board.setBoardValue(row, col, value);

        if (solveGrid(board)) {
            return true;
        }

        board.resetBoardBlock(row, col);
    }

    return false;
}

bool SerialMRVSolver::isValid(const int row, const int col, const int value, const Board &board) {
    constexpr int gridSize = static_cast<int>(CommonConstants::GridSize);

    const int startingRow = (row / gridSize) * gridSize;
    const int startingCol = (col / gridSize) * gridSize;
    const int endingRow = startingRow + gridSize;
    const int endingCol = startingCol + gridSize;

    return isValidInGrid(startingRow, startingCol, endingRow, endingCol, value, board) &&
           isValidInRow(row, value, board) && isValidInCol(col, value, board);
}

bool SerialMRVSolver::isValidInGrid(const int startingRow, const int startingCol, const int endingRow,
                                    const int endingCol, const int value, const Board &board) {
    for (int r = startingRow; r < endingRow; r++) {
        for (int c = startingCol; c < endingCol; c++) {
            if (board.getBoardValue(r, c) == value) {
                return false;
            }
        }
    }
    return true;
}

bool SerialMRVSolver::isValidInRow(const int row, const int value, const Board &board) {
    for (int c = 0; c < static_cast<int>(CommonConstants::BoardSize); c++) {
        if (board.getBoardValue(row, c) == value) {
            return false;
        }
    }
    return true;
}

bool SerialMRVSolver::isValidInCol(const int col, const int value, const Board &board) {
    for (int r = 0; r < static_cast<int>(CommonConstants::BoardSize); r++) {
        if (board.getBoardValue(r, col) == value) {
            return false;
        }
    }
    return true;
}

int SerialMRVSolver::countCandidates(const int row, const int col, const Board &board) {
    int count = 0;
    for (int val = 1; val <= static_cast<int>(CommonConstants::BoardSize); val++) {
        if (isValid(row, col, val, board)) count++;
    }
    return count;
}

bool SerialMRVSolver::findMRVCell(const Board &board, int &row, int &col) {
    constexpr int boardSize = static_cast<int>(CommonConstants::BoardSize);
    int minCandidates = boardSize + 1;
    row = -1;
    col = -1;

    for (int r = 0; r < boardSize; r++) {
        for (int c = 0; c < boardSize; c++) {
            if (board.getBoardBlock(r, c).getIsFilled()) continue;

            const int candidates = countCandidates(r, c, board);
            if (candidates < minCandidates) {
                minCandidates = candidates;
                row = r;
                col = c;
                if (minCandidates == 0) return true; // Dead end — fail fast, no point scanning further
            }
        }
    }

    return row != -1; // false = no empty cells = board is solved
}
