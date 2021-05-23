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

std::string GenTestData()
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

TEST(Benchmark, TestSimpleSequence)
{
	const auto parse_res= RegPanzer::ParseRegexString("[A-Z_a-z0-9]+");
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	const auto regex_graph= BuildRegexGraph(*regex_chain, Options());

	const std::string test_data= GenTestData();

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

} // namespace

} // namespace RegPanzer
