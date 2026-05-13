#include "OpenMPFrontierSolver.h"

#ifdef _OPENMP
#include <omp.h>
#endif

static constexpr int FRONTIER_DEPTH = 2;

OpenMPFrontierSolver::OpenMPFrontierSolver(const int maxThreads) : maxThreads(maxThreads) {
#ifdef _OPENMP
    if (this->maxThreads > 0) {
        omp_set_num_threads(this->maxThreads);
    }
#endif
}

void OpenMPFrontierSolver::solve(Board &board) {
    vector<Board> frontier;
    buildFrontier(board, frontier, 0, FRONTIER_DEPTH);

    atomic<bool> solved(false);
    Board solutionBoard;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) shared(frontier, solutionBoard, solved)
    for (int i = 0; i < static_cast<int>(frontier.size()); i++) {
        if (!solved.load()) {
            Board candidate = frontier[i];
            if (solveGridSerial(candidate, solved)) {
#pragma omp critical
                {
                    if (!solved.exchange(true)) {
                        solutionBoard = candidate;
                    }
                }
            }
        }
    }
#else
    for (int i = 0; i < static_cast<int>(frontier.size()); i++) {
        Board candidate = frontier[i];
        if (solveGridSerial(candidate, solved)) {
            solved = true;
            solutionBoard = candidate;
            break;
        }
    }
#endif

    if (solved.load()) {
        board = solutionBoard;
    }
}

void OpenMPFrontierSolver::buildFrontier(const Board &board, vector<Board> &frontier, const int depth,
                                         const int maxDepth) {
    int row = -1;
    int col = -1;

    if (depth >= maxDepth || !findMRVCell(board, row, col)) {
        frontier.push_back(board);
        return;
    }

    const vector<int> values = getValidValues(row, col, board);
    for (int value: values) {
        Board nextBoard = board;
        nextBoard.setBoardValue(row, col, value);
        buildFrontier(nextBoard, frontier, depth + 1, maxDepth);
    }
}

bool OpenMPFrontierSolver::solveGridSerial(Board &board, atomic<bool> &solved) {
    if (solved.load()) {
        return false;
    }

    int row = -1;
    int col = -1;

    if (!findMRVCell(board, row, col)) {
        return true;
    }

    const vector<int> values = getValidValues(row, col, board);
    for (int value: values) {
        if (solved.load()) {
            return false;
        }

        board.setBoardValue(row, col, value);

        if (solveGridSerial(board, solved)) {
            return true;
        }

        board.resetBoardBlock(row, col);
    }

    return false;
}

bool OpenMPFrontierSolver::findMRVCell(const Board &board, int &row, int &col) {
    constexpr int boardSize = static_cast<int>(CommonConstants::BoardSize);
    int minCandidates = boardSize + 1;
    row = -1;
    col = -1;

    for (int r = 0; r < boardSize; r++) {
        for (int c = 0; c < boardSize; c++) {
            if (board.getBoardBlock(r, c).getIsFilled()) {
                continue;
            }

            const int candidates = countCandidates(r, c, board);
            if (candidates < minCandidates) {
                minCandidates = candidates;
                row = r;
                col = c;
                if (minCandidates == 0) {
                    return true;
                }
            }
        }
    }

    return row != -1;
}

vector<int> OpenMPFrontierSolver::getValidValues(const int row, const int col, const Board &board) {
    vector<int> values;
    for (int value = 1; value <= static_cast<int>(CommonConstants::BoardSize); value++) {
        if (isValid(row, col, value, board)) {
            values.push_back(value);
        }
    }
    return values;
}

int OpenMPFrontierSolver::countCandidates(const int row, const int col, const Board &board) {
    return static_cast<int>(getValidValues(row, col, board).size());
}

bool OpenMPFrontierSolver::isValid(const int row, const int col, const int value, const Board &board) {
    constexpr int gridSize = static_cast<int>(CommonConstants::GridSize);

    const int startingRow = (row / gridSize) * gridSize;
    const int startingCol = (col / gridSize) * gridSize;
    const int endingRow = startingRow + gridSize;
    const int endingCol = startingCol + gridSize;

    return isValidInGrid(startingRow, startingCol, endingRow, endingCol, value, board) &&
           isValidInRow(row, value, board) && isValidInCol(col, value, board);
}

bool OpenMPFrontierSolver::isValidInGrid(const int startingRow, const int startingCol, const int endingRow,
                                         const int endingCol, const int value, const Board &board) {
    for (int row = startingRow; row < endingRow; row++) {
        for (int col = startingCol; col < endingCol; col++) {
            if (board.getBoardValue(row, col) == value) {
                return false;
            }
        }
    }
    return true;
}

bool OpenMPFrontierSolver::isValidInRow(const int row, const int value, const Board &board) {
    for (int col = 0; col < static_cast<int>(CommonConstants::BoardSize); col++) {
        if (board.getBoardValue(row, col) == value) {
            return false;
        }
    }
    return true;
}

bool OpenMPFrontierSolver::isValidInCol(const int col, const int value, const Board &board) {
    for (int row = 0; row < static_cast<int>(CommonConstants::BoardSize); row++) {
        if (board.getBoardValue(row, col) == value) {
            return false;
        }
    }
    return true;
}
