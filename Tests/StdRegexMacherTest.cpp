#include "MatcherTestData.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <regex>

namespace RegPanzer
{

namespace
{

class StdRegexpMatchTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(StdRegexpMatchTest, TestMatch)
{
	const auto param= GetParam();

	// Ignore non-ascii regexp and cases, because std::regex does not support UTF-8.
	if(StringContainsNonASCIISymbols(param.regexp_str))
		return;

	std::regex regex(param.regexp_str);

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

INSTANTIATE_TEST_CASE_P(M, StdRegexpMatchTest, testing::ValuesIn(g_matcher_test_data));

} // namespace

} // namespace RegPanzer
