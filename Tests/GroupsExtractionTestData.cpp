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
};

const size_t g_groups_extraction_test_data_size= std::size(g_groups_extraction_test_data);

} // namespace RegPanzer
