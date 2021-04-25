#include "MatcherTestData.hpp"
#include "Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include <llvm/Support/Regex.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

class LlvmRegexMatchTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(LlvmRegexMatchTest, TestMatch)
{
	const auto param= GetParam();

	// Ignore unsupported features.
	if((GetRegexFeatures(param.regex_str) & (
			RegexFeatureFlag::UTF8 |
			RegexFeatureFlag::LazySequences |
			RegexFeatureFlag::PossessiveSequences |
			RegexFeatureFlag::Look |
			RegexFeatureFlag::NoncapturingGroups |
			RegexFeatureFlag::AtomicGroups)) != 0)
		return;

	llvm::Regex regex(param.regex_str);

	std::string error_str;
	ASSERT_TRUE(regex.isValid(error_str));

	for(const MatcherTestDataElement::Case& c : param.cases)
	{
		if(StringContainsNonASCIISymbols(c.input_str))
			continue;

		MatcherTestDataElement::Ranges result_ranges;

		llvm::StringRef str= c.input_str;
		while(!str.empty())
		{
			llvm::SmallVector<llvm::StringRef, 16> matches;
			if(!regex.match(str, &matches))
				break;
			if(matches.empty())
				break;

			const llvm::StringRef& match= matches.front();
			result_ranges.emplace_back(size_t(match.data() - c.input_str.data()), size_t(match.data() + match.size() - c.input_str.data()));
			str= str.substr(size_t(match.data() + match.size() - str.data()));
		}

		EXPECT_EQ(result_ranges, c.result_ranges);
	}
}

INSTANTIATE_TEST_CASE_P(M, LlvmRegexMatchTest, testing::ValuesIn(g_matcher_test_data));

} // namespace

} // namespace RegPanzer
