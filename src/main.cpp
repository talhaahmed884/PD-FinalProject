#include <iostream>
#include "Benchmark/BenchmarkRunner.h"
using namespace std;

int main() {
    BenchmarkRunner::run(string(PROJECT_ROOT) + "/results", 100);
    return 0;
}
