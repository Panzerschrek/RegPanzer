#include "BenchmarkData.hpp"
#include "MatcherTestData.hpp"
#include "GroupsExtractionTestData.hpp"
#include "../RegPanzerLib/Matcher.hpp"
#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
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

class BenchmarkTest : public ::testing::TestWithParam<BenchmarkDataElement> {};

TEST_P(BenchmarkTest, Bench)
{
	const auto& param= GetParam();

	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	const auto regex_graph= BuildRegexGraph(*regex_chain, Options());

	const std::string test_data= param.data_generation_func();

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

INSTANTIATE_TEST_CASE_P(BE, BenchmarkTest, testing::ValuesIn(g_benchmark_data, g_benchmark_data + g_benchmark_data_size));

} // namespace

const BenchmarkDataElement g_benchmark_data[]
{
	{ "[A-Z_a-z0-9]+", GenRandomWords },
	{ "(?<![a-f])([a-f]+)((?R)|[a-f])?\\1(?![a-f])", GenRandomDataForPolyndromes },
};

const size_t g_benchmark_data_size= std::size(g_benchmark_data);

} // namespace RegPanzer
