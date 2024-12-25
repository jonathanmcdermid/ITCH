#include <benchmark/benchmark.h>
#include <itch_parser.hpp>
#include <stdexcept>

static void BM_parse(benchmark::State& state)
{
    for (auto _ : state)
    {
        ItchParser parser;
        parser.Parse(std::getenv("ITCH_FILE_PATH"));
        // parser.CalculateAndPrintVwap();
    }

    state.counters["per_second"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_parse);

int main(int argc, char** argv)
{
    if (argc < 2) { throw std::invalid_argument("1 argument required.\nUsage: <project root> [benchmark options]"); }

#ifdef _WIN32
    if (_putenv_s("ITCH_FILE_PATH", argv[1]) != 0) { throw std::runtime_error("Failed to set file path."); }
#else
    if (setenv("ITCH_FILE_PATH", argv[1], 1) != 0) { throw std::runtime_error("Failed to set file path."); }
#endif

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    return EXIT_SUCCESS;
}
