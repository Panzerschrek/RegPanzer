#include "MatcherTestData.hpp"

#include <MatchersGenerated.hpp>

#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

class GeneratedLLVMBinaryMatcherTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

size_t GetIndex(const std::string& regex_str)
{
	for(size_t i= 0; i < std::size(g_matcher_test_data); ++i)
		if( regex_str == g_matcher_test_data[i].regex_str)
			return i;

	assert(false);
	return size_t(~0);
}

TEST_P(GeneratedLLVMBinaryMatcherTest, TestMatch)
{
	const auto param= GetParam();

	const auto function= matcher_functions[GetIndex(param.regex_str)];
	for(const MatcherTestDataElement::Case& c : param.cases)
	{
		MatcherTestDataElement::Ranges result_ranges;
		for(size_t i= 0; i < c.input_str.size();)
		{
			const auto begin= c.input_str.data() + i;
			const auto end= c.input_str.data() + c.input_str.size();

			const auto res= function(begin, end);

			if(res == nullptr)
				++i;
			else
			{
				const size_t end_offset= size_t(res - c.input_str.data());
				result_ranges.emplace_back(i, end_offset);
				i= end_offset;
			}
		}

		EXPECT_EQ(result_ranges, c.result_ranges);
	}
}

INSTANTIATE_TEST_CASE_P(M, GeneratedLLVMBinaryMatcherTest, testing::ValuesIn(g_matcher_test_data));

} // namespace

} // namespace RegPanzer
