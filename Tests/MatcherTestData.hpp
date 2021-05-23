#pragma once
#include <string>
#include <vector>

namespace RegPanzer
{

struct MatcherTestDataElement
{
	std::string regex_str;

	using Range= std::pair<size_t, size_t>; // begin/end
	using Ranges= std::vector<Range>;
	struct Case
	{
		std::string input_str;
		Ranges result_ranges;
	};

	std::vector<Case> cases;
};

extern const MatcherTestDataElement g_matcher_test_data[];
extern const size_t g_matcher_test_data_size;

extern const MatcherTestDataElement g_matcher_multiline_test_data[];
extern const size_t g_matcher_multiline_test_data_size;

} // namespace RegPanzer
