#include <iostream>

#include "src/BoardGenerator/BoardGenerator.h"
#include "src/BoardSolver/Solver.h"
#include "src/BoardSolver/SerialSolver/SerialSolver.h"
#include "src/SudokuBoard/Board/Board.h"
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
