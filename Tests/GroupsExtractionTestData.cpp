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

	{ // Extract group inside sequence.
		"([a-z][0-9])+",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest possible match - one iteration.
				"h8",
				{ { {0, 2}, {0, 2} } }
			},
			{ // Several iterations. Extracted only last group state.
				"n7l2s7k4f6g6k8h1d0",
				{ { {0, 18}, {16, 18} } }
			},
			{ // Match in middle of string.
				"dwadawd5d2a664234",
				{ { {6, 12}, {10, 12} } }
			},
			{ // Several matches.
				"a5 h7k9f4Qf4a4j5-g53 + a4a4k7c5 = fff666",
				{
					{ { 0,  2}, { 0,  2} },
					{ { 3,  9}, { 7,  9} },
					{ {10, 16}, {14, 16} },
					{ {17, 19}, {17, 19} },
					{ {23, 31}, {29, 31} },
					{ {36, 38}, {36, 38} },
				}
			},
		}
	},

	{ // Extract two groups inside sequence.
		"(([A-Z])2([a-z]))+",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest possible match - one iteration.
				"B2h",
				{ { {0, 3}, {0, 3}, {0, 1}, {2, 3} } }
			},
			{ // Several iterations.
				"V2kP2gZ2nO2q",
				{ { {0, 12}, {9, 12}, {9, 10}, {11, 12} } }
			},
			{ // Several matches.
				" B2rP2b = Q2q - G2hhB2kR2l S2nB2d",
				{
					{ { 1,  7}, { 4,  7}, { 4,  5}, { 6,  7} },
					{ {10, 13}, {10, 13}, {10, 11}, {12, 13} },
					{ {16, 19}, {16, 19}, {16, 17}, {18, 19} },
					{ {20, 26}, {23, 26}, {23, 24}, {25, 26} },
					{ {27, 33}, {30, 33}, {30, 31}, {32, 33} },
				}
			},
		}
	},

	{ // Extract optional group inside sequence.
		"([a-z]([0-9])?)+",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest match with single iteration and no optional group.
				"b",
				{ { {0, 1}, {0, 1}, {1, 1} } }
			},
			{ // Match with multiple iterations and with no optional group.
				"gfjajvaq",
				{ { {0, 8}, {7, 8}, {8, 8} } }
			},
			{ // Single iteration with optional group.
				"z7",
				{ { {0, 2}, {0, 2}, {1, 2} } }
			},
			{ // Multiple iterations with optional group.
				"a4n7l1g6",
				{ { {0, 8}, {6, 8}, {7, 8} } }
			},
			{ // Multiple iteration but optional group appears only in one non-last iteration.
				"fcaw6haawbh",
				{ { {0, 11}, {10, 11}, {4, 5} } } // llvm::regex failed with it.
			},
			{ // Multiple iteration but optional group appears only in one last iteration.
				"bnf7",
				{ { {0, 4}, {2, 4}, {3, 4} } }
			},
			{ // Multiple iterations with multiple optional groups. Only last state of optional group returned.
				"c3bhtg4ad2vf5ha",
				{ { {0, 15}, {14, 15}, {12, 13} } } // llvm::regex failed with it.
			},
			{ // Several matches.
				"v wgr g6a3v --e3f5 =n6 + ggg3aa ;/ wtgr3 L b4a4h6j1",
				{
					{ { 0,  1}, { 0,  1}, {51, 51} },
					{ { 2,  5}, { 4,  5}, {51, 51} },
					{ { 6, 11}, {10, 11}, { 9, 10} },
					{ {14, 18}, {16, 18}, {17, 18} },
					{ {20, 22}, {20, 22}, {21, 22} },
					{ {25, 31}, {30, 31}, {28, 29} },
					{ {35, 40}, {38, 40}, {39, 40} },
					{ {43, 51}, {49, 51}, {50, 51} },
				}
			},
		}
	},

	{ // Alternative groups.
		"([a-z]+)|([0-9]+)",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest possible match for first alternative.
				"l",
				{ { {0, 1}, {0, 1}, {1, 1} } }
			},
			{ // Simplest possible match for second alternative.
				"6",
				{ { {0, 1}, {1, 1}, {0, 1} } }
			},
			{ // Long match for first alternative.
				"fhjabmo",
				{ { {0, 7}, {0, 7}, {7, 7} } }
			},
			{ // Long match for second alternative.
				"83361",
				{ { {0, 5}, {5, 5}, {0, 5} } }
			},
			{ // Several sequential matches.
				"623baw5kt7542g3236htsdf",
				{
					{ { 0,  3}, {23, 23}, { 0,  3} },
					{ { 3,  6}, { 3,  6}, {23, 23} },
					{ { 6,  7}, {23, 23}, { 6,  7} },
					{ { 7,  9}, { 7,  9}, {23, 23} },
					{ { 9, 13}, {23, 23}, { 9, 13} },
					{ {13, 14}, {13, 14}, {23, 23} },
					{ {14, 18}, {23, 23}, {14, 18} },
					{ {18, 23}, {18, 23}, {23, 23} },
				}
			},
		}
	},

	{ // Alternative groups inside sequence.
		"(([A-Z])|([a-z])|([0-9]))+",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Single element with first alternative.
				"J",
				{ { {0, 1}, {0, 1}, {0, 1}, {1, 1}, {1, 1} } }
			},
			{ // Single element with second alternative.
				"c",
				{ { {0, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1} } }
			},
			{ // Single element with third alternative.
				"8",
				{ { {0, 1}, {0, 1}, {1, 1}, {1, 1}, {0, 1} } }
			},
			{ // Sequence with only first alternatives.
				"KRAOR",
				{ { {0, 5}, {4, 5}, {4, 5}, {5, 5}, {5, 5} } }
			},
			{ // Sequence with only second alternatives.
				"yls",
				{ { {0, 3}, {2, 3}, {3, 3}, {2, 3}, {3, 3} } }
			},
			{ // Sequence with only third alternatives.
				"754300",
				{ { {0, 6}, {5, 6}, {6, 6}, {6, 6}, {5, 6} } }
			},
			{ // Sequence with altering alternatives.
				"Yl4Nq0",
				{ { {0, 6}, {5, 6}, {3, 4}, {4, 5}, {5, 6} } } // llvm::regex failed with it.
			},
			{ // Sequence with all first alternatives except one third alternative. Last third alternative should be captured.
				"NGH0GAWA",
				{ { {0, 8}, {7, 8}, {7, 8}, {8, 8}, {3, 4} } }
			},
			{ // Start with only second alternatives finish with only third alternatives.
				"bhtsrase6785650645",
				{ { {0, 18}, {17, 18}, {18, 18}, {7, 8}, {17, 18} } }
			},
			{ // Match in middle of string.
				" -- AB56cf !! ",
				{ { {4, 10}, {9, 10}, {5, 6}, {9, 10}, {7, 8} } }
			},
		}
	},

	{ // Extract group in subroutine call. Should extract only group itself, not group in call.
		"([a-z]+)2(?1)",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest possible match.
				"b2j",
				{ { {0, 3}, {0, 1} } }
			},
			{ // Match with long sequences.
				"nhrra2jjmkura",
				{ { {0, 13}, {0, 5} } }
			},
			{ // Match in middle of string.
				"wd -- nh2hjt !!",
				{ { {6, 12}, {6, 8} } }
			},
		}
	},

	{ // Extract group in subroutine call. Should extract only group itself, not group in call.
		"(?1)-([0-9]+)",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest possible match.
				"6-1",
				{ { {0, 3}, {2, 3} } }
			},
			{ // Match with long sequences.
				"5211336-05918338622",
				{ { {0, 19}, {8, 19} } }
			},
			{ // Several matches.
				"b 44-3 + 0-65432 * 431-655f f4-5a",
				{
					{ { 2,  6}, { 5,  6} },
					{ { 9, 16}, {11, 16} },
					{ {19, 26}, {23, 26} },
					{ {29, 32}, {31, 32} },
				}
			},
		}
	},

	{ // Extract internal group in subroutine call. Should extract only group itself, not group in call.
		"([a-z]+([0-9]+))=(?1)",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest possible match.
				"a5=g1",
				{ { {0, 5}, {0, 2}, {1, 2} } }
			},
			{ // Match with long sequences.
				"dbt32=nhyeag3125765111",
				{ { {0, 22}, {0, 5}, {3, 5} } }
			},
			{ // Several sequential matches.
				"gr1235=b5acf4=hda3123jhs446=bkiacbtt421654",
				{
					{ { 0,  9}, { 0,  6}, { 2, 6 } },
					{ { 9, 21}, { 9, 13}, {12, 13} },
					{ {21, 42}, {21, 27}, {24, 27} },
				}
			},
		}
	},

	{ // Extract groups in recursive call.
		"([a-z]+)(?R)?\\1",
		{
			{ // Empty string - no matches.
				"",
				{},
			},
			{ // Simplest possible match.
				"bb",
				{ { {0, 2}, {0, 1} } }
			},
			{ // Long match (wit recursive call). Should extract only first (top-level) group value.
				"bnttnb",
				{ { {0, 6}, {0, 1} } }
			},
			{ // Two sequential matches.
				"leeffeelghtthg",
				{ { {0, 8}, {0, 1} }, { {8, 14}, {8, 9} } }
			},
		}
	},
};

const size_t g_groups_extraction_test_data_size= std::size(g_groups_extraction_test_data);

} // namespace RegPanzer
