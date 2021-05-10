#pragma once
#include <string>
#include <vector>

namespace RegPanzer
{

struct GroupsExtractionTestDataElement
{
	using GroupMatchResult= std::pair<size_t, size_t>; // Begin/end offsets
	using GroupMatchResults= std::vector<GroupMatchResult>;
	struct Case
	{
		std::string input_str;
		std::vector<GroupMatchResults> results;
	};

	std::string regex_str;
	std::vector<Case> cases;
};

extern const GroupsExtractionTestDataElement g_groups_extraction_test_data[];
extern const size_t g_groups_extraction_test_data_size;

} // namespace RegPanzer
