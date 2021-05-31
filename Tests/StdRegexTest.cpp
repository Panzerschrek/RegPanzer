#include "MatcherTestData.hpp"
#include "GroupsExtractionTestData.hpp"
#include "../RegPanzerLib/Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
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
			RegexFeatureFlag::FourDigitHexCodes |
			RegexFeatureFlag::LookBehind |
			RegexFeatureFlag::LineStartEndAssertions)) != 0;
}

class StdRegexMatchTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(StdRegexMatchTest, TestMatch)
{
	const auto param= GetParam();

	if(IsUnsupportedRegex(param.regex_str))
		return;

	try
	{
		std::regex regex(param.regex_str, std::regex_constants::ECMAScript);

		for(const MatcherTestDataElement::Case& c : param.cases)
		{
			if(StringContainsNonASCIISymbols(c.input_str))
				continue;

			MatcherTestDataElement::Ranges result_ranges;

			for(auto it= std::sregex_iterator(c.input_str.begin(), c.input_str.end(), regex); it != std::sregex_iterator(); ++it)
				result_ranges.emplace_back(it->position(), it->position() + it->length());

			EXPECT_EQ(result_ranges, c.result_ranges);
		}
	}
	catch(const std::regex_error& e)
	{
		std::cout << "std::regex error: " << e.what() << std::endl;
		ASSERT_TRUE(false);
	}
}

INSTANTIATE_TEST_CASE_P(M, StdRegexMatchTest, testing::ValuesIn(g_matcher_test_data, g_matcher_test_data + g_matcher_test_data_size));


class StdRegexGroupsExtractionTest : public ::testing::TestWithParam<GroupsExtractionTestDataElement> {};

TEST_P(StdRegexGroupsExtractionTest, TestGroupsExtraction)
{
	const auto param= GetParam();

	if(IsUnsupportedRegex(param.regex_str))
		return;

	// Some implementations of std::regex does not handle properly optional groups extraction, so, skip some tests.
	if( param.regex_str == "([a-z]([0-9])?)+" ||
		param.regex_str == "(([A-Z])|([a-z])|([0-9]))+"
		)
		return;

	try
	{
		std::regex regex(param.regex_str, std::regex_constants::ECMAScript);

		for(const GroupsExtractionTestDataElement::Case& c : param.cases)
		{
			if(StringContainsNonASCIISymbols(c.input_str))
				continue;

			std::vector<GroupsExtractionTestDataElement::GroupMatchResults> results;

			for(auto it= std::sregex_iterator(c.input_str.begin(), c.input_str.end(), regex); it != std::sregex_iterator(); ++it)
			{
				GroupsExtractionTestDataElement::GroupMatchResults result;

				for(const auto& pair : *it)
				{
					const auto b= size_t(pair.first - c.input_str.begin());
					const auto e= size_t(pair.second - c.input_str.begin());
					result.emplace_back(b, e);
				}

				results.push_back(std::move(result));
			}

			EXPECT_EQ(results, c.results);
		}
	}
	catch(const std::regex_error& e)
	{
		std::cout << "std::regex error: " << e.what() << std::endl;
		ASSERT_TRUE(false);
	}
}

INSTANTIATE_TEST_CASE_P(GE, StdRegexGroupsExtractionTest, testing::ValuesIn(g_groups_extraction_test_data, g_groups_extraction_test_data + g_groups_extraction_test_data_size));

} // namespace

} // namespace RegPanzer
