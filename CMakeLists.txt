cmake_minimum_required(VERSION 4.2)
project(PDC_FinalProject)

set(CMAKE_CXX_STANDARD 11)

add_executable(PDC_FinalProject src/main.cpp
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
        src/BoardSolver/OpenMPSolver/OpenMPSolver.cpp
        src/BoardSolver/OpenMPSolver/OpenMPSolver.h
        src/CorrectnessChecker/CorrectnessChecker.cpp
        src/CorrectnessChecker/CorrectnessChecker.h
)

if (APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/libomp;/usr/local/opt/libomp" ${CMAKE_PREFIX_PATH})
else ()
    message(WARNING "OpenMP not found; the solver will build but run serially.")
endif ()

find_package(OpenMP)
if (OpenMP_CXX_FOUND)
    target_link_libraries(PDC_FinalProject PRIVATE OpenMP::OpenMP_CXX)
else ()
    message(WARNING "OpenMP not found; the solver will build but run serially.")
endif ()
