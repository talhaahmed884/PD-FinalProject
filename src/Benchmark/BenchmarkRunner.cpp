#include "BenchmarkRunner.h"
#include "../BoardGenerator/BoardGenerator.h"
#include "../BoardSolver/SerialSolver/SerialSolver.h"
#include "../BoardSolver/SerialMRVSolver/SerialMRVSolver.h"
#include "../BoardSolver/OpenMPSolver/OpenMPSolver.h"
#include "../BoardSolver/OpenMPFrontierSolver/OpenMPFrontierSolver.h"
#include "../BoardSolver/DLXSolver/DLXSolver.h"
#include "../CorrectnessChecker/CorrectnessChecker.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#endif
#include <vector>
using namespace std;

static constexpr int OPENMP_THREAD_COUNTS[] = {4, 8, 16};

static constexpr int REPETITIONS = 5;

struct DifficultyConfig {
    int level;
    const char *name;
    const char *prefix;
};

static constexpr DifficultyConfig DIFFICULTIES[] = {
    {0, "Easy", "easy"},
    {1, "Medium", "medium"},
    {2, "Hard", "hard"},
    {3, "Extreme", "extreme"},
};

string BenchmarkRunner::makeOutputPath(const string &outputDir, const string &prefix) {
    mkdir(outputDir.c_str(), 0755);
    const time_t now = time(nullptr);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", localtime(&now));
    return outputDir + "/" + prefix + "_" + buf + ".csv";
}

void BenchmarkRunner::printRow(const BenchmarkResult &r) {
    cout << fixed << setprecision(9)
            << r.boardId << ","
            << r.difficulty << ","
            << r.algorithm << ","
            << r.threads << ","
            << r.rep << ","
            << r.timeSec << ","
            << r.correct << "\n";
}

void BenchmarkRunner::run(const string &outputDir, const int puzzleCount) {
    runBenchmarks(outputDir, puzzleCount);
}

void BenchmarkRunner::runBenchmarks(const string &outputDir, const int puzzleCount) {
    const string outputPath = makeOutputPath(outputDir, "results");
    vector<BenchmarkResult> results;

    cout << "Board_ID,Difficulty,Algorithm,Threads,Rep,Time_s,Correct\n";

    for (const auto &diff: DIFFICULTIES) {
        const vector<Board> boards = BoardGenerator::loadProblems(puzzleCount, diff.level);

        for (int i = 0; i < static_cast<int>(boards.size()); i++) {
            ostringstream oss;
            oss << diff.prefix << "_" << setw(3) << setfill('0') << (i + 1);
            const string boardId = oss.str();

            for (const auto &r: benchmarkSerial(boards[i], boardId, diff.name)) {
                printRow(r);
                results.push_back(r);
            }

            for (const auto &r: benchmarkSerialMRV(boards[i], boardId, diff.name)) {
                printRow(r);
                results.push_back(r);
            }

            for (const auto &r: benchmarkDLX(boards[i], boardId, diff.name)) {
                printRow(r);
                results.push_back(r);
            }

            for (const int t: OPENMP_THREAD_COUNTS) {
                for (const auto &r: benchmarkDLXParallel(boards[i], boardId, diff.name, t)) {
                    printRow(r);
                    results.push_back(r);
                }
            }

            for (const int t: OPENMP_THREAD_COUNTS) {
                for (const auto &r: benchmarkOpenMPFrontier(boards[i], boardId, diff.name, t)) {
                    printRow(r);
                    results.push_back(r);
                }
            }

            for (const int t: OPENMP_THREAD_COUNTS) {
                for (const auto &r: benchmarkOpenMP(boards[i], boardId, diff.name, t)) {
                    printRow(r);
                    results.push_back(r);
                }
            }
        }
    }

    writeCsv(results, outputPath);
    cout << "\nResults written to " << outputPath << "\n";
}

vector<BenchmarkResult> BenchmarkRunner::benchmarkSerial(const Board &board, const string &boardId,
                                                         const string &difficulty) {
    SerialSolver solver;
    vector<BenchmarkResult> results;
    results.reserve(REPETITIONS);

    for (int rep = 1; rep <= REPETITIONS; rep++) {
        Board copy = board;
        const auto start = chrono::high_resolution_clock::now();
        solver.solve(copy);
        const auto end = chrono::high_resolution_clock::now();
        const double timeSec = chrono::duration<double>(end - start).count();
        const int correct = CorrectnessChecker::check(copy) ? 1 : 0;
        results.push_back({boardId, difficulty, "Serial", 1, rep, timeSec, correct});
    }
    return results;
}

vector<BenchmarkResult> BenchmarkRunner::benchmarkSerialMRV(const Board &board, const string &boardId,
                                                            const string &difficulty) {
    SerialMRVSolver solver;
    vector<BenchmarkResult> results;
    results.reserve(REPETITIONS);

    for (int rep = 1; rep <= REPETITIONS; rep++) {
        Board copy = board;
        const auto start = chrono::high_resolution_clock::now();
        solver.solve(copy);
        const auto end = chrono::high_resolution_clock::now();
        const double timeSec = chrono::duration<double>(end - start).count();
        const int correct = CorrectnessChecker::check(copy) ? 1 : 0;
        results.push_back({boardId, difficulty, "Serial-MRV", 1, rep, timeSec, correct});
    }
    return results;
}

vector<BenchmarkResult> BenchmarkRunner::benchmarkOpenMP(const Board &board, const string &boardId,
                                                         const string &difficulty, const int threads) {
    OpenMPSolver solver(threads);
    vector<BenchmarkResult> results;
    results.reserve(REPETITIONS);

    for (int rep = 1; rep <= REPETITIONS; rep++) {
        Board copy = board;
#ifdef _OPENMP
        const double start = omp_get_wtime();
        solver.solve(copy);
        const double timeSec = omp_get_wtime() - start;
#else
        const auto start = chrono::high_resolution_clock::now();
        solver.solve(copy);
        const double timeSec = chrono::duration<double>(chrono::high_resolution_clock::now() - start).count();
#endif
        const int correct = CorrectnessChecker::check(copy) ? 1 : 0;
        results.push_back({boardId, difficulty, "OpenMP", threads, rep, timeSec, correct});
    }
    return results;
}

vector<BenchmarkResult> BenchmarkRunner::benchmarkOpenMPFrontier(const Board &board, const string &boardId,
                                                                 const string &difficulty, const int threads) {
    OpenMPFrontierSolver solver(threads);
    vector<BenchmarkResult> results;
    results.reserve(REPETITIONS);

    for (int rep = 1; rep <= REPETITIONS; rep++) {
        Board copy = board;
#ifdef _OPENMP
        const double start = omp_get_wtime();
        solver.solve(copy);
        const double timeSec = omp_get_wtime() - start;
#else
        const auto start = chrono::high_resolution_clock::now();
        solver.solve(copy);
        const double timeSec = chrono::duration<double>(chrono::high_resolution_clock::now() - start).count();
#endif
        const int correct = CorrectnessChecker::check(copy) ? 1 : 0;
        results.push_back({boardId, difficulty, "OMP-Frontier", threads, rep, timeSec, correct});
    }
    return results;
}

vector<BenchmarkResult> BenchmarkRunner::benchmarkDLX(const Board &board, const string &boardId,
                                                       const string &difficulty) {
    DLXSolver solver;
    vector<BenchmarkResult> results;
    results.reserve(REPETITIONS);

    for (int rep = 1; rep <= REPETITIONS; rep++) {
        Board copy = board;
        const auto start = chrono::high_resolution_clock::now();
        solver.solve(copy);
        const auto end = chrono::high_resolution_clock::now();
        const double timeSec = chrono::duration<double>(end - start).count();
        const int correct = CorrectnessChecker::check(copy) ? 1 : 0;
        results.push_back({boardId, difficulty, "DLX", 1, rep, timeSec, correct});
    }
    return results;
}

vector<BenchmarkResult> BenchmarkRunner::benchmarkDLXParallel(const Board &board, const string &boardId,
                                                              const string &difficulty, const int threads) {
    DLXSolver solver(threads);
    vector<BenchmarkResult> results;
    results.reserve(REPETITIONS);

    for (int rep = 1; rep <= REPETITIONS; rep++) {
        Board copy = board;
#ifdef _OPENMP
        const double start = omp_get_wtime();
        solver.solve(copy);
        const double timeSec = omp_get_wtime() - start;
#else
        const auto start = chrono::high_resolution_clock::now();
        solver.solve(copy);
        const double timeSec = chrono::duration<double>(chrono::high_resolution_clock::now() - start).count();
#endif
        const int correct = CorrectnessChecker::check(copy) ? 1 : 0;
        results.push_back({boardId, difficulty, "DLX-OMP", threads, rep, timeSec, correct});
    }
    return results;
}

void BenchmarkRunner::writeCsv(const vector<BenchmarkResult> &results, const string &outputPath) {
    ofstream file(outputPath);
    file << "Board_ID,Difficulty,Algorithm,Threads,Rep,Time_s,Correct\n";
    file << fixed << setprecision(9);

    for (const auto &r: results) {
        file << r.boardId << ","
                << r.difficulty << ","
                << r.algorithm << ","
                << r.threads << ","
                << r.rep << ","
                << r.timeSec << ","
                << r.correct << "\n";
    }
}
