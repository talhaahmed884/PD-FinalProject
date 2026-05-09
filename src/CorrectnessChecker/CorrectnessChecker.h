#ifndef PDC_FINALPROJECT_CORRECTNESSCHECKER_H
#define PDC_FINALPROJECT_CORRECTNESSCHECKER_H

#include "../SudokuBoard/Board/Board.h"


class CorrectnessChecker {
public:
    CorrectnessChecker();

    static bool check(const Board &board);

private:
    static bool isRowValid(int row, const Board &board);

    static bool isColValid(int col, const Board &board);

    static bool isGridValid(int startingRow, int startingCol, int endingRow, int endingCol, const Board &board);
};


#endif //PDC_FINALPROJECT_CORRECTNESSCHECKER_H
