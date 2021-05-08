#include "MatcherTestData.hpp"
#include "Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <pcre.h>

namespace RegPanzer
{

namespace
{

class PcreRegexMatchTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(PcreRegexMatchTest, TestMatch)
{
	const auto param= GetParam();

	if((GetRegexFeatures(param.regex_str) & RegexFeatureFlag::FourDigitHexCodes) != 0)
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
					++i;
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

INSTANTIATE_TEST_CASE_P(M, PcreRegexMatchTest, testing::ValuesIn(g_matcher_test_data));

} // namespace

} // namespace RegPanzer
