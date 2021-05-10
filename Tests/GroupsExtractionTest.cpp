#include "GroupsExtractionTestData.hpp"
#include "../RegPanzerLib/Matcher.hpp"
#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"


namespace RegPanzer
{

namespace
{

class GroupsExtractionTest : public ::testing::TestWithParam<GroupsExtractionTestDataElement> {};

TEST_P(GroupsExtractionTest, TestGroupsExtraction)
{
	const auto param= GetParam();
	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	const auto regex_graph= BuildRegexGraph(*regex_chain);

	for(const GroupsExtractionTestDataElement::Case& c : param.cases)
	{
		std::vector<GroupsExtractionTestDataElement::GroupMatchResults> results;

		for(size_t start_pos= 0; start_pos < c.input_str.size();)
		{
			std::string_view subgroups[10];
			const size_t groups_extracted= Match(regex_graph, c.input_str, start_pos, subgroups, std::size(subgroups));
			if(groups_extracted == 0)
				break;

			GroupsExtractionTestDataElement::GroupMatchResults result;
			for(size_t i= 0; i < std::min(groups_extracted, std::size(subgroups)); ++i)
			{
				const std::string_view res= subgroups[i];
				const size_t start_offset= size_t(res.data() - c.input_str.data());
				const size_t end_offset= start_offset + res.size();
				result.emplace_back(start_offset, end_offset);
				start_pos= end_offset;
			}

			results.push_back(std::move(result));
		}

		EXPECT_EQ(results, c.results);
	}
}

INSTANTIATE_TEST_CASE_P(GE, GroupsExtractionTest, testing::ValuesIn(g_groups_extraction_test_data, g_groups_extraction_test_data + g_groups_extraction_test_data_size));

} // namespace

} // namespace RegPanzer
