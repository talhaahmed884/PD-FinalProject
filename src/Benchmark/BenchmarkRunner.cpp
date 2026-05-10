#include "BenchmarkRunner.h"
#include "../BoardGenerator/BoardGenerator.h"
#include "../BoardSolver/SerialSolver/SerialSolver.h"
#include "../BoardSolver/OpenMPSolver/OpenMPSolver.h"
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
#include <sys/stat.h>
using namespace std;

static constexpr int OPENMP_THREAD_COUNTS[] = {4, 8, 16};

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

static string makeOutputPath(const string &outputDir) {
    mkdir(outputDir.c_str(), 0755);
    const time_t now = time(nullptr);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", localtime(&now));
    return outputDir + "/results_" + buf + ".csv";
}

static void printRow(const BenchmarkResult &r) {
    cout << fixed << setprecision(6)
            << r.boardId << ","
            << r.difficulty << ","
            << r.algorithm << ","
            << r.threads << ","
            << r.timeSec << ","
            << r.correct << "\n";
}

void BenchmarkRunner::run(const string &outputDir, const int puzzleCount) {
    const string outputPath = makeOutputPath(outputDir);

    vector<BenchmarkResult> results;

    cout << "Board_ID,Difficulty,Algorithm,Threads,Time_s,Correct\n";

    for (const auto &diff: DIFFICULTIES) {
        const vector<Board> boards = BoardGenerator::loadProblems(puzzleCount, diff.level);

        for (int i = 0; i < static_cast<int>(boards.size()); i++) {
            ostringstream oss;
            oss << diff.prefix << "_" << setw(3) << setfill('0') << (i + 1);
            const string boardId = oss.str();

            BenchmarkResult serial = benchmarkSerial(boards[i], boardId, diff.name);
            printRow(serial);
            results.push_back(serial);

            for (const int t: OPENMP_THREAD_COUNTS) {
                BenchmarkResult omp = benchmarkOpenMP(boards[i], boardId, diff.name, t);
                printRow(omp);
                results.push_back(omp);
            }
        }
    }

    writeCsv(results, outputPath);
    cout << "\nResults written to " << outputPath << "\n";
}

BenchmarkResult BenchmarkRunner::benchmarkSerial(const Board &board, const string &boardId,
                                                 const string &difficulty) {
    Board copy = board;
    SerialSolver solver;

    const auto start = chrono::high_resolution_clock::now();
    solver.solve(copy);
    const auto end = chrono::high_resolution_clock::now();

    const double timeSec = chrono::duration<double>(end - start).count();
    const int correct = CorrectnessChecker::check(copy) ? 1 : 0;

    return {boardId, difficulty, "Serial", 1, timeSec, correct};
}

BenchmarkResult BenchmarkRunner::benchmarkOpenMP(const Board &board, const string &boardId,
                                                 const string &difficulty, const int threads) {
    Board copy = board;
    OpenMPSolver solver(threads);

#ifdef _OPENMP
    const double startSec = omp_get_wtime();
    solver.solve(copy);
    const double timeSec = omp_get_wtime() - startSec;
#else
    const auto start = chrono::high_resolution_clock::now();
    solver.solve(copy);
    const double timeSec = chrono::duration<double>(chrono::high_resolution_clock::now() - start).count();
#endif
    const int correct = CorrectnessChecker::check(copy) ? 1 : 0;

    return {boardId, difficulty, "OpenMP", threads, timeSec, correct};
}

void BenchmarkRunner::writeCsv(const vector<BenchmarkResult> &results, const string &outputPath) {
    ofstream file(outputPath);
    file << "Board_ID,Difficulty,Algorithm,Threads,Time_s,Correct\n";
    file << fixed << setprecision(6);

    for (const auto &r: results) {
        file << r.boardId << ","
                << r.difficulty << ","
                << r.algorithm << ","
                << r.threads << ","
                << r.timeSec << ","
                << r.correct << "\n";
    }
}
