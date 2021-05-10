#include "MatcherTestData.hpp"
#include "../RegPanzerLib/Matcher.hpp"
#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

class MatchTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(MatchTest, TestMatch)
{
	const auto param= GetParam();
	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	const auto regex_graph= BuildRegexGraph(*regex_chain);

	for(const MatcherTestDataElement::Case& c : param.cases)
	{
		MatcherTestDataElement::Ranges result_ranges;

		for(size_t start_pos= 0; start_pos < c.input_str.size();)
		{
			std::string_view subgroups[10];
			if(Match(regex_graph.root, c.input_str, start_pos, subgroups, std::size(subgroups)))
			{
				const std::string_view res= subgroups[0];
				const size_t start_offset= size_t(res.data() - c.input_str.data());
				const size_t end_offset= start_offset + res.size();
				result_ranges.emplace_back(start_offset, end_offset);
				start_pos= end_offset;
			}
			else
				break;
		}

		EXPECT_EQ(result_ranges, c.result_ranges);
	}
}

INSTANTIATE_TEST_CASE_P(M, MatchTest, testing::ValuesIn(g_matcher_test_data, g_matcher_test_data + g_matcher_test_data_size));

} // namespace

} // namespace RegPanzer
