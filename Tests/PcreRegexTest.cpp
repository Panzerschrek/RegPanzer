#include "MatcherTestData.hpp"
#include "GroupsExtractionTestData.hpp"
#include "Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <pcre.h>

namespace RegPanzer
{

namespace
{

bool IsUnsupportedRegex(const std::string& regex_str)
{
	return (GetRegexFeatures(regex_str) & RegexFeatureFlag::FourDigitHexCodes) != 0;
}

class PcreRegexMatchTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(PcreRegexMatchTest, TestMatch)
{
	const auto param= GetParam();

	if(IsUnsupportedRegex(param.regex_str))
		return;

	const char* error_ptr= nullptr;
	int error_offset= 0;
	pcre* const r= pcre_compile(param.regex_str.data(), PCRE_UTF8, &error_ptr, &error_offset, nullptr);
	ASSERT_TRUE(r != nullptr);

	try
	{
		int vec[100*3]{};
		for(const MatcherTestDataElement::Case& c : param.cases)
		{
			MatcherTestDataElement::Ranges result_ranges;
			for(size_t i= 0; i < c.input_str.size();)
			{
				if(pcre_exec(r, nullptr, c.input_str.data(), int(c.input_str.size()), int(i), PCRE_NO_UTF8_CHECK, vec, std::size(vec)) != 0)
				{
					if(vec[0] < 0 || vec[1] < 0 || size_t(vec[1]) <= i || size_t(vec[1]) > c.input_str.size())
						break;

					result_ranges.emplace_back(size_t(vec[0]), size_t(vec[1]));
					i= size_t(vec[1]);
				}
				else
					break;
			}

			EXPECT_EQ(result_ranges, c.result_ranges);
		}
	}
	catch(...)
	{
		pcre_free(r);
		throw;
	}

	pcre_free(r);
}

INSTANTIATE_TEST_CASE_P(M, PcreRegexMatchTest, testing::ValuesIn(g_matcher_test_data, g_matcher_test_data + g_matcher_test_data_size));


class PcreRegexGroupsExtractionTest : public ::testing::TestWithParam<GroupsExtractionTestDataElement> {};

TEST_P(PcreRegexGroupsExtractionTest, TestGroupsExtraction)
{
	const auto param= GetParam();

	if(IsUnsupportedRegex(param.regex_str))
		return;

	const char* error_ptr= nullptr;
	int error_offset= 0;
	pcre* const r= pcre_compile(param.regex_str.data(), PCRE_UTF8, &error_ptr, &error_offset, nullptr);
	ASSERT_TRUE(r != nullptr);

	int groups_count= 0;
	pcre_fullinfo(r, nullptr, PCRE_INFO_CAPTURECOUNT, &groups_count);
	++groups_count;

	try
	{
		int vec[100*3]{};
		for(const GroupsExtractionTestDataElement::Case& c : param.cases)
		{
			std::vector<GroupsExtractionTestDataElement::GroupMatchResults> results;
			for(size_t i= 0; i < c.input_str.size();)
			{
				const int n= pcre_exec(r, nullptr, c.input_str.data(), int(c.input_str.size()), int(i), PCRE_NO_UTF8_CHECK, vec, std::size(vec));
				if(n <= 0)
					break;

				GroupsExtractionTestDataElement::GroupMatchResults result;
				for(int j= 0; j < n; ++j)
				{
					const int begin= vec[2 * j + 0];
					const int end  = vec[2 * j + 1];

					if(begin == -1 && end == -1)
						result.emplace_back(c.input_str.size(), c.input_str.size());
					else if(begin < 0 || end < 0 || size_t(end) <= i || size_t(end) > c.input_str.size())
						break;
					else
						result.emplace_back(size_t(begin), size_t(end));
				}
				for(int j= n; j < groups_count; ++j)
					result.emplace_back(c.input_str.size(), c.input_str.size());

				i= size_t(vec[1]);
				if(!result.empty())
					results.push_back(std::move(result));
			}

			EXPECT_EQ(results, c.results);
		}
	}
	catch(...)
	{
		pcre_free(r);
		throw;
	}

	pcre_free(r);
}

INSTANTIATE_TEST_CASE_P(GE, PcreRegexGroupsExtractionTest, testing::ValuesIn(g_groups_extraction_test_data, g_groups_extraction_test_data + g_groups_extraction_test_data_size));

} // namespace

} // namespace RegPanzer
