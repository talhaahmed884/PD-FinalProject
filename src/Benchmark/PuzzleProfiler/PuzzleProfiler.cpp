#include "PuzzleProfiler.h"

#include <fstream>
#include <iostream>
using namespace std;

long long PuzzleProfiler::countNodes(const Board &board) {
    Board copy = board;
    long long nodes = 0;
    solveAndCountMRV(copy, nodes);
    return nodes;
}

void PuzzleProfiler::profileFromFile(const string &filePath, const long long minNodes, const long long maxNodes) {
    ifstream file(filePath);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filePath << "\n";
        return;
    }

    cout << "Index,Nodes,Puzzle\n";

    string line;
    int index = 0;
    while (getline(file, line)) {
        if (index >= 100) {
            break;
        }

        if (line.find(' ') == string::npos) {
            continue;
        }

        const Board board = parseFromLine(line);
        const long long nodes = countNodes(board);

        if (nodes >= minNodes && nodes <= maxNodes) {
            cout << index << "," << nodes << "," << line << "\n";
            index++;
        }
    }
}

Board PuzzleProfiler::parseFromLine(string &line) {
    constexpr int boardSize = static_cast<int>(CommonConstants::BoardSize);

    // Format: "<hash> <81-char puzzle> <rating>"  — skip hash and rating
    const size_t puzzleStart = line.find(' ');
    string puzzle = (puzzleStart != string::npos) ? line.substr(puzzleStart + 1, boardSize * boardSize) : line;

    Board board;
    for (int pos = 0; pos < boardSize * boardSize; pos++) {
        const int val = puzzle[pos] - '0';
        if (val != 0) {
            board.setBoardValue(pos / boardSize, pos % boardSize, val);
        }
    }

    line = puzzle;

    return board;
}

bool PuzzleProfiler::isValidInGrid(const int startingRow, const int startingCol, const int endingRow,
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

bool PuzzleProfiler::isValidInRow(const int row, const int value, const Board &board) {
    for (int c = 0; c < static_cast<int>(CommonConstants::BoardSize); c++) {
        if (board.getBoardValue(row, c) == value) {
            return false;
        }
    }
    return true;
}

bool PuzzleProfiler::isValidInCol(const int col, const int value, const Board &board) {
    for (int r = 0; r < static_cast<int>(CommonConstants::BoardSize); r++) {
        if (board.getBoardValue(r, col) == value) {
            return false;
        }
    }
    return true;
}

bool PuzzleProfiler::isValid(const int row, const int col, const int value, const Board &board) {
    constexpr int gridSize = static_cast<int>(CommonConstants::GridSize);

    const int startingRow = (row / gridSize) * gridSize;
    const int startingCol = (col / gridSize) * gridSize;
    const int endingRow = startingRow + gridSize;
    const int endingCol = startingCol + gridSize;

    return isValidInGrid(startingRow, startingCol, endingRow, endingCol, value, board) &&
           isValidInRow(row, value, board) && isValidInCol(col, value, board);
}

int PuzzleProfiler::countCandidates(const int row, const int col, const Board &board) {
    int count = 0;
    for (int val = 1; val <= static_cast<int>(CommonConstants::BoardSize); val++) {
        if (isValid(row, col, val, board)) count++;
    }
    return count;
}

bool PuzzleProfiler::findMRVCell(const Board &board, int &row, int &col) {
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

bool PuzzleProfiler::solveAndCountMRV(Board &board, long long &nodes) {
    int row = -1;
    int col = -1;

    if (!findMRVCell(board, row, col)) {
        return true;
    }

    for (int value = 1; value <= static_cast<int>(CommonConstants::BoardSize); value++) {
        if (!isValid(row, col, value, board)) {
            continue;
        }

        ++nodes;
        board.setBoardValue(row, col, value);

        if (solveAndCountMRV(board, nodes)) {
            return true;
        }

        board.resetBoardBlock(row, col);
    }

    return false;
}

bool PuzzleProfiler::solveAndCount(Board &board, long long &nodes) {
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

                ++nodes;

                if (solveAndCount(board, nodes)) {
                    return true;
                }

                board.resetBoardBlock(row, col);
            }

            return false;
        }
    }

    return true;
}
