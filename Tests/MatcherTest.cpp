#include "MatcherTestData.hpp"
#include "GroupsExtractionTestData.hpp"
#include "../RegPanzerLib/Matcher.hpp"
#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/RegexGraphOptimizer.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

void RunTestCase(const MatcherTestDataElement& param, const bool is_multiline)
{
	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	Options options;
	options.multiline= is_multiline;
	const auto regex_graph= OptimizeRegexGraph( BuildRegexGraph(*regex_chain, options) );

	for(const MatcherTestDataElement::Case& c : param.cases)
	{
		MatcherTestDataElement::Ranges result_ranges;

		for(size_t start_pos= 0; start_pos < c.input_str.size();)
		{
			std::string_view res;
			if(Match(regex_graph, c.input_str, start_pos, &res, 1) != 0)
			{
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

class MatchTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(MatchTest, TestMatch)
{
	RunTestCase(GetParam(), false);
}

INSTANTIATE_TEST_SUITE_P(M, MatchTest, testing::ValuesIn(g_matcher_test_data, g_matcher_test_data + g_matcher_test_data_size));


class MatchMultilineTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(MatchMultilineTest, TestMatch)
{
	RunTestCase(GetParam(), true);
}

INSTANTIATE_TEST_SUITE_P(M, MatchMultilineTest, testing::ValuesIn(g_matcher_multiline_test_data, g_matcher_multiline_test_data + g_matcher_multiline_test_data_size));


class GroupsExtractionTest : public ::testing::TestWithParam<GroupsExtractionTestDataElement> {};

TEST_P(GroupsExtractionTest, TestGroupsExtraction)
{
	const auto param= GetParam();
	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	Options options;
	options.extract_groups= true;
	const auto regex_graph= OptimizeRegexGraph( BuildRegexGraph(*regex_chain, options) );

	for(const GroupsExtractionTestDataElement::Case& c : param.cases)
	{
		std::vector<GroupsExtractionTestDataElement::GroupMatchResults> results;

		for(size_t start_pos= 0; start_pos < c.input_str.size();)
		{
			std::string_view subgroups[10];
			const size_t groups_extracted= Match(regex_graph, c.input_str, start_pos, subgroups, std::size(subgroups));
			if(groups_extracted == 0)
				break;

			GroupsExtractionTestDataElement::GroupMatchResults result;
			for(size_t i= 0; i < std::min(groups_extracted, std::size(subgroups)); ++i)
			{
				const std::string_view res= subgroups[i];
				const size_t start_offset= size_t(res.data() - c.input_str.data());
				const size_t end_offset= start_offset + res.size();
				result.emplace_back(start_offset, end_offset);
			}
			start_pos= size_t(subgroups[0].data() - c.input_str.data()) + subgroups[0].size();

			results.push_back(std::move(result));
		}

		EXPECT_EQ(results, c.results);
	}
}

INSTANTIATE_TEST_SUITE_P(GE, GroupsExtractionTest, testing::ValuesIn(g_groups_extraction_test_data, g_groups_extraction_test_data + g_groups_extraction_test_data_size));

} // namespace

} // namespace RegPanzer
