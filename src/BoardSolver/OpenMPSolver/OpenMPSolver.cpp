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
            solveGridParallel(board, solved);
        }
    }
#else
    solveGridSerial(board, solved);
#endif
}

bool OpenMPSolver::solveGridParallel(Board &board, std::atomic<bool> &solved) {
    int row = -1;
    int col = -1;

    for (int r = 0; r < static_cast<int>(CommonConstants::BoardSize); r++) {
        for (int c = 0; c < static_cast<int>(CommonConstants::BoardSize); c++) {
            if (!board.getBoardBlock(r, c).getIsFilled()) {
                row = r;
                col = c;
                break;
            }
        }
        if (row != -1) {
            break;
        }
    }

    if (row == -1) {
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
#pragma omp task firstprivate(row, col, value) shared(board, solved, solvedHere)
            {
                // Create a local copy of the board for this task to explore
                Board candidate = board;
                candidate.setBoardValue(row, col, value);

                if (solveGridSerial(candidate, solved)) {
                    if (!solved.exchange(true)) {
                        // Protects shared board update
                        // Without this critical section, multiple threads could update the board simultaneously
#pragma omp critical
                        {
                            board = candidate;
                            solvedHere = true;
                        }
                    }
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

    for (int r = 0; r < static_cast<int>(CommonConstants::BoardSize); r++) {
        for (int c = 0; c < static_cast<int>(CommonConstants::BoardSize); c++) {
            if (!board.getBoardBlock(r, c).getIsFilled()) {
                row = r;
                col = c;
                break;
            }
        }
        if (row != -1) {
            break;
        }
    }

    if (row == -1) {
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
