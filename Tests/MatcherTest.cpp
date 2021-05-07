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

		std::string_view str= c.input_str;
		while(true)
		{
			const MatchResult res= Match(regex_graph.root, str);
			if(res == std::nullopt || res->empty())
				break;

			result_ranges.emplace_back(size_t(res->data() - c.input_str.data()), size_t(res->data() + res->size() - c.input_str.data()));
			str= str.substr(size_t(res->data() + res->size() - str.data()));
		}

		EXPECT_EQ(result_ranges, c.result_ranges);
	}
}

INSTANTIATE_TEST_CASE_P(M, MatchTest, testing::ValuesIn(g_matcher_test_data));

} // namespace

} // namespace RegPanzer
