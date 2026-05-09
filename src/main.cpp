#include <iostream>
#include "Benchmark/BenchmarkRunner.h"
using namespace std;

int main() {
    BenchmarkRunner::run(PROJECT_ROOT, 100);
    return 0;
}
