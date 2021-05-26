#include "BenchmarkData.hpp"
#include "../RegPanzerLib/Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <benchmark/benchmark.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <regex>

namespace RegPanzer
{

namespace
{

bool IsUnsupportedRegex(const std::string& regex_str)
{
	// Ignore unsupported features.
	return
		(GetRegexFeatures(regex_str) & (
			RegexFeatureFlag::UTF8 |
			RegexFeatureFlag::PossessiveSequences |
			RegexFeatureFlag::AtomicGroups |
			RegexFeatureFlag::ConditionalElements |
			RegexFeatureFlag::Subroutines |
			RegexFeatureFlag::FourDigitHexCodes  |
			RegexFeatureFlag::LookBehind)) != 0;
}

void StdRegexMatcherBenchmark(benchmark::State& st)
{
	const auto& param= g_benchmark_data[st.range(0)];

	if(IsUnsupportedRegex(param.regex_str))
		return;

	std::regex regex(param.regex_str, std::regex_constants::ECMAScript | std::regex_constants::optimize);

	const auto test_data= param.data_generation_func();

	for (auto _ : st)
	{
		size_t count= 0;
		for(auto it= std::sregex_iterator(test_data.begin(), test_data.end(), regex); it != std::sregex_iterator(); ++it)
			++count;
	}
}

BENCHMARK(StdRegexMatcherBenchmark)->DenseRange(0, int64_t(g_benchmark_data_size) - 1)->Unit(benchmark::kMillisecond);

} // namespace

} // namespace RegPanzer
