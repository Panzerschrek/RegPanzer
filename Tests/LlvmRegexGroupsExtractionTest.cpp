#include "GroupsExtractionTestData.hpp"
#include "Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include <llvm/Support/Regex.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

class LlvmRegexGroupsExtractionTest : public ::testing::TestWithParam<GroupsExtractionTestDataElement> {};

TEST_P(LlvmRegexGroupsExtractionTest, TestGroupsExtraction)
{
	const auto param= GetParam();

	// Ignore unsupported features.
	if((GetRegexFeatures(param.regex_str) & (
			RegexFeatureFlag::UTF8 |
			RegexFeatureFlag::LazySequences |
			RegexFeatureFlag::PossessiveSequences |
			RegexFeatureFlag::Look |
			RegexFeatureFlag::NoncapturingGroups |
			RegexFeatureFlag::AtomicGroups |
			RegexFeatureFlag::Subroutines |
			RegexFeatureFlag::SymbolClasses)) != 0)
		return;

	llvm::Regex regex(param.regex_str);

	std::string error_str;
	ASSERT_TRUE(regex.isValid(error_str));

	for(const GroupsExtractionTestDataElement::Case& c : param.cases)
	{
		if(StringContainsNonASCIISymbols(c.input_str))
			continue;

		std::vector<GroupsExtractionTestDataElement::GroupMatchResults> results;

		llvm::StringRef str= c.input_str;
		while(!str.empty())
		{
			llvm::SmallVector<llvm::StringRef, 16> matches;
			if(!regex.match(str, &matches) || matches.empty())
				break;

			GroupsExtractionTestDataElement::GroupMatchResults result;
			for(const auto& match : matches)
				result.emplace_back(size_t(match.data() - c.input_str.data()), size_t(match.data() + match.size() - c.input_str.data()));

			results.push_back(std::move(result));

			const auto& root_match= matches[0];
			str= str.substr(size_t(root_match.data() + root_match.size() - str.data()));
		}

		EXPECT_EQ(results, c.results);
	}
}

INSTANTIATE_TEST_CASE_P(GE, LlvmRegexGroupsExtractionTest, testing::ValuesIn(g_groups_extraction_test_data, g_groups_extraction_test_data + g_groups_extraction_test_data_size));

} // namespace

} // namespace RegPanzer
