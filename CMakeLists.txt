cmake_minimum_required(VERSION 3.15)
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
        src/BoardGenerator/PuzzleBank.h
        src/BoardSolver/Solver.h
        src/BoardSolver/SerialSolver/SerialSolver.cpp
        src/BoardSolver/SerialSolver/SerialSolver.h
        src/BoardSolver/OpenMPSolver/OpenMPSolver.cpp
        src/BoardSolver/OpenMPSolver/OpenMPSolver.h
        src/BoardSolver/DLXSolver/DLXSolver.cpp
        src/BoardSolver/DLXSolver/DLXSolver.h
        src/CorrectnessChecker/CorrectnessChecker.cpp
        src/CorrectnessChecker/CorrectnessChecker.h
        src/Benchmark/BenchmarkRunner.cpp
        src/Benchmark/BenchmarkRunner.h
        src/Benchmark/PuzzleProfiler/PuzzleProfiler.h
        src/Benchmark/PuzzleProfiler/PuzzleProfiler.cpp
)

target_compile_definitions(PDC_FinalProject PRIVATE "PROJECT_ROOT=\"${CMAKE_SOURCE_DIR}\"")

option(HPC "Build with manual -fopenmp flags for HPC clusters" OFF)

if (HPC)
    message(STATUS "HPC mode: applying -fopenmp flags directly")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp")
elseif (APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/libomp;/usr/local/opt/libomp" ${CMAKE_PREFIX_PATH})
    find_package(OpenMP)
    if (OpenMP_CXX_FOUND)
        target_link_libraries(PDC_FinalProject PRIVATE OpenMP::OpenMP_CXX)
    else ()
        message(WARNING "OpenMP not found; the solver will build but run serially.")
    endif ()
else ()
    find_package(OpenMP)
    if (OpenMP_CXX_FOUND)
        target_link_libraries(PDC_FinalProject PRIVATE OpenMP::OpenMP_CXX)
    else ()
        message(WARNING "OpenMP not found; the solver will build but run serially.")
    endif ()
endif ()
