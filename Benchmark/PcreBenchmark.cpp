#include "BenchmarkData.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <benchmark/benchmark.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <pcre.h>

namespace RegPanzer
{

namespace
{

void PcreMatcherBenchmark(benchmark::State& st)
{
	const auto& param= g_benchmark_data[st.range(0)];

	const char* error_ptr= nullptr;
	int error_offset= 0;
	pcre* const r= pcre_compile(param.regex_str.data(), PCRE_UTF8, &error_ptr, &error_offset, nullptr);

	const auto test_data= param.data_generation_func();

	for (auto _ : st)
	{
		size_t count= 0;
		int vec[100*3]{};

		for(size_t i= 0; i < test_data.size();)
		{
			if(pcre_exec(r, nullptr, test_data.data(), int(test_data.size()), int(i), PCRE_NO_UTF8_CHECK, vec, std::size(vec)) != 0)
			{
				if(vec[0] < 0 || vec[1] < 0 || size_t(vec[1]) <= i || size_t(vec[1]) > test_data.size())
					break;
				++count;
				i= size_t(vec[1]);
			}
			else
				break;
		}
	}
}

BENCHMARK(PcreMatcherBenchmark)->DenseRange(0, int64_t(g_benchmark_data_size) - 1)->Unit(benchmark::kMillisecond);

void PcreJITMatcherBenchmark(benchmark::State& st)
{
	const auto& param= g_benchmark_data[st.range(0)];

	const char* error_ptr= nullptr;
	int error_offset= 0;
	pcre* const r= pcre_compile(param.regex_str.data(), PCRE_UTF8, &error_ptr, &error_offset, nullptr);

	const char* err_ptr= nullptr;
	const auto jit_extra= pcre_study(r, PCRE_STUDY_JIT_COMPILE, &err_ptr);
	const auto stack= pcre_jit_stack_alloc(1024, 4 * 1024 * 1024);

	const auto test_data= param.data_generation_func();

	for (auto _ : st)
	{
		size_t count= 0;
		int vec[100*3]{};

		for(size_t i= 0; i < test_data.size();)
		{
			if(pcre_jit_exec(r, jit_extra, test_data.data(), int(test_data.size()), int(i), PCRE_NO_UTF8_CHECK, vec, std::size(vec), stack) != 0)
			{
				if(vec[0] < 0 || vec[1] < 0 || size_t(vec[1]) <= i || size_t(vec[1]) > test_data.size())
					break;
				++count;
				i= size_t(vec[1]);
			}
			else
				break;
		}
	}

	pcre_jit_stack_free(stack);
	pcre_free_study(jit_extra);
}

BENCHMARK(PcreJITMatcherBenchmark)->DenseRange(0, int64_t(g_benchmark_data_size) - 1)->Unit(benchmark::kMillisecond);

} // namespace

} // namespace RegPanzer
