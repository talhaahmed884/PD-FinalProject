cmake_minimum_required(VERSION 4.2)
project(PDC_FinalProject)

set(CMAKE_CXX_STANDARD 26)

add_executable(PDC_FinalProject main.cpp
        src/SudokuBoard/Block/Block.cpp
        src/SudokuBoard/Block/Block.h
        src/SudokuBoard/Board/Board.cpp
        src/SudokuBoard/Board/Board.h
        src/BoardGenerator/BoardGenerator.cpp
        src/BoardGenerator/BoardGenerator.h
        src/SudokuBoard/CommonConstants.h
        src/BoardSolver/Solver.h
        src/BoardSolver/SerialSolver/SerialSolver.cpp
        src/BoardSolver/SerialSolver/SerialSolver.h
)
