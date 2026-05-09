#include <iostream>

#include "BoardGenerator/BoardGenerator.h"
#include "BoardSolver/OpenMPSolver/OpenMPSolver.h"
#include "BoardSolver/SerialSolver/SerialSolver.h"
#include "SudokuBoard/Board/Board.h"
#include "CorrectnessChecker/CorrectnessChecker.h"
using namespace std;

int main() {
    auto board = Board();
    board.print();

    cout << endl;

    auto generator = BoardGenerator();
    generator.generateBoard(board);
    board.print();

    auto solver = OpenMPSolver();
    // auto solver = SerialSolver();
    solver.solve(board);

    cout << endl;

    board.print();

    cout << endl << "Is board solved with a valid solution: " << CorrectnessChecker::check(board) << endl;

    return 0;
}
