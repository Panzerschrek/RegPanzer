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

	// One of with some symbols.
	{
		"[GrTx]",
		{
			{
				OneOf
				{
					{ 'G', 'r', 'T', 'x' },
					{}
				},
				{ 1, 1, false },
			},
		}
	},

	// One of with range.
	{
		"[a-f]",
		{
			{
				OneOf
				{
					{},
					{
						{ 'a', 'f' },
					}
				},
				{ 1, 1, false },
			},
		}
	},

	// One of with symobls and range.
	{
		"[@a-z_0-9]",
		{
			{
				OneOf
				{
					{ '@', '_' },
					{
						{ 'a', 'z' },
						{ '0', '9' },
					}
				},
				{ 1, 1, false },
			},
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

	// Alternatives.
	{
		"lol|wat",
		{ {
			Alternatives
			{ {
				{
					{ SpecificSymbol{ 'l' }, { 1, 1, false }, },
					{ SpecificSymbol{ 'o' }, { 1, 1, false }, },
					{ SpecificSymbol{ 'l' }, { 1, 1, false }, },
				},
				{
					{ SpecificSymbol{ 'w' }, { 1, 1, false }, },
					{ SpecificSymbol{ 'a' }, { 1, 1, false }, },
					{ SpecificSymbol{ 't' }, { 1, 1, false }, },
				},
			} },
			{ 1, 1, false }
		} }
	},

	// Alternatives with multiple variants.
	{
		"Qrf|zc+|(Vx)*|svt",
		{ {
			Alternatives
			{ {
				{
					{ SpecificSymbol{ 'Q' }, { 1, 1, false }, },
					{ SpecificSymbol{ 'r' }, { 1, 1, false }, },
					{ SpecificSymbol{ 'f' }, { 1, 1, false }, },
				},
				{
					{ SpecificSymbol{ 'z' }, { 1, 1, false }, },
					{ SpecificSymbol{ 'c' }, { 1, std::numeric_limits<size_t>::max(), false }, },
				},
				{
					{
						BracketExpression
						{ {
							{ SpecificSymbol{ 'V' }, { 1, 1, false }, },
							{ SpecificSymbol{ 'x' }, { 1, 1, false }, },
						} },
						{ 0, std::numeric_limits<size_t>::max(), false },
					}
				},
				{
					{ SpecificSymbol{ 's' }, { 1, 1, false }, },
					{ SpecificSymbol{ 'v' }, { 1, 1, false }, },
					{ SpecificSymbol{ 't' }, { 1, 1, false }, },
				}
			} },
			{ 1, 1, false }
		} }
	},

	// Zero or more quantifier.
	{
		"B*",
		{
			{ SpecificSymbol{ 'B' }, { 0, std::numeric_limits<size_t>::max(), false }, },
		}
	},

	// One or more quantifier.
	{
		"WrV+",
		{
			{ SpecificSymbol{ 'W' }, { 1, 1, false }, },
			{ SpecificSymbol{ 'r' }, { 1, 1, false }, },
			{ SpecificSymbol{ 'V' }, { 1, std::numeric_limits<size_t>::max(), false }, },
		}
	},

	// Specific amount quantifier (range).
	{
		"z{354,789}",
		{
			{ SpecificSymbol{ 'z' }, { 354, 789, false }, },
		}
	},

	// Specific amount quantifier (single value).
	{
		"Q{1234}W",
		{
			{ SpecificSymbol{ 'Q' }, { 1234, 1234, false }, },
			{ SpecificSymbol{ 'W' }, { 1, 1, false }, }
		}
	},

	// Specific amount quantifier (lower bound).
	{
		"s{34,}t",
		{
			{ SpecificSymbol{ 's' }, { 34, std::numeric_limits<size_t>::max(), false }, },
			{ SpecificSymbol{ 't' }, { 1, 1, false }, }
		}
	},

	// Specific amount quantifier (upper bound).
	{
		"J{,786}_",
		{
			{ SpecificSymbol{ 'J' }, { 0, 786, false }, },
			{ SpecificSymbol{ '_' }, { 1, 1, false }, }
		}
	},

	// Quantifier for brackets.
	{
		"l(Qwe)+P",
		{
			{ SpecificSymbol{ 'l' }, { 1, 1, false }, },
			{
				BracketExpression
				{ {
					{ SpecificSymbol{ 'Q' }, { 1, 1, false }, },
					{ SpecificSymbol{ 'w' }, { 1, 1, false }, },
					{ SpecificSymbol{ 'e' }, { 1, 1, false }, },
				} },
				{ 1, std::numeric_limits<size_t>::max(), false }
			},
			{ SpecificSymbol{ 'P' }, { 1, 1, false }, },
		}
	},

	// Non-ASCII symbols.
	{
		"ДёСÜ☭",
		{
			{ SpecificSymbol{ U'Д' }, { 1, 1, false }, },
			{ SpecificSymbol{ U'ё' }, { 1, 1, false }, },
			{ SpecificSymbol{ U'С' }, { 1, 1, false }, },
			{ SpecificSymbol{ U'Ü' }, { 1, 1, false }, },
			{ SpecificSymbol{ U'☭' }, { 1, 1, false }, },
		}
	},
};

INSTANTIATE_TEST_CASE_P(P, CheckParseTest, testing::ValuesIn(c_test_data));

} // namespace

} // namespace RegPanzer
