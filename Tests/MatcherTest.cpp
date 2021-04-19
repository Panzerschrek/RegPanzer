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

class CheckMatchTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(CheckMatchTest, MatchTest)
{
	const auto param= GetParam();
	const auto regexp= RegPanzer::ParseRegexpString(param.regexp_str);
	ASSERT_NE(regexp, std::nullopt);

	for(const MatcherTestDataElement::Case& c : param.cases)
	{
		MatcherTestDataElement::Ranges result_ranges;

		std::string_view str= c.input_str;
		while(true)
		{
			const MatchResult res= Match(*regexp, str);
			if(res == std::nullopt)
				break;

			result_ranges.emplace_back(size_t(res->data() - c.input_str.data()), size_t(res->data() + res->size() - c.input_str.data()));
			str= str.substr(size_t(res->data() + res->size() - str.data()));
		}

		ASSERT_EQ(result_ranges, c.result_ranges);
	}
}

INSTANTIATE_TEST_CASE_P(M, CheckMatchTest, testing::ValuesIn(g_matcher_test_data));

} // namespace

} // namespace RegPanzer
