#include "GroupsExtractionTestData.hpp"
#include "Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <regex>

namespace RegPanzer
{

namespace
{

class StdRegexGroupsExtractionTest : public ::testing::TestWithParam<GroupsExtractionTestDataElement> {};

TEST_P(StdRegexGroupsExtractionTest, TestGroupsExtraction)
{
	const auto param= GetParam();

	// Ignore unsupported features.
	if((GetRegexFeatures(param.regex_str) & (
			RegexFeatureFlag::UTF8 |
			RegexFeatureFlag::PossessiveSequences |
			RegexFeatureFlag::AtomicGroups |
			RegexFeatureFlag::ConditionalElements |
			RegexFeatureFlag::Subroutines |
			RegexFeatureFlag::FourDigitHexCodes  |
			RegexFeatureFlag::LookBehind)) != 0)
		return;

	try
	{
		std::regex regex(param.regex_str, std::regex_constants::ECMAScript);

		for(const GroupsExtractionTestDataElement::Case& c : param.cases)
		{
			if(StringContainsNonASCIISymbols(c.input_str))
				continue;

			std::vector<GroupsExtractionTestDataElement::GroupMatchResults> results;

			for(auto it= std::sregex_iterator(c.input_str.begin(), c.input_str.end(), regex); it != std::sregex_iterator(); ++it)
			{
				GroupsExtractionTestDataElement::GroupMatchResults result;

				for(const auto& pair : *it)
				{
					const auto b= size_t(pair.first - c.input_str.begin());
					const auto e= size_t(pair.second - c.input_str.begin());
					result.emplace_back(b, e);
				}

				results.push_back(std::move(result));
			}

			EXPECT_EQ(results, c.results);
		}
	}
	catch(const std::regex_error& e)
	{
		std::cout << "std::regex error: " << e.what() << std::endl;
		ASSERT_TRUE(false);
	}
}

INSTANTIATE_TEST_CASE_P(GE, StdRegexGroupsExtractionTest, testing::ValuesIn(g_groups_extraction_test_data, g_groups_extraction_test_data + g_groups_extraction_test_data_size));

} // namespace

} // namespace RegPanzer
