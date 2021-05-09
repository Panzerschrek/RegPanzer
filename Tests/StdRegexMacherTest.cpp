#include "MatcherTestData.hpp"
#include "Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <regex>

namespace RegPanzer
{

namespace
{

class StdRegexMatchTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(StdRegexMatchTest, TestMatch)
{
	const auto param= GetParam();

	// Ignore unsupported features.
	if((GetRegexFeatures(param.regex_str) & (
			RegexFeatureFlag::UTF8 |
			RegexFeatureFlag::PossessiveSequences |
			RegexFeatureFlag::AtomicGroups |
			RegexFeatureFlag::ConditionalElements |
			RegexFeatureFlag::Subroutines |
			RegexFeatureFlag::FourDigitHexCodes  |
			RegexFeatureFlag::LookBehind)) != 0)
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

INSTANTIATE_TEST_CASE_P(M, StdRegexMatchTest, testing::ValuesIn(g_matcher_test_data));

} // namespace

} // namespace RegPanzer
