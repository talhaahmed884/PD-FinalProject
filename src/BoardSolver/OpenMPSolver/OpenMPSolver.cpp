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
    // flag to indicate if the solution has been found, a shared atomic variable to flag
    std::atomic<bool> solved(false);

#ifdef _OPENMP
    // Parallelize the solving process using OpenMP tasks.
    // Create threads to explore
#pragma omp parallel
    {
#pragma omp single nowait //Without single, all threads would start solving the grid independently resulting in wrong result



        {
            // Only ONE thread starts recursively solving the grid.
            // Other threads are immediately available for parallel tasks created by the first thread
            solveGridParallel(board, solved, 0);
        }
    }
#else
    solveGridSerial(board, solved);
#endif
}

static constexpr int TASK_DEPTH_CUTOFF = 3;

bool OpenMPSolver::solveGridParallel(Board &board, std::atomic<bool> &solved, const int depth) {
    int row = -1;
    int col = -1;

    if (!findMRVCell(board, row, col)) {
        solved = true;
        return true;
    }

    bool solvedHere = false;
    // Create a synchronization region for tasks created in this loop
#pragma omp taskgroup
    {
        // try all possible values for the current empty cell
        for (int value = 1; value <= static_cast<int>(CommonConstants::BoardSize); value++) {
            if (solved.load()) {
                continue;
            }

            if (!isValid(row, col, value, board)) {
                continue;
            }
            // Create new tasks for each valid value
            // Each task get a copy of the current board state and tries to solve it recursively
#pragma omp task firstprivate(row, col, value, depth) shared(board, solved, solvedHere)
            {
                // Create a local copy of the board for this task to explore
                Board candidate = board;
                candidate.setBoardValue(row, col, value);

                bool branchSolved;
                // Only spawn new tasks while below the cutoff depth; beyond that fall back to serial DFS
                if (depth + 1 < TASK_DEPTH_CUTOFF) {
                    branchSolved = solveGridParallel(candidate, solved, depth + 1);
                } else {
                    branchSolved = solveGridSerial(candidate, solved);
                }

                if (branchSolved) {
                    // Protects shared board update — only the first task to finish updates the board
#pragma omp critical
                    {
                        if (!solvedHere) {
                            board = candidate;
                            solvedHere = true;
                        }
                    }
                    // Signal remaining tasks to abandon their branches
                    solved = true;
                }
            }
        }
    }

    return solvedHere;
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
