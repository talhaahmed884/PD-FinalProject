#include "BoardGenerator.h"
#include "../SudokuBoard/CommonConstants.h"
#include "PuzzleBank.h"

#include <algorithm>
#include <numeric>
#include <vector>
using namespace std;

BoardGenerator::BoardGenerator() : randomEngine(random_device{}()) {
}

void BoardGenerator::generateBoard(Board &board) {
    generateFullBoard(board);

    constexpr int boardSize = static_cast<int>(CommonConstants::BoardSize);
    constexpr int totalCells = boardSize * boardSize;

    vector<int> indices(totalCells);
    iota(indices.begin(), indices.end(), 0);
    shuffle(indices.begin(), indices.end(), randomEngine);

    int cluesRemaining = totalCells;

    for (const int idx: indices) {
        if (cluesRemaining <= getNoOfClues()) break;

        const int row = idx / boardSize;
        const int col = idx % boardSize;
        const int savedValue = board.getBoardValue(row, col);

        board.resetBoardBlock(row, col);

        int count = 0;
        countSolutions(board, count);

        if (count == 1) {
            cluesRemaining--;
        } else {
            board.setBoardValue(row, col, savedValue);
        }
    }
}

bool BoardGenerator::generateFullBoard(Board &board) {
    constexpr int boardSize = static_cast<int>(CommonConstants::BoardSize);

    for (int row = 0; row < boardSize; row++) {
        for (int col = 0; col < boardSize; col++) {
            if (board.getBoardBlock(row, col).getIsFilled()) continue;

            vector<int> values(boardSize);
            iota(values.begin(), values.end(), 1);
            shuffle(values.begin(), values.end(), randomEngine);

            for (const int val: values) {
                if (!isValidInGrid(row, col, val, board) || !isValidInRow(row, val, board) ||
                    !isValidInCol(col, val, board))
                    continue;

                board.setBoardValue(row, col, val);

                if (generateFullBoard(board)) return true;

                board.resetBoardBlock(row, col);
            }

            return false;
        }
    }

    return true;
}

void BoardGenerator::countSolutions(Board &board, int &count) {
    if (count >= 2) return;

    constexpr int boardSize = static_cast<int>(CommonConstants::BoardSize);

    for (int row = 0; row < boardSize; row++) {
        for (int col = 0; col < boardSize; col++) {
            if (board.getBoardBlock(row, col).getIsFilled()) continue;

            for (int val = 1; val <= boardSize; val++) {
                if (!isValidInGrid(row, col, val, board) || !isValidInRow(row, val, board) || !isValidInCol(col, val,
                        board))
                    continue;

                board.setBoardValue(row, col, val);
                countSolutions(board, count);
                board.resetBoardBlock(row, col);

                if (count >= 2) return;
            }

            return;
        }
    }

    count++;
}

bool BoardGenerator::isValidInGrid(const int row, const int column, const int value, const Board &board) {
    constexpr int gridSize = static_cast<int>(CommonConstants::GridSize);

    const int startingRow = (row / gridSize) * gridSize;
    const int startingCol = (column / gridSize) * gridSize;
    const int endingRow = startingRow + gridSize;
    const int endingCol = startingCol + gridSize;

    for (int a = startingRow; a < endingRow; a++) {
        for (int b = startingCol; b < endingCol; b++) {
            if (board.getBoardValue(a, b) == value) {
                return false;
            }
        }
    }
    return true;
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

vector<Board> BoardGenerator::loadProblems(const int count, const int difficultyLevel) {
    constexpr int boardSize = static_cast<int>(CommonConstants::BoardSize);
    vector<string> puzzleBank;

    if (difficultyLevel == 0) {
        puzzleBank = EASY_PUZZLE_BANK;
    } else if (difficultyLevel == 1) {
        puzzleBank = MEDIUM_PUZZLE_BANK;
    } else if (difficultyLevel == 2) {
        puzzleBank = HARD_PUZZLE_BANK;
    } else {
        puzzleBank = EXTREME_HARD_PUZZLE_BANK;
    }

    const int limit = min(count, static_cast<int>(puzzleBank.size()));

    vector<Board> boards;
    boards.reserve(limit);

    for (int i = 0; i < limit; i++) {
        Board board;
        const string &puzzle = puzzleBank[i];

        for (int pos = 0; pos < static_cast<int>(puzzle.size()); pos++) {
            if (puzzle[pos] != '.') {
                board.setBoardValue(pos / boardSize, pos % boardSize, puzzle[pos] - '0');
            }
        }

        boards.push_back(board);
    }

    return boards;
}
