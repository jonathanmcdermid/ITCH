#include <benchmark/benchmark.h>

static void BM_transposition_table(benchmark::State& state)
{
    for (auto _ : state) {}

    state.counters["per_second"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_transposition_table)->Range(1, 1000)->Iterations(100);

int main(int argc, char** argv)
{
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    return EXIT_SUCCESS;
}
