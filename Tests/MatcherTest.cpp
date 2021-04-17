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
				{ { 0, 3 }, },
			},
			{ // Signle match in middle of string.
				"lolQabcwat",
				{ { 4, 7 }, },
			},
			{ // Signle match at end of string.
				"012345abc",
				{ { 6, 9 }, },
			},
			{ // Multiple ranges in string.
				"abcSSabcQ",
				{ { 0, 3 }, { 5, 8 }, },
			},
		},
	},

	// Match sequence of same symbols.
	{
		"Q+",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Non-empty string - no matches.
				"WWWAaSD",
				{},
			},
			{ // Single match - whole string with one length.
				"Q",
				{ {0, 1}, }
			},
			{ // Single match - whole string with multiple symbols.
				"QQQQQQQ",
				{ {0, 7}, }
			},
			{ // Single match in middle of string.
				"aaQQbb",
				{ {2, 4}, }
			},
			{ // Single match in end of string.
				"fffFQQQ",
				{ {4, 7}, }
			},
			{ // Multiple matches.
				"wawdQwdawdQQQwafawfQQfffQQQQa",
				{ {4, 5}, {10, 13}, {19, 21}, {24, 28}, }
			},
		}
	},

	// Match sequence of symobl ranges.
	{
		"[a-z]+",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Non-empty string - no matches.
				"@6656%%1",
				{},
			},
			{ // No matches for symobls outside range.
				"CAPITAL LETTERS",
				{},
			},
			{ // Single match for single symobl in range.
				"f",
				{ {0, 1}, }
			},
			{ // Single match for single word.
				"leftwing",
				{ {0, 8}, }
			},
			{ // Single match for single word in middle of string.
				"66 cups!",
				{ {3, 7}, }
			},
			{ // Multiple words.
				"quick brown fox jumps over the lazy dog",
				{ {0, 5}, {6, 11}, {12, 15}, {16, 21}, {22, 26}, {27, 30}, {31, 35}, {36, 39} }
			},
			{
				// Non-words at begin and at end.
				"45lol-wat6",
				{ {2, 5}, {6, 9} }
			},
		}
	},
};

INSTANTIATE_TEST_CASE_P(M, CheckMatchTest, testing::ValuesIn(c_test_data));

} // namespace

} // namespace RegPanzer
