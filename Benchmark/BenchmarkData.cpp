#include "BenchmarkData.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <benchmark/benchmark.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <random>

namespace RegPanzer
{

namespace
{

std::string GenRandomWords()
{
	const size_t word_count= 1024 * 1024;
	const size_t min_word_length= 1;
	const size_t max_word_length= 20;
	const char c_symbols[]= "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz0123456789";

	std::string res;

	std::minstd_rand gen;
	for(size_t i= 0; i < word_count; ++i)
	{
		const size_t len= gen() % (max_word_length - min_word_length) + min_word_length;

		for(size_t j= 0; j < len; ++j)
			res.push_back(c_symbols[gen() % (std::size(c_symbols) - 1)]);
		res.push_back(' ');
	}

	return res;
}

std::string GenRandomDecimalNumbers()
{
	const size_t word_count= 1024 * 256;
	const size_t min_word_length= 1;
	const size_t max_word_length= 30;
	const char c_symbols[]= "0123456789";

	std::string res;

	std::minstd_rand gen;
	for(size_t i= 0; i < word_count; ++i)
	{
		const size_t len= gen() % (max_word_length - min_word_length) + min_word_length;

		for(size_t j= 0; j < len; ++j)
			res.push_back(c_symbols[gen() % (std::size(c_symbols) - 1)]);
		res.push_back(' ');
	}

	return res;
}

std::string GenRandomDataForPolyndromes()
{
	const size_t word_count= 1024 * 1024;
	const size_t min_word_length= 1;
	const size_t max_word_length= 8;
	const char c_symbols[]= "abcdef";

	std::string res;

	std::minstd_rand gen;
	for(size_t i= 0; i < word_count; ++i)
	{
		const size_t len= gen() % (max_word_length - min_word_length) + min_word_length;

		for(size_t j= 0; j < len; ++j)
			res.push_back(c_symbols[gen() % (std::size(c_symbols) - 1)]);
		res.push_back(' ');
	}

	return res;
}

} // namespace

const BenchmarkDataElement g_benchmark_data[]
{
	// Simple sequence of symbols.
	{ "[A-Z_a-z0-9]+", GenRandomWords },

	// Sequence with backwalk.
	{ "[0-9]*3", GenRandomDecimalNumbers },

	// Complex expression with recursion, lookbehind, lookahead, backreference.
	{ "(?<![a-f])(([a-f])((?1)|[a-f])?\\2)(?![a-f])", GenRandomDataForPolyndromes },
};

constexpr size_t g_benchmark_data_size= std::size(g_benchmark_data);

} // namespace RegPanzer
