#ifndef PDC_FINALPROJECT_BOARD_H
#define PDC_FINALPROJECT_BOARD_H
#pragma once

#include "../Block/Block.h"
#include "../CommonConstants.h"


class Board {
public:
    Board();

    static int getBoardSize();

    void setBoardValue(int row, int col, int value);

    [[nodiscard]] int getBoardValue(int row, int col) const;

    [[nodiscard]] Block getBoardBlock(int row, int col) const;

    void resetBoardBlock(int row, int col);

    void print() const;

private:
    Block board[static_cast<int>(CommonConstants::BoardSize)][static_cast<int>(CommonConstants::BoardSize)];
};


#endif //PDC_FINALPROJECT_BOARD_H
