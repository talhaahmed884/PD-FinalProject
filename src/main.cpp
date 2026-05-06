#include <iostream>

#include "BoardGenerator/BoardGenerator.h"
#include "BoardSolver/SerialSolver/SerialSolver.h"
#include "SudokuBoard/Board/Board.h"
using namespace std;

int main() {
    Board board = Board();
    board.print();

    cout << endl;

    BoardGenerator generator = BoardGenerator();
    generator.generateBoard(board);
    board.print();

    SerialSolver solver = SerialSolver();
    solver.solve(board);

    cout << endl;

    board.print();

    return 0;
}
