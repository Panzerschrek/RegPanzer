#include "../RegPanzerLib/Matcher.hpp"
#include "../RegPanzerLib/RegexpBuilder.hpp"
#include <gtest/gtest.h>

namespace RegPanzer
{

namespace
{

struct TestDataElement
{
	std::string regexp_str;

	using Range= std::pair<size_t, size_t>; // begin/end
	using Ranges= std::vector<Range>;
	struct Case
	{
		std::string input_str;
		Ranges result_ranges;
	};

	std::vector<Case> cases;
};

class CheckMatchTest : public ::testing::TestWithParam<TestDataElement> {};

TEST_P(CheckMatchTest, MatchTest)
{
	const auto param= GetParam();
	const auto regexp= RegPanzer::ParseRegexpString(param.regexp_str);
	ASSERT_NE(regexp, std::nullopt);

	for(const TestDataElement::Case& c : param.cases)
	{
		TestDataElement::Ranges result_ranges;

		std::basic_string_view<CharType> str= c.input_str;
		while(true)
		{
			const MatchResult res= Match(*regexp, str);
			if(res == std::nullopt)
				break;

			result_ranges.emplace_back(size_t(res->data() - c.input_str.data()), size_t(res->data() + res->size() - c.input_str.data()));
			str= str.substr(res->data() + res->size() - str.data());
		}

		ASSERT_EQ(result_ranges, c.result_ranges);
	}
}

const TestDataElement c_test_data[]
{
	// Match simple fixed sequence.
	{
		"abc",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Non-empty string - no matches.
				"wdawf 4654 aawgabC cba bbac",
				{},
			},
			{ // String is equal to regexp.
				"abc",
				{
					{ 0, 3 },
				},
			},
			{ // Signle match in middle of string.
				"lolQabcwat",
				{
					{ 4, 7 },
				},
			},
			{ // Signle match at end of string.
				"012345abc",
				{
					{ 6, 9 },
				},
			},
			{ // Multiple ranges in string.
				"abcSSabcQ",
				{
					{ 0, 3 }, { 5, 8 },
				},
			},
		},
	},
};

INSTANTIATE_TEST_CASE_P(M, CheckMatchTest, testing::ValuesIn(c_test_data));

} // namespace

} // namespace RegPanzer
