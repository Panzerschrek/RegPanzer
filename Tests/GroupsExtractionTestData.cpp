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
		},
	},
};

const size_t g_groups_extraction_test_data_size= std::size(g_groups_extraction_test_data);

} // namespace RegPanzer
