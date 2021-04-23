#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

using TestDataElement = std::pair<std::string, RegexElementsChain>;
class ParseTest : public ::testing::TestWithParam<TestDataElement> {};

TEST_P(ParseTest, CheckParse)
{
	const auto param= GetParam();
	const auto res = RegPanzer::ParseRegexString(param.first);
	ASSERT_EQ(res, param.second);
}

const TestDataElement c_test_data[]
{
	// Empty result for empty regex.
	{
		"",
		{},
	},

	// Any symbol.
	{
		".",
		{
			{ AnySymbol(), { 1, 1, SequenceMode::Greedy }, },
		}
	},

	// Simple sequence of specific symbols.
	{
		"QrX",
		{
			{ SpecificSymbol{ 'Q' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ 'r' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ 'X' }, { 1, 1, SequenceMode::Greedy }, },
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
				{ 1, 1, SequenceMode::Greedy },
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
				{ 1, 1, SequenceMode::Greedy },
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
				{ 1, 1, SequenceMode::Greedy },
			},
		}
	},

	{ // One of with inverse flag.
		"[^0-9]",
		{
			{
				OneOf
				{
					{},
					{
						{ '0', '9' },
					},
					true
				},
				{ 1, 1, SequenceMode::Greedy },
			},
		}
	},

	{ // One of with '^' in middle - should not inverse condition.
		"[0-9^]",
		{
			{
				OneOf
				{
					{ '^' },
					{
						{ '0', '9' },
					},
					false
				},
				{ 1, 1, SequenceMode::Greedy },
			},
		}
	},

	{ // One of with inverse flag and '^';
		"[^abg_^1]",
		{
			{
				OneOf
				{
					{ 'a', 'b', 'g', '_', '^', '1' },
					{},
					true
				},
				{ 1, 1, SequenceMode::Greedy },
			},
		}
	},

	// Bracket expression.
	{
		"A(bc)ef",
		{
			{ SpecificSymbol{ 'A' }, { 1, 1, SequenceMode::Greedy }, },
			{
				BracketExpression
				{ {
					{ SpecificSymbol{ 'b' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 'c' }, { 1, 1, SequenceMode::Greedy }, },
				} },
				{ 1, 1, SequenceMode::Greedy }
			},
			{ SpecificSymbol{ 'e' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ 'f' }, { 1, 1, SequenceMode::Greedy }, },
		}
	},

	// Bracket expression inside bracket expression.
	{
		"g(bc(RT)Q)ww",
		{
			{ SpecificSymbol{ 'g' }, { 1, 1, SequenceMode::Greedy }, },
			{
				BracketExpression
				{ {
					{ SpecificSymbol{ 'b' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 'c' }, { 1, 1, SequenceMode::Greedy }, },
					{
						BracketExpression
						{ {
							{ SpecificSymbol{ 'R' }, { 1, 1, SequenceMode::Greedy }, },
							{ SpecificSymbol{ 'T' }, { 1, 1, SequenceMode::Greedy }, },
						} },
						{ 1, 1, SequenceMode::Greedy }
					},
					{ SpecificSymbol{ 'Q' }, { 1, 1, SequenceMode::Greedy }, },
				} },
				{ 1, 1, SequenceMode::Greedy }
			},
			{ SpecificSymbol{ 'w' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ 'w' }, { 1, 1, SequenceMode::Greedy }, },
		}
	},

	// Alternatives.
	{
		"lol|wat",
		{ {
			Alternatives
			{ {
				{
					{ SpecificSymbol{ 'l' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 'o' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 'l' }, { 1, 1, SequenceMode::Greedy }, },
				},
				{
					{ SpecificSymbol{ 'w' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 'a' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 't' }, { 1, 1, SequenceMode::Greedy }, },
				},
			} },
			{ 1, 1, SequenceMode::Greedy }
		} }
	},

	// Alternatives with multiple variants.
	{
		"Qrf|zc+|(Vx)*|svt",
		{ {
			Alternatives
			{ {
				{
					{ SpecificSymbol{ 'Q' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 'r' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 'f' }, { 1, 1, SequenceMode::Greedy }, },
				},
				{
					{ SpecificSymbol{ 'z' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 'c' }, { 1, Sequence::c_max, SequenceMode::Greedy }, },
				},
				{
					{
						BracketExpression
						{ {
							{ SpecificSymbol{ 'V' }, { 1, 1, SequenceMode::Greedy }, },
							{ SpecificSymbol{ 'x' }, { 1, 1, SequenceMode::Greedy }, },
						} },
						{ 0, Sequence::c_max, SequenceMode::Greedy },
					}
				},
				{
					{ SpecificSymbol{ 's' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 'v' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 't' }, { 1, 1, SequenceMode::Greedy }, },
				}
			} },
			{ 1, 1, SequenceMode::Greedy }
		} }
	},

	// Zero or more quantifier.
	{
		"B*",
		{
			{ SpecificSymbol{ 'B' }, { 0, Sequence::c_max, SequenceMode::Greedy }, },
		}
	},

	// One or more quantifier.
	{
		"WrV+",
		{
			{ SpecificSymbol{ 'W' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ 'r' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ 'V' }, { 1, Sequence::c_max, SequenceMode::Greedy }, },
		}
	},

	// Zero or one quentifier.
	{
		"6f?",
		{
			{ SpecificSymbol{ '6' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ 'f' }, { 0, 1, SequenceMode::Greedy }, },
		}
	},

	// Specific amount quantifier (range).
	{
		"z{354,789}",
		{
			{ SpecificSymbol{ 'z' }, { 354, 789, SequenceMode::Greedy }, },
		}
	},

	// Specific amount quantifier (single value).
	{
		"Q{1234}W",
		{
			{ SpecificSymbol{ 'Q' }, { 1234, 1234, SequenceMode::Greedy }, },
			{ SpecificSymbol{ 'W' }, { 1, 1, SequenceMode::Greedy }, }
		}
	},

	// Specific amount quantifier (lower bound).
	{
		"s{34,}t",
		{
			{ SpecificSymbol{ 's' }, { 34, Sequence::c_max, SequenceMode::Greedy }, },
			{ SpecificSymbol{ 't' }, { 1, 1, SequenceMode::Greedy }, }
		}
	},

	// Specific amount quantifier (upper bound).
	{
		"J{,786}_",
		{
			{ SpecificSymbol{ 'J' }, { 0, 786, SequenceMode::Greedy }, },
			{ SpecificSymbol{ '_' }, { 1, 1, SequenceMode::Greedy }, }
		}
	},

	// Quantifier for brackets.
	{
		"l(Qwe)+P",
		{
			{ SpecificSymbol{ 'l' }, { 1, 1, SequenceMode::Greedy }, },
			{
				BracketExpression
				{ {
					{ SpecificSymbol{ 'Q' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 'w' }, { 1, 1, SequenceMode::Greedy }, },
					{ SpecificSymbol{ 'e' }, { 1, 1, SequenceMode::Greedy }, },
				} },
				{ 1, Sequence::c_max, SequenceMode::Greedy }
			},
			{ SpecificSymbol{ 'P' }, { 1, 1, SequenceMode::Greedy }, },
		}
	},

	// Quantifier mode - possessive.
	{ "S?+", { { SpecificSymbol{ 'S' }, { 0, 1, SequenceMode::Possessive } } } },
	{ "@*+", { { SpecificSymbol{ '@' }, { 0, Sequence::c_max, SequenceMode::Possessive } } } },
	{ "g++", { { SpecificSymbol{ 'g' }, { 1, Sequence::c_max, SequenceMode::Possessive } } } },
	{ "_{7,12}+", { { SpecificSymbol{ '_' }, { 7, 12, SequenceMode::Possessive } } } },
	{ "F{345}+", { { SpecificSymbol{ 'F' }, { 345, 345, SequenceMode::Possessive } } } },
	{ "%{17,}+", { { SpecificSymbol{ '%' }, { 17, Sequence::c_max, SequenceMode::Possessive } } } },
	{ "K{,20}+", { { SpecificSymbol{ 'K' }, { 0, 20, SequenceMode::Possessive } } } },

	// Quantifier mode - lzy.
	{ "S??", { { SpecificSymbol{ 'S' }, { 0, 1, SequenceMode::Lazy } } } },
	{ "@*?", { { SpecificSymbol{ '@' }, { 0, Sequence::c_max, SequenceMode::Lazy } } } },
	{ "g+?", { { SpecificSymbol{ 'g' }, { 1, Sequence::c_max, SequenceMode::Lazy } } } },
	{ "_{7,12}?", { { SpecificSymbol{ '_' }, { 7, 12, SequenceMode::Lazy } } } },
	{ "F{345}?", { { SpecificSymbol{ 'F' }, { 345, 345, SequenceMode::Lazy } } } },
	{ "%{17,}?", { { SpecificSymbol{ '%' }, { 17, Sequence::c_max, SequenceMode::Lazy } } } },
	{ "K{,20}?", { { SpecificSymbol{ 'K' }, { 0, 20, SequenceMode::Lazy } } } },

	// Non-ASCII symbols.
	{
		"ДёСÜ☭",
		{
			{ SpecificSymbol{ U'Д' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ U'ё' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ U'С' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ U'Ü' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ U'☭' }, { 1, 1, SequenceMode::Greedy }, },
		}
	},

	// Basic escape sequences.
	{ "\\[", { { SpecificSymbol{ '[' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\]", { { SpecificSymbol{ ']' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\{", { { SpecificSymbol{ '{' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\}", { { SpecificSymbol{ '}' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\(", { { SpecificSymbol{ '(' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\)", { { SpecificSymbol{ ')' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\^", { { SpecificSymbol{ '^' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\$", { { SpecificSymbol{ '$' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\.", { { SpecificSymbol{ '.' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\*", { { SpecificSymbol{ '*' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\+", { { SpecificSymbol{ '+' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\|", { { SpecificSymbol{ '|' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\?", { { SpecificSymbol{ '?' }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "\\\\", { { SpecificSymbol{ '\\' }, { 1, 1, SequenceMode::Greedy }, } } },

	// Basic escape sequences for "OneOf".
	{ "[\\[]", { { OneOf{ {'['}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\]]", { { OneOf{ {']'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\{]", { { OneOf{ {'{'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\}]", { { OneOf{ {'}'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\(]", { { OneOf{ {'('}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\)]", { { OneOf{ {')'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\^]", { { OneOf{ {'^'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\$]", { { OneOf{ {'$'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\.]", { { OneOf{ {'.'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\*]", { { OneOf{ {'*'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\+]", { { OneOf{ {'+'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\|]", { { OneOf{ {'|'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\?]", { { OneOf{ {'?'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
	{ "[\\\\]", { { OneOf{ {'\\'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
};

INSTANTIATE_TEST_CASE_P(P, ParseTest, testing::ValuesIn(c_test_data));

} // namespace

} // namespace RegPanzer
