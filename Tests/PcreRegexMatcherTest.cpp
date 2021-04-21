#include "MatcherTestData.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <pcrecpp.h>

namespace RegPanzer
{

namespace
{

class PcreRegexpMatchTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(PcreRegexpMatchTest, TestMatch)
{
	const auto param= GetParam();

	pcrecpp::RE_Options options;
	options.set_utf8(true);
	pcrecpp::RE r(param.regexp_str, options);

	for(const MatcherTestDataElement::Case& c : param.cases)
	{
		MatcherTestDataElement::Ranges result_ranges;

		pcrecpp::StringPiece str(c.input_str.data());
		while(!str.empty())
		{
			const auto prev_str= str;
			if(r.Consume(&str))
				result_ranges.emplace_back(size_t(prev_str.data() - c.input_str.data()), size_t(str.data() - c.input_str.data()));
			else
				str.remove_prefix(1);
		}

		EXPECT_EQ(result_ranges, c.result_ranges);
	}
}

INSTANTIATE_TEST_CASE_P(M, PcreRegexpMatchTest, testing::ValuesIn(g_matcher_test_data));

} // namespace

} // namespace RegPanzer
