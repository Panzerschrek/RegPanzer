#include "GroupsExtractionTestData.hpp"


namespace RegPanzer
{

const GroupsExtractionTestDataElement g_groups_extraction_test_data[]
{
	// Match simple fixed sequence with no groups.
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
				{ { {0, 3} }, },
			},
			{ // Signle match in middle of string.
				"lolQabcwat",
				{ { {4, 7} }, },
			},
			{ // Signle match at end of string.
				"012345abc",
				{ { {6, 9} }, },
			},
			{ // Multiple ranges in string.
				"abcSSabcQ",
				{ {{ 0, 3 }}, {{ 5, 8 }}, },
			},
		}
	},

	{ // Extract simpliest group for whole expression.
		"(g)",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simpliest match.
				"g",
				{ { {0, 1}, {0, 1}, } }
			},
			{ // Two sequential matches.
				"gg",
				{ { {0, 1}, {0, 1}, }, { {1, 2}, {1, 2}, }, }
			},
			{ // Match in middle of string.
				"bjgda",
				{ { {2, 3}, {2, 3}, } }
			},
		}
	},

	{ // Extract variable-length group from middle of expression.
		"n([0-9]+)r",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest possible match.
				"n7r",
				{ { {0, 3}, {1, 2}, } }
			},
			{ // Match in middle of string with long sequense.
				" 6tn1732807r!n",
				{ { {3, 12}, {4, 11}, } }
			},
			{ // Three sequential matches.
				"n51rn9rn1234567890555223r",
				{ { {0, 4}, {1, 3}, }, { {4, 7}, {5, 6}, }, { {7, 25}, {8, 24}, }, }
			},
		}
	},

	{ // Extract two sequential groups.
		"([a-z]+)([0-9]+)",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest possible match.
				"z3",
				{ { {0, 2}, {0, 1}, {1, 2} } }
			},
			{ // Match for long sequences.
				"hjahntt43672",
				{ { {0, 12}, {0, 7}, {7, 12} } }
			},
			{ // Two sequential matches.
				"abc54nmytr661",
				{ { {0, 5}, {0, 3}, {3, 5} }, { {5, 13}, {5, 10}, {10, 13} } }
			},
			{ // Two matches in long string.
				"!$afr4 += hjy45;",
				{ { {2, 6}, {2, 5}, {5, 6} }, { {10, 15}, {10, 13}, {13, 15} } }
			},
		}
	},

	{ // Extract two groups - one is inside another.
		"\\((Q([A-Z]+)W)\\)",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // No math - missing required symbol in sequence.
				"(QW)",
				{},
			},
			{ // Simplest possible match.
				"(QBW)",
				{ { {0, 5}, {1, 4}, {2, 3} } },
			},
			{ // Match with long sequence.
				"(QGIONDMOMW)",
				{ { {0, 12}, {1, 11}, {2, 10} } },
			},
			{ // Three sequential matches.
				"(QMW)(QHNW)(QQWW)",
				{ { {0, 5}, {1, 4}, {2, 3} }, { {5, 11}, {6, 10}, {7, 9} }, { {11, 17}, {12, 16}, {13, 15} } },
			},
		}
	},

	{ // Several groups - sequential and nested.
		"<(A([0-9])(([a-z]+)=(\\4))Z);([a-f]+)>",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest possible match.
				"<A6s=sZ;b>",
				{ { {0, 10}, {1, 7}, {2, 3}, {3, 6}, {3, 4}, {5, 6}, {8, 9} } }
			},
			{ // Long match.
				"<A1hjtwq=hjtwqZ;bcbbadf>",
				{ { {0, 24}, {1, 15}, {2, 3}, {3, 14}, {3, 8}, {9, 14}, {16, 23} } }
			},
		},
	},

	{ // Optional group.
		"<(Q[A-Z]+W)?>",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simpliest match with no optional group.
				"<>",
				{ { {0, 2}, {2, 2} } }
			},
			{ // Match with optional group.
				"<QSFWDW>",
				{ { {0, 8}, {1, 7} } }
			},
			{ // Match with no optional group in middle of string.
				"  R<> vb!",
				{ { {3, 5}, {9, 9} } }
			},
			{ // Match with  optional group in middle of string.
				"__<QBW>H ",
				{ { {2, 7}, {3, 6} } }
			},
		}
	},

	{ // Non-optional group after optional group.
		"\\((QW)?\\)([a-z]+)",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simpliest match with no optional group.
				"()a",
				{ { {0, 3}, {3, 3}, {2, 3} } }
			},
			{ // Match with optional group.
				"(QW)bgrt",
				{ { {0, 8}, {1, 3}, {4, 8} } }
			},
			{ // Match with no optional group in middle of string.
				"--()bgr---",
				{ { {2, 7}, {10, 10}, {4, 7} } }
			},
			{ // Match with  optional group in middle of string.
				"eee(QW)sZZZ-K",
				{ { {3, 8}, {4, 6}, {7, 8} } }
			},
		}
	},

	{ // Optional group inside non-optional group.
		"([a-z]+([0-9]*f)?)N",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest match with no optional group.
				"aN",
				{ { {0, 2}, {0, 1}, {2, 2} } }
			},
			{ // Match with optional group.
				"abr53fN",
				{ { {0, 7}, {0, 6}, {3, 6} } }
			},
			{ // Match with no optional group in middle of string.
				"5664sNytaw2",
				{ { {4, 6}, {4, 5}, {11, 11} } }
			},
			{ // Match with optional group in middle of string.
				"__ g67fN + 99",
				{ { {3, 8}, {3, 7}, {4, 7} } }
			},
		}
	},

	{ // Non-optional group inside optional group.
		"[0-9]+(\\.[0-9]+e([0-9]+))?",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest match with no optional group.
				"3",
				{ { {0, 1}, {1, 1}, {1, 1} } }
			},
			{ // Match with optional group.
				"954.5e67",
				{ { {0, 8}, {3, 8}, {6, 8} } }
			},
			{ // Match with no optional group in middle of string.
				"awd 6421 fz+",
				{ { {4, 8}, {12, 12}, {12, 12} } }
			},
			{ // Two matches - without and with optional group.
				"expr= 4213 + 44.78e3;",
				{ { {6, 10}, {21, 21}, {21, 21} }, { {13, 20}, {15, 20}, {19, 20} } }
			},
		}
	},
};

const size_t g_groups_extraction_test_data_size= std::size(g_groups_extraction_test_data);

} // namespace RegPanzer
