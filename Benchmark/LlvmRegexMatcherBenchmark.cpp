#include "BenchmarkData.hpp"
#include "../RegPanzerLib/Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <benchmark/benchmark.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Regex.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

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
			RegexFeatureFlag::LazySequences |
			RegexFeatureFlag::PossessiveSequences |
			RegexFeatureFlag::Look |
			RegexFeatureFlag::NoncapturingGroups |
			RegexFeatureFlag::AtomicGroups |
			RegexFeatureFlag::Subroutines |
			RegexFeatureFlag::SymbolClasses)) != 0;
}

void LlvmRegexMatcherBenchmark(benchmark::State& st)
{
	const auto& param= g_benchmark_data[st.range(0)];

	if(IsUnsupportedRegex(param.regex_str))
		return;

	llvm::Regex regex(param.regex_str);

	const auto test_data= param.data_generation_func();

	for (auto _ : st)
	{
		llvm::StringRef str= test_data;
		size_t count= 0;
		while(!str.empty())
		{
			llvm::SmallVector<llvm::StringRef, 16> matches;
			if(!regex.match(str, &matches))
				break;

			if(matches.empty())
				break;

			++count;

			const llvm::StringRef& match= matches.front();
			str= str.substr(size_t(match.data() + match.size() - str.data()));
		}
	}
}

BENCHMARK(LlvmRegexMatcherBenchmark)->Range(0, int64_t(g_benchmark_data_size) - 1);

} // namespace

} // namespace RegPanzer
