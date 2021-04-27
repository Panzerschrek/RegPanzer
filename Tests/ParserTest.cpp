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

	// Group.
	{
		"A(bc)ef",
		{
			{ SpecificSymbol{ 'A' }, { 1, 1, SequenceMode::Greedy }, },
			{
				Group
				{
					1,
					{
						{ SpecificSymbol{ 'b' }, { 1, 1, SequenceMode::Greedy }, },
						{ SpecificSymbol{ 'c' }, { 1, 1, SequenceMode::Greedy }, },
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			},
			{ SpecificSymbol{ 'e' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ 'f' }, { 1, 1, SequenceMode::Greedy }, },
		}
	},

	// Group inside group.
	{
		"g(bc(RT)Q)ww",
		{
			{ SpecificSymbol{ 'g' }, { 1, 1, SequenceMode::Greedy }, },
			{
				Group
				{
					1,
					{
						{ SpecificSymbol{ 'b' }, { 1, 1, SequenceMode::Greedy }, },
						{ SpecificSymbol{ 'c' }, { 1, 1, SequenceMode::Greedy }, },
						{
							Group
							{
								2,
								{
									{ SpecificSymbol{ 'R' }, { 1, 1, SequenceMode::Greedy }, },
									{ SpecificSymbol{ 'T' }, { 1, 1, SequenceMode::Greedy }, },
								}
							},
							{ 1, 1, SequenceMode::Greedy }
						},
						{ SpecificSymbol{ 'Q' }, { 1, 1, SequenceMode::Greedy }, },
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			},
			{ SpecificSymbol{ 'w' }, { 1, 1, SequenceMode::Greedy }, },
			{ SpecificSymbol{ 'w' }, { 1, 1, SequenceMode::Greedy }, },
		}
	},

	// Several groups - sequential and nested.
	{
		"(vb)(C(D)(E))+(.(F(G)H))",
		{
			{
				Group
				{
					1,
					{
						{ SpecificSymbol{ 'v' }, { 1, 1, SequenceMode::Greedy } },
						{ SpecificSymbol{ 'b' }, { 1, 1, SequenceMode::Greedy } },
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			},
			{
				Group
				{
					2,
					{
						{ SpecificSymbol{ 'C' }, { 1, 1, SequenceMode::Greedy } },
						{
							Group
							{
								3,
								{
									{ SpecificSymbol{ 'D' }, { 1, 1, SequenceMode::Greedy } }
								}
							},
							{ 1, 1, SequenceMode::Greedy }
						},
						{
							Group
							{
								4,
								{
									{ SpecificSymbol{ 'E' }, { 1, 1, SequenceMode::Greedy } }
								}
							},
							{ 1, 1, SequenceMode::Greedy }
						},
					}
				},
				{ 1, Sequence::c_max, SequenceMode::Greedy }
			},
			{
				Group
				{
					5,
					{
						{ AnySymbol{}, { 1, 1, SequenceMode::Greedy } },
						{
							Group
							{
								6,
								{
									{ SpecificSymbol{ 'F' }, { 1, 1, SequenceMode::Greedy } },
									{
										Group
										{
											7,
											{
												{ SpecificSymbol{ 'G' }, { 1, 1, SequenceMode::Greedy } }
											}
										},
										{ 1, 1, SequenceMode::Greedy }
									},
									{ SpecificSymbol{ 'H' }, { 1, 1, SequenceMode::Greedy } },
								}
							},
							{ 1, 1, SequenceMode::Greedy }
						},
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			},
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
						Group
						{
							1,
							{
								{ SpecificSymbol{ 'V' }, { 1, 1, SequenceMode::Greedy }, },
								{ SpecificSymbol{ 'x' }, { 1, 1, SequenceMode::Greedy }, },
							}
						},
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
				Group
				{
					1,
					{
						{ SpecificSymbol{ 'Q' }, { 1, 1, SequenceMode::Greedy }, },
						{ SpecificSymbol{ 'w' }, { 1, 1, SequenceMode::Greedy }, },
						{ SpecificSymbol{ 'e' }, { 1, 1, SequenceMode::Greedy }, },
					}
				},
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

	// Quantifier mode - lazy.
	{ "S??", { { SpecificSymbol{ 'S' }, { 0, 1, SequenceMode::Lazy } } } },
	{ "@*?", { { SpecificSymbol{ '@' }, { 0, Sequence::c_max, SequenceMode::Lazy } } } },
	{ "g+?", { { SpecificSymbol{ 'g' }, { 1, Sequence::c_max, SequenceMode::Lazy } } } },
	{ "_{7,12}?", { { SpecificSymbol{ '_' }, { 7, 12, SequenceMode::Lazy } } } },
	{ "F{345}?", { { SpecificSymbol{ 'F' }, { 345, 345, SequenceMode::Lazy } } } },
	{ "%{17,}?", { { SpecificSymbol{ '%' }, { 17, Sequence::c_max, SequenceMode::Lazy } } } },
	{ "K{,20}?", { { SpecificSymbol{ 'K' }, { 0, 20, SequenceMode::Lazy } } } },

	{ // Simple lookahead.
		"(?=F)",
		{
			{
				Look
				{
					true,
					true,
					{
						{ SpecificSymbol{ 'F' }, { 1, 1, SequenceMode::Greedy } }
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			}
		}
	},

	{ // Negative lookahead with several elements inside.
		"(?!zGr)",
		{
			{
				Look
				{
					true,
					false,
					{
						{ SpecificSymbol{ 'z' }, { 1, 1, SequenceMode::Greedy } },
						{ SpecificSymbol{ 'G' }, { 1, 1, SequenceMode::Greedy } },
						{ SpecificSymbol{ 'r' }, { 1, 1, SequenceMode::Greedy } },
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			}
		}
	},

	{ // Lookahead after element.
		"Q(?!u)",
		{
			{ SpecificSymbol{ 'Q' }, { 1, 1, SequenceMode::Greedy } },
			{
				Look
				{
					true,
					false,
					{
						{ SpecificSymbol{ 'u' }, { 1, 1, SequenceMode::Greedy } },
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			},
		}
	},

	{ // Two lookaheads.
		"F(?=[0-9])(?!7)",
		{
			{ SpecificSymbol{ 'F' }, { 1, 1, SequenceMode::Greedy } },
			{
				Look
				{
					true,
					true,
					{
						{ OneOf{ {}, { {'0', '9'} }, false }, { 1, 1, SequenceMode::Greedy } },
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			},
			{
				Look
				{
					true,
					false,
					{
						{ SpecificSymbol{ '7' }, { 1, 1, SequenceMode::Greedy } },
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			},
		}
	},

	{ // Lookahead and alternatives.
		"W(?=d)|s",
		{
			{
				Alternatives
				{ {
					{
						{ SpecificSymbol{ 'W' }, { 1, 1, SequenceMode::Greedy } },
						{
							Look
							{
								true,
								true,
								{
									{ SpecificSymbol{ 'd' }, { 1, 1, SequenceMode::Greedy } },
								}
							},
							{ 1, 1, SequenceMode::Greedy }
						}
					},
					{
						{ SpecificSymbol{ 's' }, { 1, 1, SequenceMode::Greedy } },
					},
				} },
				{ 1, 1, SequenceMode::Greedy }
			}
		}
	},

	{ // Lookahead and alternatives.
		"Z|4(?=0{3})",
		{
			{
				Alternatives
				{ {
					{
						{ SpecificSymbol{ 'Z' }, { 1, 1, SequenceMode::Greedy } },
					},
					{
						{ SpecificSymbol{ '4' }, { 1, 1, SequenceMode::Greedy } },
						{
							Look
							{
								true,
								true,
								{
									{ SpecificSymbol{ '0' }, { 3, 3, SequenceMode::Greedy } },
								}
							},
							{ 1, 1, SequenceMode::Greedy }
						}
					},
				} },
				{ 1, 1, SequenceMode::Greedy }
			}
		}
	},

	{ // Alternatives inside lookahead.
		"n(?=0|1)",
		{
			{ SpecificSymbol{ 'n' }, { 1, 1, SequenceMode::Greedy } },
			{
				Look
				{
					true,
					true,
					{
						{
							Alternatives
							{ {
								{
									{ SpecificSymbol{ '0' }, { 1, 1, SequenceMode::Greedy } }
								},
								{
									{ SpecificSymbol{ '1' }, { 1, 1, SequenceMode::Greedy } }
								},
							} },
							{ 1, 1, SequenceMode::Greedy }
						},
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			}
		}
	},

	{ // Lookbehind (negative).
		"[a-z]+(?<!n)",
		{
			{ OneOf{ {}, { {'a', 'z'} }, false }, { 1, Sequence::c_max, SequenceMode::Greedy } },
			{
				Look
				{
					false,
					false,
					{
						{ SpecificSymbol{ 'n' }, { 1, 1, SequenceMode::Greedy } }
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			}
		}
	},

	{ // Lookbehind (positive).
		"(?<=q)S",
		{
			{
				Look
				{
					false,
					true,
					{
						{ SpecificSymbol{ 'q' }, { 1, 1, SequenceMode::Greedy } }
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			},
			{ SpecificSymbol{ 'S' }, { 1, 1, SequenceMode::Greedy } }
		}
	},

	{ // Noncapturing group.
		"(?:gf+d){44,}",
		{
			{
				NonCapturingGroup
				{{
					{ SpecificSymbol{ 'g' }, { 1, 1, SequenceMode::Greedy } },
					{ SpecificSymbol{ 'f' }, { 1, Sequence::c_max, SequenceMode::Greedy } },
					{ SpecificSymbol{ 'd' }, { 1, 1, SequenceMode::Greedy } },
				}},
				{ 44, Sequence::c_max, SequenceMode::Greedy }
			}
		}
	},

	{ // Atomic group.
		"(?>a|bc|x)",
		{
			{
				AtomicGroup
				{ {
					{
						Alternatives
						{ {
							{
								{ SpecificSymbol{ 'a' }, { 1, 1, SequenceMode::Greedy } },
							},
							{
								{ SpecificSymbol{ 'b' }, { 1, 1, SequenceMode::Greedy } },
								{ SpecificSymbol{ 'c' }, { 1, 1, SequenceMode::Greedy } },
							},
							{
								{ SpecificSymbol{ 'x' }, { 1, 1, SequenceMode::Greedy } },
							},
						} },
						{ 1, 1, SequenceMode::Greedy }
					},
				} },
				{ 1, 1, SequenceMode::Greedy }
			}
		}
	},

	{ // Conditional element.
		"(?(?=0)[0-9]+|[a-z]+)",
		{
			{
				ConditionalElement
				{
					{
						true,
						true,
						{
							{ SpecificSymbol{ '0' }, { 1, 1, SequenceMode::Greedy } }
						}
					},
					{ {
						{
							{ OneOf{ {}, { {'0', '9'} }, false }, { 1, Sequence::c_max, SequenceMode::Greedy } }
						},
						{
							{ OneOf{ {}, { {'a', 'z'} }, false }, { 1, Sequence::c_max, SequenceMode::Greedy } }
						}
					} }
				},
				{ 1, 1, SequenceMode::Greedy }
			}
		}
	},

	{ // Recursion for whole expression.
		"a(?R)?b",
		{
			{ SpecificSymbol{ 'a' }, { 1, 1, SequenceMode::Greedy } },
			{ RecursionGroup{ 0 }, { 0, 1, SequenceMode::Greedy } },
			{ SpecificSymbol{ 'b' }, { 1, 1, SequenceMode::Greedy } },
		}
	},

	{ // Recursion for part of expression.
		"(Q)(z(?2)?w)T",
		{
			{
				Group
				{
					1,
					{ { SpecificSymbol{ 'Q' }, { 1, 1, SequenceMode::Greedy } } }
				},
				{ 1, 1, SequenceMode::Greedy }
			},
			{
				Group
				{
					2,
					{
						{ SpecificSymbol{ 'z' }, { 1, 1, SequenceMode::Greedy } },
						{ RecursionGroup{ 2 }, { 0, 1, SequenceMode::Greedy } },
						{ SpecificSymbol{ 'w' }, { 1, 1, SequenceMode::Greedy } },
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			},
			{ SpecificSymbol{ 'T' }, { 1, 1, SequenceMode::Greedy } },
		}
	},

	{ // Recursion for '0' - recursion for whole pattern.
		"([a-z])(?0)?\\1",
		{
			{
				Group
				{
					1,
					{ { OneOf{ {}, { {'a', 'z'} }, false }, { 1, 1, SequenceMode::Greedy } } }
				},
				{ 1, 1, SequenceMode::Greedy }
			},
			{ RecursionGroup{ 0 }, { 0, 1, SequenceMode::Greedy } },
			{ BackReference{ 1 }, { 1, 1, SequenceMode::Greedy } },
		}
	},

	{ // Simplest backreference.
		"(wa)\\1",
		{
			{
				Group
				{
					1,
					{
						{ SpecificSymbol{ 'w' }, { 1, 1, SequenceMode::Greedy } },
						{ SpecificSymbol{ 'a' }, { 1, 1, SequenceMode::Greedy } }
					}
				},
				{ 1, 1, SequenceMode::Greedy }
			},
			{
				BackReference{ 1 },
				{ 1, 1, SequenceMode::Greedy }
			},
		}
	},

	{ // Some backreferences (should parse successfully, but regex itself is not valid).
		"a\\1?b\\5{7,12}c\\9+d\\4*",
		{
			{ SpecificSymbol{ 'a' }, { 1, 1, SequenceMode::Greedy } },
			{ BackReference{ 1 }, { 0, 1, SequenceMode::Greedy } },
			{ SpecificSymbol{ 'b' }, { 1, 1, SequenceMode::Greedy } },
			{ BackReference{ 5 }, { 7, 12, SequenceMode::Greedy } },
			{ SpecificSymbol{ 'c' }, { 1, 1, SequenceMode::Greedy } },
			{ BackReference{ 9 }, { 1, Sequence::c_max, SequenceMode::Greedy } },
			{ SpecificSymbol{ 'd' }, { 1, 1, SequenceMode::Greedy } },
			{ BackReference{ 4 }, { 0, Sequence::c_max, SequenceMode::Greedy } },
		}
	},

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
	{ "\\/", { { SpecificSymbol{ '/' }, { 1, 1, SequenceMode::Greedy }, } } },

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
	{ "[\\/]", { { OneOf{ {'/'}, {}, false }, { 1, 1, SequenceMode::Greedy }, } } },
};

INSTANTIATE_TEST_CASE_P(P, ParseTest, testing::ValuesIn(c_test_data));

} // namespace

} // namespace RegPanzer
