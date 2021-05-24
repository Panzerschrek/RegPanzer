#include "BenchmarkData.hpp"
#include "../RegPanzerLib/Matcher.hpp"
#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <benchmark/benchmark.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

void MatcherBenchmark(benchmark::State& st)
{
	const auto& param= g_benchmark_data[st.range(0)];

	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	const auto regex_graph= BuildRegexGraph(*regex_chain, Options());

	const auto test_data= param.data_generation_func();

	for (auto _ : st)
	{
		size_t count= 0;
		for(size_t start_pos= 0; start_pos < test_data.size();)
		{
			std::string_view res;
			if(Match(regex_graph, test_data, start_pos, &res, 1) != 0)
			{
				++count;
				start_pos= size_t(res.data() - test_data.data()) + res.size();
			}
			else
				break;
		}
	}
}

BENCHMARK(MatcherBenchmark)->Range(0, int64_t(g_benchmark_data_size) - 1);

} // namespace

} // namespace RegPanzer
