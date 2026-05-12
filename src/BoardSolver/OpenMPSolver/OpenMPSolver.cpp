#include "OpenMPSolver.h"

#ifdef _OPENMP
#include <omp.h>
#endif

// Stores maximum thread count for OpenMP parallelization. 
// If set to 0, OpenMP will use the default number of threads
OpenMPSolver::OpenMPSolver(int maxThreads) : maxThreads(maxThreads) {
#ifdef _OPENMP
    if (this->maxThreads > 0) {
        omp_set_num_threads(this->maxThreads);
    }
#endif
}

void OpenMPSolver::solve(Board &board) {
    std::atomic<bool> solved(false);
    Board solutionBoard;

#ifdef _OPENMP
#pragma omp parallel
    {
#pragma omp single nowait
        {
            solveGridParallel(board, solutionBoard, solved, 0);
        }
    }
    if (solved.load()) {
        board = solutionBoard;
    }
#else
    solveGridSerial(board, solved);
#endif
}

static constexpr int TASK_DEPTH_CUTOFF = 3;

bool OpenMPSolver::solveGridParallel(const Board &board, Board &solutionBoard,
                                     std::atomic<bool> &solved, const int depth) {
    if (solved.load()) return false;

    int row = -1;
    int col = -1;

    if (!findMRVCell(board, row, col)) {
        // No empty cells — board is fully solved; claim the solution slot
#pragma omp critical
        {
            if (!solved.exchange(true)) {
                solutionBoard = board;
            }
        }
        return true;
    }

    // `board` is const from here on — tasks never write to it.
    // Each task gets a firstprivate copy of this snapshot to explore independently.
    const Board snapshot = board;

#pragma omp taskgroup
    {
        for (int value = 1; value <= static_cast<int>(CommonConstants::BoardSize); value++) {
            if (solved.load()) break;
            if (!isValid(row, col, value, snapshot)) continue;

#pragma omp task firstprivate(row, col, value, depth, snapshot) shared(solutionBoard, solved)
            {
                if (!solved.load()) {
                    Board candidate = snapshot;
                    candidate.setBoardValue(row, col, value);

                    if (depth + 1 < TASK_DEPTH_CUTOFF) {
                        // Recurse: inner call writes directly to solutionBoard when it finds a solution
                        solveGridParallel(candidate, solutionBoard, solved, depth + 1);
                    } else {
                        // Serial fallback: candidate is task-private, no sharing
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
            }
        }
    }

    return solved.load();
}

bool OpenMPSolver::solveGridSerial(Board &board, std::atomic<bool> &solved) {
    if (solved.load()) {
        return false;
    }

    int row = -1;
    int col = -1;

    if (!findMRVCell(board, row, col)) {
        return true;
    }

    for (int value = 1; value <= static_cast<int>(CommonConstants::BoardSize); value++) {
        if (solved.load()) {
            return false;
        }

        if (!isValid(row, col, value, board)) {
            continue;
        }

        board.setBoardValue(row, col, value);

        if (solveGridSerial(board, solved)) {
            return true;
        }

        board.resetBoardBlock(row, col);
    }

    return false;
}

int OpenMPSolver::countCandidates(const int row, const int col, const Board &board) {
    int count = 0;
    for (int val = 1; val <= static_cast<int>(CommonConstants::BoardSize); val++) {
        if (isValid(row, col, val, board)) count++;
    }
    return count;
}

bool OpenMPSolver::findMRVCell(const Board &board, int &row, int &col) {
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

bool OpenMPSolver::isValid(const int row, const int column, const int value, const Board &board) {
    constexpr int gridSize = static_cast<int>(CommonConstants::GridSize);

    const int startingRow = (row / gridSize) * gridSize;
    const int startingCol = (column / gridSize) * gridSize;
    const int endingRow = startingRow + gridSize;
    const int endingCol = startingCol + gridSize;

    return isValidInGrid(startingRow, startingCol, endingRow, endingCol, value, board) &&
           isValidInRow(row, value, board) && isValidInCol(column, value, board);
}

bool OpenMPSolver::isValidInGrid(const int startingRow, const int startingCol, const int endingRow,
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

bool OpenMPSolver::isValidInRow(const int row, const int value, const Board &board) {
    for (int c = 0; c < static_cast<int>(CommonConstants::BoardSize); c++) {
        if (board.getBoardValue(row, c) == value) {
            return false;
        }
    }
    return true;
}

bool OpenMPSolver::isValidInCol(const int col, const int value, const Board &board) {
    for (int r = 0; r < static_cast<int>(CommonConstants::BoardSize); r++) {
        if (board.getBoardValue(r, col) == value) {
            return false;
        }
    }
    return true;
}
