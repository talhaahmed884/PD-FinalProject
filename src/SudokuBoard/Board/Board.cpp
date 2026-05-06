#include "Board.h"

#include <iostream>
#include <ostream>
#include <iomanip>
using namespace std;

Board::Board() = default;

int Board::getBoardSize() {
    return static_cast<int>(CommonConstants::BoardSize);
}

void Board::setBoardValue(const int row, const int col, const int value) {
    this->board[row][col].setBlockValue(value);
}

int Board::getBoardValue(const int row, const int col) const {
    return this->board[row][col].getBlockValue();
}

Block Board::getBoardBlock(const int row, const int col) const {
    return this->board[row][col];
}

void Board::resetBoardBlock(const int row, const int col) {
    this->board[row][col].resetBlock();
}

void Board::print() const {
    for (const auto &row: this->board) {
        for (auto col: row) {
            cout << setw(2) << "|";
            col.print();
        }
        cout << setw(2) << "|";
        cout << endl;
    }
}
