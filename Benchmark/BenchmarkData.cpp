#include "BenchmarkData.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <benchmark/benchmark.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <random>

namespace RegPanzer
{

namespace
{

std::string GetSpaceSeparatedSequences(
	const size_t min_word_length,
	const size_t max_word_length,
	const std::string_view symbols,
	const size_t word_count)
{
	std::string res;

	std::minstd_rand gen;
	for(size_t i= 0; i < word_count; ++i)
	{
		const size_t len= gen() % (max_word_length - min_word_length) + min_word_length;

		for(size_t j= 0; j < len; ++j)
			res.push_back(symbols[gen() % symbols.length()]);
		res.push_back(' ');
	}

	return res;
}

} // namespace

const BenchmarkDataElement g_benchmark_data[]
{
	// Simple sequence of symbols.
	{
		"[A-Z_a-z0-9]+",
		[]{ return GetSpaceSeparatedSequences(1, 20, "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz0123456789", 1024 * 1024); }
	},

	// Sequence with backwalk.
	{
		"[0-9]*3",
		[]{ return GetSpaceSeparatedSequences(1, 30, "0123456789", 1024 * 256); }
	},

	// Long fixed sequnce.
	{
		"1101011011101110100101011010101110101",
		[]{ return GetSpaceSeparatedSequences(3, 40, "01", 1024 * 256); }
	},

	// Two sequences with separator present in both sequences.
	{
		"[w-z]+y[w-z]+",
		[]{ return GetSpaceSeparatedSequences(3, 50, "wwwwwwwwxxxxxxxxzzzzzzzzy" /* non-even distribution */, 1024 * 256); }
	},

	// Manulally optimized version of previous expression.
	{
		"[w-z]+?y[w-z]+",
		[]{ return GetSpaceSeparatedSequences(3, 50, "wwwwwwwwxxxxxxxxzzzzzzzzy" /* non-even distribution */, 1024 * 256); }
	},

	// Simple sequence with fixed size.
	{
		"(?:abc){3}",
		[]{ return GetSpaceSeparatedSequences(6, 20, "abc", 1024 * 1024); }
	},

	// Complex expression with recursion, lookbehind, lookahead, backreference.
	{
		"(?<![a-f])(([a-f])((?1)|[a-f])?\\2)(?![a-f])",
		[]{ return GetSpaceSeparatedSequences(1, 8, "abcdef", 1024 * 1024); }
	},
};

const size_t g_benchmark_data_size= std::size(g_benchmark_data);

} // namespace RegPanzer
