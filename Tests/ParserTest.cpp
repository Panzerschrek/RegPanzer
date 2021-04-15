#include "../RegPanzerLib/RegexpBuilder.hpp"
#include <gtest/gtest.h>

namespace RegPanzer
{

namespace
{

using TestDataElement = std::pair<std::string, RegexpElementsChain>;
class CheckParseTest : public ::testing::TestWithParam<TestDataElement> {};

TEST_P(CheckParseTest, ParseTest)
{
	const auto param= GetParam();
	const auto res = RegPanzer::ParseRegexpString(param.first);
	ASSERT_EQ(res, param.second);
}

const TestDataElement c_test_data[]
{
	// Empty result for empty regexp.
	{
		"",
		{},
	},

	// Any symbol.
	{
		".",
		{
			{ AnySymbol(), { 1, 1, false }, },
		}
	},

	// Simple sequence of specific symbols.
	{
		"QrX",
		{
			{ SpecificSymbol{ 'Q' }, { 1, 1, false }, },
			{ SpecificSymbol{ 'r' }, { 1, 1, false }, },
			{ SpecificSymbol{ 'X' }, { 1, 1, false }, },
		}
	},

	// Bracket expression.
	{
		"A(bc)ef",
		{
			{ SpecificSymbol{ 'A' }, { 1, 1, false }, },
			{
				BracketExpression
				{ {
					{ SpecificSymbol{ 'b' }, { 1, 1, false }, },
					{ SpecificSymbol{ 'c' }, { 1, 1, false }, },
				} },
				{ 1, 1, false }
			},
			{ SpecificSymbol{ 'e' }, { 1, 1, false }, },
			{ SpecificSymbol{ 'f' }, { 1, 1, false }, },
		}
	},

	// Bracket expression inside bracket expression.
	{
		"g(bc(RT)Q)ww",
		{
			{ SpecificSymbol{ 'g' }, { 1, 1, false }, },
			{
				BracketExpression
				{ {
					{ SpecificSymbol{ 'b' }, { 1, 1, false }, },
					{ SpecificSymbol{ 'c' }, { 1, 1, false }, },
					{
						BracketExpression
						{ {
							{ SpecificSymbol{ 'R' }, { 1, 1, false }, },
							{ SpecificSymbol{ 'T' }, { 1, 1, false }, },
						} },
						{ 1, 1, false }
					},
					{ SpecificSymbol{ 'Q' }, { 1, 1, false }, },
				} },
				{ 1, 1, false }
			},
			{ SpecificSymbol{ 'w' }, { 1, 1, false }, },
			{ SpecificSymbol{ 'w' }, { 1, 1, false }, },
		}
	},
};

INSTANTIATE_TEST_CASE_P(P, CheckParseTest, testing::ValuesIn(c_test_data));

} // namespace

} // namespace RegPanzer
