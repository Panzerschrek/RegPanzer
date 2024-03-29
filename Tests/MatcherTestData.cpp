#include "MatcherTestData.hpp"

namespace RegPanzer
{

const MatcherTestDataElement g_matcher_test_data[]
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
			{ // String is equal to regex.
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

	// Match any symbol.
	{
		"r.wd",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Non-empty string - no matches.
				"rsc wda",
				{},
			},
			{ // Missing "any" symbol - no matches.
				"rwd",
				{},
			},
			{ // Too many  "any" symbols - no matches.
				"rz+wd",
				{},
			},
			{ // Match whole string.
				"rzwd",
				{ {0, 4} }
			},
			{ // Match whole string with different "any" symbol.
				"rQwd",
				{ {0, 4} }
			},
			{ // Match whole string with non-letter "any" symbol.
				"r7wd",
				{ {0, 4} }
			},
			{ // Match whole string with special "any" symbol.
				"r#wd",
				{ {0, 4} }
			},
			{ // Match whole string with space as "any" symbol.
				"r wd",
				{ {0, 4} }
			},
			{ // Match whole string with non-ascii "any" symbol.
				"rГwd",
				{ {0, 5} }
			},
			{ // Match whole string with non-ascii "any" symbol.
				"r굞wd",
				{ {0, 6} }
			},
			{ // Multiple matches.
				"warJwddr+wdd br/wadnr wd",
				{ {2, 6}, {7, 11}, {20, 24} }
			}
		},
	},

	// Match number symbol class.
	{
		"\\d",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // All possible matches.
				"0123456789",
				{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 8}, {8, 9}, {9, 10} }
			},
			{ // No matches - symbols outside range.
				"gb_Z!~ ",
				{}
			},
		}
	},

	// Match NOT number symbol class.
	{
		"\\D",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // All possible symbols in range, should no match anything.
				"0123456789",
				{}
			},
			{ // All symbols are outside range - should match them all.
				"gb_Z!~ ",
				{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7} }
			},
		}
	},

	// Match word symbol class.
	{
		"\\w",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // All possible matches.
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz0123456789",
				{
					{ 0,  1}, { 1,  2}, { 2,  3}, { 3,  4}, { 4,  5}, { 5,  6}, { 6,  7}, { 7,  8}, { 8,  9}, { 9, 10}, {10, 11}, {11, 12}, {12, 13}, {13, 14}, {14, 15}, {15, 16}, {16, 17}, {17, 18}, {18, 19}, {19, 20}, {20, 21}, {21, 22}, {22, 23}, {23, 24}, {24, 25}, {25, 26},
					{26, 27},
					{27, 28}, {28, 29}, {29, 30}, {30, 31}, {31, 32}, {32, 33}, {33, 34}, {34, 35}, {35, 36}, {36, 37}, {37, 38}, {38, 39}, {39, 40}, {40, 41}, {41, 42}, {42, 43}, {43, 44}, {44, 45}, {45, 46}, {46, 47}, {47, 48}, {48, 49}, {49, 50}, {50, 51}, {51, 52}, {52, 53},
					{53, 54}, {54, 55}, {55, 56}, {56, 57}, {57, 58}, {58, 59}, {59, 60}, {60, 61}, {61, 62}, {62, 63},

				}
			},
			{ // No matches - symbols outside range.
				"%& ()!-Я",
				{}
			},
		}
	},

	// Match NOT word symbol class.
	{
		"\\W",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // All possible matches.
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz0123456789",
				{}
			},
			{ // All matches - symbols outside range.
				"%& ()!-`",
				{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 8} }
			},
		}
	},

	// Match space symbol class.
	{
		"\\s",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // All possible mathes.
				"\r\n\t\f\v ",
				{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6} }
			},
			{ // No matches for non-space symbols.
				"wbA2#9-=~`",
				{}
			},
		}
	},

	// Match NOT space symbol class.
	{
		"\\S",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // All possible space symbols - no matches.
				"\r\n\t\f\v ",
				{}
			},
			{ // No matches for non-space symbols.
				"wbA2#9-=~`",
				{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}, {7, 8}, {8, 9}, {9, 10} }
			},
		}
	},

	{ // Macth hexademical (2 and 4 digits) constants.
		"\\x6c\\x53\\u002B\\u0447\\u091b",
		{
			{
				"lS+чछ",
				{ {0, 8} }
			}
		},
	},

	// Match inverted "OneOf".
	{
		"[^0-9]",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Single symbol in range - no matches.
				"6",
				{}
			},
			{ // Multiple symbols in range - no matches.
				"68547",
				{},
			},
			{ // Single symbol outside range - one match.
				"z",
				{ {0, 1} }
			},
			{ // Multiple symbols outside range - multiple matches.
				"p R-!~\\",
				{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7} }
			},
			{ // Some symbols in string are forbidden.
				"w56n 1Z",
				{ {0, 1}, {3, 4}, {4, 5}, {6, 7} }
			},
		}
	},

	// Match inverted "OneOf" with list of symbols (including '^').
	{
		"[^Q^q]",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Single symbol in list - no matches.
				"q",
				{}
			},
			{ // Single symbol in list - no matches.
				"Q",
				{}
			},
			{ // Single symbol in list - no matches.
				"^",
				{}
			},
			{ // Single symbol outside list - one match.
				"F",
				{ {0, 1} }
			},
			{ // Multiple symbols outside list - multiple matches.
				"aPr $",
				{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5} }
			},
			{ // Some symbols in string are forbidden.
				"aQc^ qz",
				{ {0, 1}, {2, 3}, {4, 5}, {6, 7} }
			},
		}
	},

	// Match optional element.
	{
		"[0-9]+f?",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Non-empty string - no matches.
				"wdadsf",
				{},
			},
			{ // Shortest possible match.
				"0",
				{ {0, 1} }
			},
			{ // Match with optional element present.
				"12f",
				{ {0, 3} }
			},
			{ // Match with duplicated optional element - should match only first entry.
				"3ff",
				{ {0, 2} }
			},
			{ // Several sequential matches.
				"12f32312f9f724",
				{ {0, 3}, {3, 9}, {9, 11}, {11, 14} }
			},
			{ // Multiple matches.
				"wdw 454 1 j5f wwd kmn1235 owf f 653235235 44f",
				{ {4, 7}, {8, 9}, {11, 13}, {21, 25}, {32, 41}, {42, 45} }
			},
		}
	},

	// Match optional group.
	{
		"D(dr)?",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Non-empty string - no matches.
				"ddr",
				{},
			},
			{ // Shortest possible match.
				"D",
				{ {0, 1} }
			},
			{ // Match with optional group present.
				"Ddr",
				{ {0, 3} }
			},
			{ // Match with optional group repeated.
				"drDdrdrdr",
				{ {2, 5} }
			},
			{ // Several sequential matches.
				"DdrDDDdr",
				{ {0, 3}, {3, 4}, {4, 5}, {5, 8} }
			},
			{ // Multiple matches.
				"56675--Ddr D dqd WDS drdr +Ddr wdwDDdr wxczDdrDdr wxccWDdrDz FDD",
				{ {7, 10}, {11, 12}, {18, 19}, {27, 30}, {34, 35}, {35, 38}, {43, 46}, {46, 49}, {55, 58}, {58, 59}, {62, 63}, {63, 64} }
			},
		}
	},

	// Match optional sequence.
	{
		"Num([0-9]+)?",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Non-empty string - no matches.
				"12354",
				{},
			},
			{ // Shortest possible match.
				"Num",
				{ {0, 3} }
			},
			{ // Match with optional sequence present.
				"Num6",
				{ {0, 4} }
			},
			{ // Match with long sequence present.
				"Num987654321",
				{ {0, 12} }
			},
			{ // Several sequential matches.
				"NumNum545NumNum12",
				{ {0, 3}, {3, 9}, {9, 12}, {12, 17} }
			},
			{ // Multiple matches.
				"wdNum1 aNumz Num + Num6124zs - Num5 WRNum12",
				{ {2, 6}, {8, 11}, {13, 16}, {19, 26}, {31, 35}, {38, 43} }
			},
		}
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

	// Match sequence of symbol ranges.
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
			{ // No matches for symbols outside range.
				"CAPITAL LETTERS",
				{},
			},
			{ // Single match for single symbol in range.
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

	{ // Match group with alternatives inside.
		"a(bc|b|x)cc",
		{
			{
				"abcc", // Should capture "bc" first, than found nothing return and capture "b" instead.
				{ {0, 4} }
			}
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

	// Match sequence at end after single symbol.
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

	{ // Match sequence with complex suffix. Suffix valid as sequence part.
		"[a-z]*rq",
		{

			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string with no matces.
				"wgvrwdqr",
				{}
			},
			{ // Minimap match.
				"rq",
				{ {0, 2} }
			},
			{ // Sequence with included end suffix.
				"abrqwwrq",
				{ {0, 8} }
			},
			{ // Duplicated suffix.
				"abcrqrq",
				{ {0, 7} }
			},
			{ // Sequence elements matches first suffix element.
				"rrrrq",
				{ {0, 5} }
			},
			{ // Sequence elements matches second suffix element.
				"qqqqqqqrq",
				{ {0, 9} }
			},
			{ // Match in middle of string.
				" qrqr",
				{ {1, 4} }
			},
			{ // Match multiple sequences.
				"wvbrq-- rqq wtrqrqrqwllrq zqrq dddacrqrq",
				{ {0, 5}, {8, 10}, {12, 25}, {26, 30}, {31, 40} }
			},
		},
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

	// Match lazy sequence of single symbol.
	{
		"b+?",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-emtpy string - no matches.
				"z",
				{}
			},
			{ // Minimal match.
				"b",
				{ {0, 1} }
			},
			{ // Multiple symbols - should extract only single symbol.
				"bbbbb",
				{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5} }
			},
			{ // Multiple matches in complex string.
				"wadwbokwb]owbb]wadbbb b awdb bbawd ba",
				{ {4, 5}, {8, 9}, {12, 13}, {13, 14}, {18, 19}, {19, 20}, {20, 21}, {22, 23}, {27, 28}, {29, 30}, {30, 31}, {35, 36} }
			},
		}
	},

	// Match lazy sequence with terminator present in sequence elements.
	{
		"[a-z]+?s",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No matches for non-empty string.
				"a",
				{}
			},
			{ // No matches for non-empty string.
				"awdawmn",
				{}
			},
			{ // Minimal match.
				"hs",
				{ {0, 2} },
			},
			{ // Match with sequence longer than 1.
				"abhheas",
				{ {0, 7} },
			},
			{ // Search should stop on first 's' symbol.
				"wss",
				{ {0, 2} }
			},
			{ // First 's' included because at least one symbol required in sequence.
				"ss",
				{ {0, 2} }
			},
			{ // Third 's' is not included.
				"sss",
				{ {0, 2} }
			},
			{ // Should get 2 matches for 4 's' symbols.
				"ssss",
				{ {0, 2}, {2, 4} }
			},
			{ // Search should stop on first 's' symbol. Result should have two matches.
				"abwdsgraews",
				{ {0, 5}, {5, 11} }
			},
			{ // Match in middle of string.
				"w w  hhwfoksq",
				{ {5, 12} }
			},
			{ // Multiple matches.
				" aws ssa wwss rrPOs hes, 75is saq apojeeapojtgsps ssssssw hfgassss",
				{ {1, 4}, {5, 7}, {9, 12}, {20, 23}, {27, 29}, {34, 47}, {47, 49}, {50, 52}, {52, 54}, {54, 56}, {58, 63}, {63, 65} }
			},
		}
	},

	// Match lazy sequence with min/max elements and with terminator present in sequence elements.
	{
		"[0-9]{3,7}?0",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No matches for non-empty string.
				"a",
				{}
			},
			{ // Not enough symbols in sequence.
				"670",
				{}
			},
			{ // No terminal 0.
				"376",
				{}
			},
			{ // Minimal match.
				"7430",
				{ {0, 4} }
			},
			{ // Maximal match.
				"12345670",
				{ {0, 8} }
			},
			{ // Too many symbols in sequence, beginning is ignored.
				"9876543210",
				{ {2, 10} }
			},
			{ // Take zeroes from sequence up to required count, ignore others.
				"000000",
				{ {0, 4} }
			},
			{ // Take first minimal match and then second minimal match. Tail is ignored
				"00000000000",
				{ {0, 4}, {4, 8} }
			},
			{ // Zero in the middle breaks long sequence.
				"4340320",
				{ {0, 4} }
			},
			{ // Two sequential matches.
				"5333054343310",
				{ {0, 5}, {5, 13} }
			},
			{ // Sequential matches with zeros inside.
				"5202100500",
				{ {0, 6}, {6, 10} }
			},
			{ // Multiple matches.
				"dw a043050dw 1230 wda 66666660wa w0000000wadm r000awdawdwd333awdawd6584272aw aw012334 wadw00wdaw323 0 000 w4~3450123012345510",
				{ {4, 8}, {13, 17}, {22, 30}, {34, 38}, {109, 113}, {113, 117}, {117, 125} }
			},
		}
	},

	// Match lazy sequence with lower bound and terminator with non-sequence element.
	{
		"[a-f]{3,}?n",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches.
				"awhj",
				{}
			},
			{ // No terminator - no match.
				"abcd",
				{},
			},
			{ // Not enough symbols - no match.
				"cdn",
				{},
			},
			{ // Minimal match.
				"fean",
				{ {0, 4} }
			},
			{ // Looooong match.
				"abcdeffedbcan",
				{ {0, 13} }
			},
			{ // Two sequential matches.
				" bedanaaaaabccccn",
				{ {1, 6}, {6, 17} }
			},
			{ // Multiple matches.
				"  !!aaan bcdedddddccan wwwabcn cdddddefnaabbcc ABCn aaaaaN",
				{ {4, 8}, {9, 22}, {26, 30}, {31, 40} }
			},
		}
	},

	// Match possessive sequence.
	{
		// There is no way to match this regex, because possessive sequence will always extract 'q' symbol.
		"[a-z]++q",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches
				"764!",
				{}
			},
			{ // No matches for letter sequence with 'q' at end - because of possessive mode.
				"ghfq",
				{}
			},
			{
				"qq",
				{}
			},
		}
	},

	// Match possessive sequence with length in range.
	{
		"[0-9]{2,5}+7",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches
				"daw",
				{}
			},
			{ // Not enough symbols - no match.
				"17",
				{}
			},
			{ // No match for this sequence because of possessive mode - '7' will be extracted.
				"547",
				{}
			},
			{ // Sequence finished after reqching maximum numbers of elements. After that '7' symbol may be captured.
				"012347",
				{ {0, 6} }
			},
			{ // First two symbols are ignored because there is no way to finish sequence with '7' starting from them.
				"22222227",
				{ {2, 8} }
			},
			{ // Two sequential sequences.
				"777777777777",
				{ {0, 6}, {6, 12} }
			},
			{ // Finish first sequence and left second sequence unifished (because of possessive mode).
				"66666733337",
				{ {0, 6} }
			},
			{ // Single match for string, instead of two matches in lazy mode.
				"127127",
				{ {0, 6 } }
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
			{ // Match single symbol from first alternative.
				"0",
				{ {0, 1} }
			},
			{ // Match single symbol from second alternative.
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

	// Sequence of element with non-fixed size and with following element, containing one of sequence possible start symbols.
	// Auto-possessification or fixed length element sequence optimization should not work for this.
	{
		"(1|23)*1",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{
				"1",
				{ {0, 1} },
			},
			{
				"23",
				{},
			},
			{
				"11",
				{ {0, 2} },
			},
			{
				"111",
				{ {0, 3} },
			},
			{
				"2323232323231",
				{ {0, 13} },
			},
			{
				"1231231",
				{ {0, 7} },
			},
			{
				"123",
				{ {0, 1} },  // Only "1" is extracted.
			},
		},
	},

	// Greedy sequence inside another sequence.
	// Should match properly with backtracking.
	{
		"(a+){3}",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{
				"aaa",
				{ {0, 3} },
			},
			{
				"aaaa",
				{ {0, 4} },
			},
			{
				"aaaaaaaaaa",
				{ {0, 10} },
			},
			{
				"aaaa aaaa aaaaaaaaaaaa",
				{ {0, 4}, {5, 9}, {10, 22} },
			},
		},
	},

	// Simple positive lookahead.
	{
		"w(?=Q)",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches.
				"zva",
				{}
			},
			{ // No match in lookahed - no matches.
				"w",
				{}
			},
			{ // Minimap match. Symbol in lookahed is not captured.
				"wQ",
				{ {0, 1} }
			},
			{ // Match in middle of string.
				" 12wQf_",
				{ {3, 4} }
			},
			{ // Two sequential matches.
				"wQwQ",
				{ {0, 1}, {2, 3} }
			},
		}
	},

	{ // Simple negative lookahead.
		"[a-z]+(?![0-9])",
		{

			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches.
				"6! ",
				{}
			},
			{ // Minimal match with no negative element at end.
				"j",
				{ {0, 1} }
			},
			{ // No match because of negative element at end.
				"b5",
				{}
			},
			{ // Last element in sequence not captured because of negative match.
				"word6",
				{ {0, 3} }
			},
			{ // No negative element - full match of sequence.
				"#ratata ?",
				{ {1, 7} }
			},
			{ // Multiple matches.
				"bntde!e 65awd1 wdawjk htokth7 b1 nhy!",
				{ {0, 5}, {6, 7}, {10, 12}, {15, 21}, {22, 27}, {33, 36} }
			},
		}
	},

	{  // Negative lookahead for possessive sequence.
		"[a-z]++(?!\\.)", // This regex extracts all words but ignore whole words with '.' at end.
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches.
				"6! ",
				{}
			},
			{ // Minimal possible match.
				"k",
				{ {0, 1} }
			},
			{ // No negative element - have match.
				"yththjt",
				{ {0, 7} }
			},
			{ // Have negative element - skip all match (because of possessive sequence mode).
				"ghjewwa.",
				{}
			},
			{ // Multiple matches.
				"wdawd, htht htp]z e. rhnna. .awdwad yy aa. .. gh_wz.q",
				{ {0, 5}, {7, 11}, {12, 15}, {16, 17}, {29, 35}, {36, 38}, {46, 48}, {52, 53} }
			},
		}
	},

	{ // Negative lookahead in middle of regex.
		"Q(?!jb)[a-z]+",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches.
				"wzq",
				{}
			},
			{ // No match - empty sequence.
				"Q",
				{}
			},
			{ // Minimal possible match.
				"Qb",
				{ {0, 2} }
			},
			{ // No match because of lookahead condition.
				"Qjbge",
				{}
			},
			{ // Have match - lookahead condition is true.
				"Qjzge",
				{ {0, 5} }
			},
			{ // Have match - lookahead sequence is too short.
				"zz Qf",
				{ {3, 5} }
			},
			{ // Several sequential matches.
				"QvwwQhyjyujawQobQcQngg",
				{ {0, 4}, {4, 13}, {13, 16}, {16, 18}, {18, 22} }
			},
		},
	},

	{ // Positive lookahead in start of regex with condition inside.
		"(?=p|TU)[a-zA-Z]+",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches.
				"!5.",
				{}
			},
			{ // false lookahead condition - no match.
				"z",
				{}
			},
			{ // Minimal match with first variant of lookahead condition.
				"p",
				{ {0, 1} }
			},
			{ // Minimal match with second variant of lookahead condition.
				"TU",
				{ {0, 2} }
			},
			{ // Long match with condition.
				"TUrawcbtpawdAz",
				{ {0, 14} }
			},
			{ // Match starts in middle of string because of lookahead condition.
				"wdwapzbv",
				{ {4, 8} }
			},
			{ // Multiple matches.
				"wbvon pasv ehrhp bbTUa TUz, TU avnTUp sp-p TTU",
				{ {6, 10}, {15, 16}, {19, 22}, {23, 26}, {28, 30}, {34, 37}, {39, 40}, {41, 42}, {44, 46} }
			},
		}
	},

	{ // Two lookaheads - positive and negative.
		"c(?=[0-9])(?!00)",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches.
				"76",
				{}
			},
			{ // No match because of positive lookahead condition.
				"c",
				{}
			},
			{ // Simplest possible match.
				"c7",
				{ {0, 1} }
			},
			{ // No match because of negative lookahead condition.
				"c00",
				{}
			},
			{ // Have matches because of true positive and negative conditions.
				"c05c10",
				{ {0, 1}, {3, 4} }
			},
		}
	},

	{ // Simple lookbehind.
		"(?<=Q)w",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No lookbehind symbol at beginning - no match.
				"w",
				{}
			},
			{ // No lookbehind symbol at beginning - no match.
				" w",
				{}
			},
			{ // Simplest possible match.
				"Qw",
				{ {1, 2} }
			},
			{ // No match for second 'w'.
				"aQww",
				{ {2, 3} }
			},
			{ // Two closest matches.
				"QwQw",
				{ {1, 2}, {3, 4} }
			},
		}
	},

	{ // Negative lookbehind with length more, than 1.
		"(?<!10)[a-z]+",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Simplest posible match.
				"t",
				{ {0, 1} }
			},
			{ // Match with multiple symbols.
				"wat",
				{ {0, 3} }
			},
			{ // Match starts with position '3' because of lookbehind condition.
				"10wat",
				{ {3, 5} }
			},
			{ // Have full match because lookbehind condition is not full.
				"0wat",
				{ {1, 4} }
			},
			{ // Have full first match and second match with first symbol skipped.
				"rty10ghnb",
				{ {0, 3}, {6, 9} }
			},
		}
	},

	{ // Negative lookbehind with alternatives.
		"(?<!lol|wat)\\.",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Simplest possible match.
				".",
				{ {0, 1} },
			},
			{ // Three sequential matches.
				"...",
				{ {0, 1}, {1, 2}, {2, 3} },
			},
			{ // No match because of first alternative.
				"lol.",
				{}
			},
			{ // No match because of second alternative.
				"wat.",
				{}
			},
		}
	},

	{ // Positive lookbehind with lookbehind symbol valid as full expression.
		"(?<=a)[a-z]",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No match - can't lookbehind.
				"v",
				{}
			},
			{ // Simplest possible match.
				"ab",
				{ {1, 2} }
			},
			{ // Two sequential matches, where symbol from first match used in lookbehind in second match.
				"aak",
				{ {1, 2}, {2, 3} }
			},
			{ // Three sequential matches.
				"aaaa",
				{ {1, 2}, {2, 3}, {3, 4} }
			},
		}
	},

	{ // String start assertion.
		"^[a-z]+",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Simplest possible match.
				"k",
				{ {0, 1} }
			},
			{ // Match with ling sequence.
				"noeab",
				{ {0, 5} }
			},
			{ // Mo match - sequence is not at start of string.
				" vbg",
				{}
			},
			{ // Have match only for first sequence because it is at start of string.
				"bytanh\nvbf",
				{ {0, 6} }
			},
		}
	},

	{ // String end assertion.
		"nbv$",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Simplest possible match.
				"nbv",
				{ {0, 3} }
			},
			{ // No match - sequence is not at end of string.
				"nbv ",
				{}
			},
			{ // Match only second sequence because first sequence is not at end of string.
				"nbv\nnbv",
				{ {4, 7} }
			},
		}
	},

	{ // String start and string end assertions together.
		"^[0-9]+?$", // sequence iz lazy, but will match many symbols because of string end assertion.
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Simplest possible match.
				"4",
				{ {0, 1} }
			},
			{ // Long match.
				"6524582110",
				{ {0, 10} }
			},
			{ // No match because sequence is not at string start.
				"-5434",
				{}
			},
			{ // No match because sequence is not at string end.
				"1236781q",
				{}
			},
			{ // No match for sequence between two newlines, because start/end assertions works only for string start/end if multiline mode is disabled (by default).
				"wadwd\n43721\ns1!",
				{}
			},
		}
	},

	{ // Useless line start assertion.
		"a^b",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No match because it's not possible to be in start of string when any symbol is already extracted.
				"ab",
				{}
			},
		}
	},

	{ // Optional content before string startassertion - it's not possible to extract it.
		"T?^[0-9]",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Simplest possible match without optional element.
				"2",
				{ {0, 1} }
			},
			{ // No match because there is no string start before "7".
				"T7",
				{}
			},
		}
	},

	{ // Optional content after string end assertion - it's not possible to extract it.
		"5$0?",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Simplest possible match without optional element.
				"5",
				{ {0, 1} }
			},
			{ // No match because there is no string end after "5".
				"50",
				{}
			},
		}
	},

	{ // String end assertion in alternatives.
		"A([a-z]+|[0-9]+$)",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Simplest match with first alternative.
				"Ab",
				{ {0, 2} }
			},
			{ // Match with first alternative not ended at string end.
				"Akyr--!",
				{ {0, 4} }
			},
			{ // Simplest match with second alternativve.
				"A5",
				{ {0, 2} }
			},
			{ // Long match with second alternativve.
				"A368901123",
				{ {0, 10} }
			},
			{ // Nom match because second alternative ends not at string end.
				"A65 ",
				{}
			},
		}
	},

	{ // Simplest backreference.
		"(w)\\1",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches.
				"z1N",
				{}
			},
			{ // No match - not enough symbols.
				"w",
				{}
			},
			{ // Simplest possible match.
				"ww",
				{ {0, 2} }
			},
			{ // Two sequential matches.
				"wwww",
				{ {0, 2}, {2, 4} }
			},
			{ // Several matches.
				"wdaww wwaq Baww ww zwwwwww",
				{ {3, 5}, {6, 8}, {13, 15}, {16, 18}, {20, 22}, {22, 24}, {24, 26} }
			},
		}
	},

	{ // Backreference to OneOf.
		"([a-z])\\1",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches.
				"z1N",
				{}
			},
			{ // Fist and seconds symbols are different - no match.
				"hr",
				{}
			},
			{ // Simplest match.
				"hh",
				{ {0, 2} }
			},
			{ // Two sequential matches with same symbol.
				"mmmm",
				{ {0, 2}, {2, 4} }
			},
			{ // Two sequential matches with different symbols.
				"ssoo",
				{ {0, 2}, {2, 4} }
			},
			{ // Several matches.
				"bbreuu awwzz ggg abcc oons hh rss",
				{ {0, 2}, {4, 6}, {8, 10}, {10, 12}, {13, 15}, {19, 21}, {22, 24}, {27, 29}, {31, 33} }
			},
		}
	},

	{ // Backreference to sequence.
		"([a-f]+)2\\1",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Non-empty string - no matches.
				"Q5/",
				{}
			},
			{ // No match because sequences are different.
				"aab2dac",
				{}
			},
			{ // Simplest match.
				"e2e",
				{ {0, 3} }
			},
			{ // Match with long sequence.
				"aabcaddeadcbb2aabcaddeadcbb",
				{ {0, 27} }
			},
			{ // No match - second sequence is truncated.
				"bac2ba",
				{}
			},
			{ // Have match. Remaining symbols are ignored.
				"accbe2accbeab",
				{ {0, 11} }
			},
			{ // Different parts of same sequence are ignored.
				"bdacea2ceaaccb",
				{ {3, 10} }
			},
			{ // Different parts of same sequence are ignored.
				"aedbeec2cbav1",
				{ {6, 9} }
			},
		}
	},

	{ // Backreference inside sequence.
		"(([a-z])\\2)+",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No match - symbols are different.
				"nt",
				{}
			},
			{ // Simplest possible match.
				"aa",
				{ {0, 2} }
			},
			{ // Match with several same sequence elemens.
				"bbbbbbbb",
				{ {0, 8} }
			},
			{ // Match with several different sequence elemens.
				"ggmmoottrr",
				{ {0, 10} }
			},
			{ // Single non-paired symbol breaks sequence into parts.
				"hhtteechhuuooppzzzwwhhttgggg",
				{ {0, 6}, {7, 17}, {18, 28} }
			},
		}
	},

	{ // Backreference to group with backreference inside.
		"(([0-9])\\2+f)\\1",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No matches - different symbols in inner group.
				"56f56f",
				{}
			},
			{ // Simplest match.
				"33f33f",
				{ {0, 6} }
			},
			{ // Match with long sequence.
				"22222f22222f",
				{ {0, 12} }
			},
			{ // No match - second sequence is longer than first sequence.
				"777f7777f",
				{}
			},
			{ // First sequence is longer than second - it is truncated"
				"99999999f999f",
				{ {5, 13} }
			},
		}
	},

	{ // Backreference inside alternative.
		"(Q)|(([0-7])\\3)",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Match first alternative.
				"Q",
				{ {0, 1} }
			},
			{ // Match second alternative.
				"33",
				{ {0, 2} }
			},
			{ // Several sequential matches.
				"Q226655QQQ22Q336677",
				{ {0, 1}, {1, 3}, {3, 5}, {5, 7}, {7, 8}, {8, 9}, {9, 10}, {10, 12}, {12, 13}, {13, 15}, {15, 17}, {17, 19} }
			},
		}
	},

	{ // Backreference in middle of expression.
		"U([w-z])\\1V",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Simplest match.
				"UyyV",
				{ {0, 4} },
			},
			{ // No match - different symbols.
				"UzwV",
				{}
			},
			{ //multiple matches.
				"UzzV waz!UzxV UxxVUwwV zzzzUzzV",
				{ {0, 4}, {14, 18}, {18, 22}, {27, 31} }
			},
		}
	},

	{ // Look ahead with backreference.
		"([a-z])[a-z]*(?=\\1)",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Simplest match.
				"aa",
				{ {0, 1} }
			},
			{ // No match - all symbols are different.
				"hjtsw",
				{}
			},
			{ // Macth long word ended with start symbol, but non including them.
				"htrwach",
				{ {0, 6} }
			},
			{ // Match starts in middle of string.
				"zhtegawdg",
				{ {4, 8} }
			},
			{ // Match ends in middle of word.
				"bcdtrlebghi",
				{ {0, 7} }
			},
		}
	},

	/*
	{ // Use backreference to previous loop iteration. This works properly for PCRE expressions but not for ECMAScript.
		"((([a-z])|G)\\3?1)+",
		{
			{
				"g1Gg1",
				{ {0, 5} }
			},
		}
	},
	*/

	{ // Non-capturing group groups symbols.
		"(?:zbv)+",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Not enough symbols.
				"zb",
				{}
			},
			{ // Shortest possible match.
				"zbv",
				{ {0, 3} }
			},
			{ // Long match.
				"zbvzbvzbvzbv",
				{ {0, 12} }
			},
			{ // Several matches.
				"hwawzbv zbv zbvefef wzbvzbvzbvf zbvzbv!zbvzbvzbv#",
				{ {4, 7}, {8, 11}, {12, 15}, {21, 30}, {32, 38}, {39, 48} }
			},
		}
	},

	{ // Non-capturing group does not caputure.
		"(?:ta)+([0-9])\\1",  // \1 referes to ([0-9]) group, not to (?:ta).
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Not enough symbols.
				"ta1",
				{}
			},
			{ // Simplest match.
				"ta22",
				{ {0, 4} }
			},
			{ // Match with long sequence.
				"tatatata99",
				{ {0, 10} }
			},
			{ // Two sequential matches.
				"tata44tatata11",
				{ {0, 6}, {6, 14} }
			},
			{ // Several matches.
				"hjtta66-- ta33 rtatata88 tatata55taq",
				{ {3, 7}, {10, 14}, {16, 24}, {25, 33} }
			},
		}
	},

	{ // Atomic group.
		"a(?>bc|b|x)cc",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // There is no way back after alternative "ab" selected.
				"abcc",
				{}
			},
			{ // Ok, alternative "x" selected.
				"axcc",
				{ {0, 4} }
			},
			{ // After variant "abcc" found, there is no way back, so match fails for "abcc", but succeeded for "axcc".
				"abccaxcc",
				{ {4, 8} }
			},
		}
	},

	{ // Atomic group.
		"(?>x*)xa",
		{
			{ // There is no way back after all "x" extracted inside atomic group.
				"xxxa",
				{}
			},
			{
				"xa",
				{}
			},
		}
	},

	{ // Conditional element.
		"(?(?=0)[0-9]+|[a-z]+)Q",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Shortest match with first alternative.
				"0Q",
				{ {0, 2} }
			},
			{ // Long match with first alternative.
				"0542354653Q",
				{ {0, 11} }
			},
			{ // No match with first alternative because of condition.
				"54Q",
				{}
			},
			{ // Shortest match with second alternative.
				"hQ",
				{ {0, 2} }
			},
			{ // Long match with second alternative.
				"rhijtrcvklofQ",
				{ {0, 13} }
			},
		}
	},

	{ // Conditional element with negative condition.
		"(?(?!7)[A-Z]+|[0-9]+)",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Shortest first alternative.
				"H",
				{ {0, 1} }
			},
			{ // Long first alternative.
				"NJOWNAWW",
				{ {0, 8} }
			},
			{ // Control flow goes to first alternative but later match fails. So, match succeeded only statred with position 1.
				"6NH",
				{ {1, 3} }
			},
			{ // Shortest second alternative.
				"7",
				{ {0, 1} }
			},
			{ // Long second alternative.
				"765444224",
				{ {0, 9} }
			},
			{ // Sequence started with first '7' extracted.
				"056373783",
				{ {4, 9} }
			},
		}
	},

	{ // Conditional element with long look condition.
		"(?(?=..y)[a-z]+|[a-xz]+)",  // if third symbol is 'y' - match words, else - match words without 'y'.
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Shortest first alternative.
				"ywy",
				{ {0, 3} }
			},
			{ // Long first alternative.
				"wyywdfwkmhdybva",
				{ {0, 15} }
			},
			{ // Shortest second alternative.
				"c",
				{ {0, 1} }
			},
			{ // No match - if there is no 'y' in third position 'y' will be ignored.
				"y",
				{}
			},
			{ // There is no match starting from start position because there is no 'y' in third position and 'y' in beginning will be ignored.
				"yoc",
				{ {1, 3} }
			},
			{ // Several matches with second alternative - break on 'y' symbol
				"wdvwyvnjywcdbjjywdfyy",
				{ {0, 4}, {5, 8}, {9, 15}, {16, 19} }
			},
			{ // Two sequential matches - for first and for second alternatives.
				"abcdefyzyjhyu",
				{ {0, 6}, {6, 13} }
			},
			{ // Replacing single 'y' in sequence abore produces significantly different result.
				"abcdefyzfjhyu",
				{ {0, 6}, {7, 11}, {12, 13} }
			},
		}
	},

	{ // Non-recursive subroutine call.
		"([a-z]+)2(?1)", // Equivalent to ([a-z]+)2([a-z]+)
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No match - not enough symbols in second sequence.
				"bg2",
				{}
			},
			{ // No match - no '2' symbol.
				"wdad7dbgh",
				{}
			},
			{ // Shortest possible match.
				"h2w",
				{ {0, 3} }
			},
			{ // Match with shortest equal.
				"l2l",
				{ {0, 3} }
			},
			{ // Match with equal parts.
				"bng2bng",
				{ {0, 7} }
			},
			{ // Match with non-equal partw with same length.
				"nbwf2joac",
				{ {0, 9} }
			},
			{ // Match with non-equal partw with different length.
				"vy2nyeq",
				{ {0, 7} }
			},
		}
	},

	{ // Non-recursive subroutine call to group defined later.
		"(?1)@([0-9]{2,4})",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No match - not enough symbols at beginning.
				"5@78543",
				{}
			},
			{ // No match - not enough symbols at end.
				"233566@8",
				{}
			},
			{ // Shortest possible match.
				"73@14",
				{ {0, 5} }
			},
			{ // Too much symbols - sequence will be truncated
				"72456476953@1680890214",
				{ {7, 16} }
			},
			{ // Two sequential sequences.
				"5466@136623@6640",
				{ {0, 9}, {9, 16} }
			},
		}
	},

	{ // Two non-recursive subroutine calls.
		"(?1)\\/([a-f])+\\/(?1)",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Shortest possible match.
				"b/c/a",
				{ {0, 5} }
			},
			{ // Match with long sequence.
				"d/fabcaafcc/b",
				{ {0, 13} }
			},
			{ // Excessive symbols at front and at end are discarded.
				"bacced/e/abddc",
				{ {5, 10} }
			},
			{ // Two sequential sequences.
				"b/acd/ee/f/c",
				{ {0, 7}, {7, 12} }
			},
		}
	},

	{ // Non-recursive call to group, backreference to which used later.  Backreference should point to first group usage result, not to indirect call.
		"([a-z]+)\\/(?1)\\/\\1",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No match - not enoug symbols.
				"jy//jy",
				{}
			},
			{ // No match - not enoug symbols.
				"hly/t/hl",
				{}
			},
			{ // Minimal possible match.
				"m/n/m",
				{ {0, 5} }
			},
			{ // Valid match - third part if a copy of first part.
				"abc/de/abc",
				{ {0, 10} },
			},
			{ // No match - third part is a copy of second part.
				"abc/de/de",
				{}
			},
			{ // Match with all parts same.
				"ytrb/ytrb/ytrb",
				{ {0, 14} }
			},
			{ // Two sequential matches.
				"nbfrt/io/nbfrtz/yuoi/z",
				{ {0, 14}, {14, 22} }
			},
		}
	},

	{ // Two calls to group. Group itself directly not matched because of {0} quantifier.
		"([a-z]+){0}Q(?1)-(?1)",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No start symbol - no match.
				"bbt-aa",
				{}
			},
			{ // Shortest possible match.
				"Qa-n",
				{ {0, 4} }
			},
			{ // Match of same sequences.
				"Qhjt-hjt",
				{ {0, 8} }
			},
			{ // Match of long words.
				"Qwadwad-wdawdbvbr",
				{ {0, 17} }
			},
			{ // Group at begnning not matched.
				"ubQa-ze",
				{ {2, 7} }
			},
			{ // Two sequential matches.
				"Qhhjtt-awQbnty-vf",
				{ {0, 9}, {9, 17} }
			},
		},
	},

	{ // Simple recursive subroutine call - call whole expression.
		"a(?R)?b",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No match - not enough symbols.
				"a",
				{}
			},
			{ // No match - wrong symbol in middle of string.
				"aacbb",
				{}
			},
			{ // Shortest possible match.
				"ab",
				{ {0, 2} }
			},
			{ // Long match for whole string.
				"aaaabbbb",
				{ {0, 8} }
			},
			{ // Symbols at begining are ignored.
				"aaaaabbb",
				{ {2, 8} }
			},
			{ // Symbols at end are ignored.
				"aabbbbb",
				{ {0, 4} }
			},
		}
	},

	{ // Recursive call to group.
		"B(q(?1)?w)E",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No match - no trailing symbol.
				"Bqw",
				{}
			},
			{ // No match - no symbols in group.
				"BE",
				{}
			},
			{ // No match - different number of symbols.
				"BqqwwwE",
				{}
			},
			{ // Shortest possible match.
				"BqwE",
				{ {0, 4} }
			},
			{ // Long match.
				"BqqqqqwwwwwE",
				{ {0, 12} }
			},
		}
	},

	{ // Recursive call for whole expression and backreference.
		"([a-z]+)(?R)?\\1",  // Expressions for polyndromes.
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No match - different symbols.
				"ud",
				{}
			},
			{ // Shortest possible match.
				"hh",
				{ {0, 2} }
			},
			{ // Long match with same symbols.
				"tttttttt",
				{ {0, 8} }
			},
			{ // Long match with different symbols.
				"qwertyytrewq",
				{ {0, 12} }
			},
			{ // Polyndrome of polyndromes.
				"abbaabba",
				{ {0, 8} }
			},
			{ // Non-paired symbols are truncated.
				"zehjuttujhyu",
				{ {2, 10} }
			},
			{ // Two sequential matches.
				"rtggtrjkkj",
				{ {0, 6}, {6, 10} }
			},
			{ // Two non-sequential matches.
				"rtggtrqjkkj",
				{ {0, 6}, {7, 11} }
			},
		}
	},

	{ // Expression for numeric expressions with possible brackets. Contains recursive call and indirect call.
		"(([0-9]+)|(\\((?R)\\)))(((\\+)|(-)|(\\*)|(\\/))(?1))*",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Shortest possible match.
				"7",
				{ {0, 1} }
			},
			{ // Single number with multiple digits.
				"64347",
				{ {0, 5} }
			},
			{ // Expression with two components.
				"76-312",
				{ {0, 6} }
			},
			{ // Expression with multiple components.
				"1+54/3-676*21/2+7874*543*122",
				{ {0, 28} }
			},
			{ // Simple expression in brackets.
				"(56)",
				{ {0, 4} }
			},
			{ // Multiple brackets around expression.
				"(((8644)))",
				{ {0, 10} }
			},
			{ // long expression with brackets.
				"772*(45-56)+95/(13/2)-(77*(22-122))/8",
				{ {0, 37} }
			},
			{ // Unclosed left bracket.
				"(35",
				{ {1, 3} }
			},
			{ // Unclosed right bracket.
				"66+17)",
				{ {0, 5} }
			},
			{ // Leading binary operator.
				"/66-44*256-(54/22)",
				{ {1, 18} }
			},
			{ // Trailing binary operator.
				"5+4+21+7563*",
				{ {0, 11} }
			},
			{ // Two separate matches because of space between operands.
				"55 +771",
				{ {0, 2}, {4, 7} }
			},
			{ // Two separate matches because of duplicated operand.
				"77//(45-4)",
				{ {0, 2}, {4, 10} }
			},
		}
	},

	{ // Indirect call and recursive call.
		"([a-z]+|(\\((?R)\\)))( (?1))*",  // Sequence of words separated by spaces or bracket pairs with same sequence inside.
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Shortes possible match - single word.
				"u",
				{ {0, 1} }
			},
			{ // Single long word.
				"trraht",
				{ {0, 6} }
			},
			{ // Multiple words.
				"rererr rhthjtj eewabnpepq feeimq",
				{ {0, 32} }
			},
			{ // single word inside brackets.
				"(whttgr)",
				{ {0, 8} }
			},
			{ // single word inside nested brackets.
				"(((((jkiopj)))))",
				{ {0, 16} }
			},
			{ // Long expression with nested subparts.
				"twac (trra) dawawpzqqvfk (trr awdw) daz (tgrtr (wdd (ssd ((bhh)) juoklf) wwda) (wwd) wdaad (wwgnnz (wdcvxx)) wdnmnnpmq ((ve)))",
				{ {0, 126} }
			},
			{ // Duplicated space - two matches.
				"yyhhtth  awdwd wdwa",
				{ {0, 7}, {9, 19} }
			},
			{ // Missing bracket - two matches.
				"vewac wdwadwd (httt aaz",
				{ {0, 13}, {15, 23} }
			},
		}
	},

	{ // Recursive call from internal loop. Should preserve counter.
		"([A-Z])((\\*\\((?R)\\)){2,3})?",
		{
			{
				"A*(X)*(Y)",
				{ {0, 9} },
			},
			{
				"A*(X)*(Y)*(Z)*(W)",
				{ {0, 13}, {15, 16} }
			},
			{
				"A*(X)*(Y)",
				{ {0, 9} },
			},
			{
				"A*(X)*(Z*(S)*(T))*(Y)*(W)",
				{ {0, 21}, {23, 24} }
			},
		},
	},

	{ // Indirect recursive call.
		"(\\((?2)?\\))(\\[(?1)?\\])",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Shortest possible match.
				"()[]",
				{ {0, 4} }
			},
			{ // Call second group in first group.
				"([])[]",
				{ {0, 6} }
			},
			{ // Call first group in second group.
				"()[()]",
				{ {0, 6} }
			},
			{ // Deep recursion.
				"([([])])[([([([()])])])]",
				{ {0, 24} }
			},
			{ // Two sequential matches.
				"()[]()[]",
				{ {0, 4}, {4, 8} }
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

	// Non-ASCII code points inside "OneOf"
	{
		"[яИჂΩλ힒𐤈]",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			// Matches for specified symbols.
			{ "я", { {0, 2} } },
			{ "И", { {0, 2} } },
			{ "Ω", { {0, 2} } },
			{ "λ", { {0, 2} } },
			{ "Ⴢ", { {0, 3} } },
			{ "힒", { {0, 3} } },
			{ "𐤈", { {0, 4} } },
			// No matches for symbols nearby.
			{ "ђ", {} },
			{ "ю", {} },
			{ "Ж", {} },
			{ "Ф", {} },
			{ "Ⴣ", {} },
			{ "Ⴠ", {} },
			{ "μ", {} },
			{ "θ", {} },
			{ "Ψ", {} },
			{ "Ϋ", {} },
			{ "힎", {} },
			{ "힘", {} },
			{ "𐤇", {} },
			{ "𐤌", {} },
			// No matches for incomplete UTF-8.
			{ "\xBC", {} },
			{ "\xBC", {} },
			{ "\xF1\x87", {} }
		}
	},

	// Inverted "OneOf" should extract code points, not just chars.
	{
		"A[^B]C",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // No match - contains forbidden symbol.
				"ABC",
				{}
			},
			{ // Simpliest match - non-forbidden ASCII symbol.
				"AqC",
				{ {0, 3} }
			},
			{ // Match with non-forbidden non-ASCII symbol.
				"AфC",
				{ {0, 4} }
			},
			{ // Match with non-forbidden non-ASCII symbol.
				"A휅C",
				{ {0, 5} }
			},
			{ // Match with non-forbidden non-ASCII symbol.
				"A𐤒C",
				{ {0, 6} }
			},
		}
	}
};

const size_t g_matcher_test_data_size= std::size(g_matcher_test_data);

const MatcherTestDataElement g_matcher_multiline_test_data[]
{
	{ // Line start assertion.
		"^[0-9]",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Match at start of string.
				"6",
				{ {0, 1} }
			},
			{ // Match at start of line.
				"  v  \n12 ",
				{ {6, 7} }
			},
			{ // Match both at start of string and at start of line.
				"5\n9",
				{ {0, 1}, {2, 3} }
			},
		}
	},

	{ // Line end assertion.
		"[a-z]$",
		{
			{ // Empty string - no matches.
				"",
				{}
			},
			{ // Match at end of string.
				"  df",
				{ {3, 4} }
			},
			{ // Match at end of line.
				"f\n55",
				{ {0, 1} }
			},
			{ // match both at end of string and at end of line.
				"avuy\nytr",
				{ {3, 4}, {7, 8} }
			},
		}
	},
};

const size_t g_matcher_multiline_test_data_size= std::size(g_matcher_multiline_test_data);


} // namespace RegPanzer
