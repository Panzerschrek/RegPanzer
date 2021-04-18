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

		std::string_view str= c.input_str;
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

	// Match sequence of letters with 's' at end.
	{
		"[a-z]*s",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Non-empty string with no words - no matches.
				"@6656%%1",
				{},
			},
			{ // Single letter 's' - single match.
				"s",
				{ {0, 1} },
			},
			{ // Three letters 's' - single word match.
				"sss",
				{ {0, 3} },
			},
			{ // Single match in middle of string .
				"az ijs ife",
				{ {3, 6} },
			},
			{ // Two matches.
				"dawd wtes omngs",
				{ {5, 9}, {10, 15} }
			},
			{ // Word with 's' inside and at end.
				"this is success",
				{ {0, 4}, {5, 7}, {8, 15} }
			},
			{ // Part of word before 's' + 's' itself.
				"trswtb",
				{ {0, 3} }
			},
			{ // Words with 's' at beginning - match only first letter.
				"shrh fe shty j sgh",
				{ {0, 1}, {8, 9}, {15, 16} }
			},
		},
	},

	// Match groups. Grouping symbols are just ignored.
	{
		"(b(g(r)t))",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Non-empty string with no matches.
				"@fafthtjtysaxvmkl",
				{},
			},
			{ // Match result is whole string.
				"bgrt",
				{ {0, 4} }
			},
			{ // Match in middle of string.
				"awdawda54 bgrtwd 5fa",
				{ {10, 14} }
			},
			{ // Match multiple times.
				"febgrtwdawbgrt96m ]w wbgrtpok53",
				{ {2, 6}, {10, 14}, {22, 26} }
			},
		}
	},

	// Match sequence of groups.
	{
		"(ZX)+",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Non-empty string with no matches.
				"ZWXXZ",
				{},
			},
			{ // Single match of sequence with one element.
				"ZX",
				{ {0, 2} }
			},
			{ // Single match of sequence with multiple elements.
				"RRRZXZXZXGGB",
				{ {3, 9} }
			},
			{ // Multiple matches of sequance with one element.
				"B ZXq WZX daw wWXZ, ZX",
				{ {2, 4}, {7, 9}, {20, 22} }
			},
			{ // Break long sequence with one symbol.
				"ZXZXZXXZXZXZX",
				{ {0, 6}, {7, 13} }
			},
		}
	},

	// Match two sequences - word with decimal number at end.
	{
		"[a-z]+[0-9]+",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string with no matches.
				"whereismynumber",
				{}
			},
			{ // Non-empty string with no matches.
				"5672",
				{}
			},
			{ // Single match of whole string.
				"masyana85",
				{ {0, 9} }
			},
			{ // Single match with shortest length.
				"da ladno q4 kruto",
				{ {9, 11} }
			},
			{ // Word with number in middle.
				"  r av75jrtr",
				{ {4, 8 } }
			},
			{ // Word with number in middle and at end.
				"raz1dva2 - lol?",
				{ {0, 4}, {4, 8} }
			},
			{ // Two separate words with number ad end
				" f1 ghr9563-",
				{ {1, 3}, {4, 11} }
			},
		}
	},

	// Match sequence at end after single symobl.
	{
		"[a-zA-Z][a-zA-Z0-9]*",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string with no matches.
				"_$!",
				{}
			},
			{ // '-' is not a one of elements but special symbol for ranges.
				"-",
				{}
			},
			{ // Single match of single symbol.
				" $6 G ",
				{ {4, 5} }
			},
			{ // Multiple matches of single symbol.
				"a b T R 5 O 44 f",
				{ {0, 1}, {2, 3}, {4, 5}, {6, 7}, {10, 11}, {15, 16} }
			},
			{ // Several different matches.
				"Q q4 Quake3 r46tR6 04 Zz, Gn4444 S Hnr eeee",
				{ {0, 1}, {2, 4}, {5, 11}, {12, 18}, {22, 24}, {26, 32}, {33, 34}, {35, 38}, {39, 43} }
			},
		}
	},

	// Match sequence with exact size.
	{
		"A{3}",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string with no matches.
				"BBB",
				{}
			},
			{ // Not enough symbols for match.
				"QAvAAwwAA1A",
				{}
			},
			{ // Single match for whole string.
				"AAA",
				{ {0, 3} }
			},
			{ // Two matches for sequence longer that needed.
				"AAAAAAA",
				{ {0, 3}, {3, 6} }
			},
			{ // Several separate matches.
				"ABCWWAAAAGHTAAAWGGBLAAAAAGGEEAAAAAAH",
				{ {5, 8}, {12, 15}, {20, 23}, {29, 32}, {32, 35} }
			}
		}
	},

	// Match sequence of groups with exact size.
	{
		"(ta){2}",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string with no matches.
				"rtra",
				{}
			},
			{ // Not enough symbols for match.
				"tat",
				{}
			},
			{ // Break sequence.
				"taqta",
				{}
			},
			{ // Match for whole string.
				"tata",
				{ {0, 4} }
			},
			{ // Match in middle of string.
				"atatat",
				{ {1, 5} }
			},
			{ // Match only first two sequences.
				"tatata",
				{ {0, 4} }
			},
			{ // Match several sequential sequences.
				"tatatatatatatata",
				{ {0, 4}, {4, 8}, {8, 12}, {12, 16} }
			},
			{ // Match several separate sequences.
				"ratatas statata ta tatutataW wtatatata",
				{ {2, 6}, {9, 13}, {23, 27}, {30, 34}, {34, 38} }
			},
		},
	},

	// Match sequence with size in range.
	{
		"f{3,7}",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string with no matches.
				"awjw",
				{}
			},
			{ // Wrong case.
				"FFFFFFFFFF",
				{}
			},
			{ // Not enought symbols.
				"ff",
				{}
			},
			{ // Minimal match for whole string.
				"fff",
				{ {0, 3} }
			},
			{ // Maximal match for whole string.
				"fffffff",
				{ {0, 7} }
			},
			{ // Match sequence up to limit, igore left symbols.
				"fffffffff",
				{ {0, 7} }
			},
			{ // Two sequential sequences.
				"fffffffffff",
				{ {0, 7}, {7, 11} }
			},
			{ // Two sequential sequences with size equal to limit.
				"ffffffffffffff",
				{ {0, 7}, {7, 14} }
			},
			{ // Two sequential sequences with size equal to limit, left small tail.
				"fffffffffffffff",
				{ {0, 7}, {7, 14} }
			},
			{ // Several sequenses.
				"wdawdimffwasfawafffawd[o,ffffwdawo[pffffffffffwdwop;fffwl;affffp[pwdwadkmgpfffff",
				{ {16, 19}, {25, 29}, {36, 43}, {43, 46}, {52, 55}, {59, 63}, {75, 80} }
			},
		}
	},

	// Match sequence with % 3 == 0 elements.
	{
		"(s{3})+",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string with no matches.
				"awjw",
				{}
			},
			{ // Not enough symbols.
				"ss",
				{}
			},
			{ // Exact match with minimal size.
				"sss",
				{ {0, 3} }
			},
			{ // Exact match for beginning of strig, ignore tail elements.
				"sssss",
				{ {0, 3} }
			},
			{ // Multiple "sss" elements in single match.
				"ssssssssssss",
				{ {0, 12} }
			},
			{ // Multiple "sss" elements in single match, ignore tail elements.
				"ssssssssssssss",
				{ {0, 12} }
			},
			{ // Several matches.
				"wrgsssgrhessswdssssss wa wwwsssssssss bnrklmssSsssss",
				{ {3, 6}, {10, 13}, {15, 21}, {28, 37}, {47, 50} }
			},
		}
	},

	// Simple alternative.
	{
		"a|b",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string with no matches.
				"crt k",
				{}
			},
			{ // One match for first alternative.
				"a",
				{ {0, 1} }
			},
			{ // One match for second alternative.
				"b",
				{ {0, 1} }
			},
			{ // All matches is first alternative.
				"awwaaca",
				{ {0, 1}, {3, 4}, {4, 5}, {6, 7} }
			},
			{ // All matches is second alternative.
				"hjtbpoy,k][pkboesfbbb",
				{ {3, 4}, {13, 14}, {18, 19}, {19, 20}, {20, 21} }
			},
			{ // Matches for both alternatives.
				"bthawdbbwdaaefbawdwaab",
				{ {0, 1}, {3, 4}, {6, 7}, {7, 8}, {10, 11}, {11, 12}, {14, 15}, {15, 16}, {19, 20}, {20, 21}, {21, 22} }
			},
		}
	},

	// Multiple alternatives with groups.
	{
		"Q|(zz)|(wtf)",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string with no matches.
				"zwtq",
				{}
			},
			{ // Match single first alternative.
				"Q",
				{ {0, 1} },
			},
			{ // Match single second alternative.
				"zz",
				{ {0, 2} },
			},
			{ // Match single third alternative.
				"wtf",
				{ {0, 3} },
			},
			{ // Match multiple alternatives.
				"wdwzz w Qwtf awtf zzQ wtfzz",
				{ {3, 5}, {8, 9}, {9, 12}, {14, 17}, {18, 20}, {20, 21}, {22, 25}, {25, 27} }
			},
		}
	},

	// Multiple alternative sequences.
	{
		"[a-z]+|[A-Z]+|[0-9]+", // lowercase word or uppercase word or decimal number.
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Match single symbol for first alternative.
				"g",
				{ {0, 1} }
			},
			{ // Match single symbol for second alternative.
				"O",
				{ {0, 1} }
			},
			{ // Match single symbol for third alternative.
				"7",
				{ {0, 1} }
			},
			{ // Match first sequence.
				"hfnueabnt",
				{ {0, 9} }
			},
			{ // Match second sequence.
				"BTANEI",
				{ {0, 6} }
			},
			{ // Match third sequence.
				"88005353535",
				{ {0, 11} }
			},
			{ // Match sequence in middle of string.
				"$^&!@-RGB~~",
				{ {6, 9} }
			},
			{ // Match sequential sequences.
				"RTGbfcd12",
				{ {0, 3}, {3, 7}, {7, 9} }
			},
			{ // Match all together.
				"Word wiTH number77s 666 times NONE 3456GG",
				{ {0, 1}, {1, 4}, {5, 7}, {7, 9}, {10, 16}, {16, 18}, {18, 19}, {20, 23}, {24, 29}, {30, 34}, {35, 39}, {39, 41} }
			},
		}
	},

	// Sequence of alternative elements.
	{
		"(0|1)+",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches
				"etssef",
				{}
			},
			{ // Match single symobl from first alternative.
				"0",
				{ {0, 1} }
			},
			{ // Match single symobl from second alternative.
				"1",
				{ {0, 1} }
			},
			{ // Match sequence of first alternatives.
				"w11111GG",
				{ {1, 6} }
			},
			{ // Match sequence of second alternatives.
				"--00000",
				{ {2, 7} }
			},
			{ // Match sequence with both alternatives.
				"1110111000101101",
				{ {0, 16} }
			},
			{ // Match several sequences.
				"001 111 000 ww0z1 1-0*11 = 1010111",
				{ {0, 3}, {4, 7}, {8, 11}, {14, 15}, {16, 17}, {18, 19}, {20, 21}, {22, 24}, {27, 34} }
			},
		}
	},

	// Non-ASCII symbols match.
	{
		"ё",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches
				"жЖha",
				{}
			},
			{ // Single match for whole string.
				"ё",
				{ {0, 2} }
			},
			{ // Single match in middle of string.
				"Аёш",
				{ {2, 4} }
			},
			{ // Multiple matches.
				"grtё1ё☯ё",
				{ {3, 5}, {6, 8}, {11, 13} }
			},
		},
	},
};

INSTANTIATE_TEST_CASE_P(M, CheckMatchTest, testing::ValuesIn(c_test_data));

} // namespace

} // namespace RegPanzer
