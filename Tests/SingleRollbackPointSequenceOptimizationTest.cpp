#include "../RegPanzerLib/RegexGraph.hpp"
#include "../RegPanzerLib/RegexGraphOptimizer.hpp"
#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <optional>

namespace RegPanzer
{

namespace
{

struct TestDataElement
{
	std::string regex_str;
	bool is_optimized;
};

const TestDataElement g_test_data[]
{
	{ // Not a sequence.
		"abcQ",
		false,
	},
	{ // Not a sequence.
		"[0-9]",
		false,
	},
	{ // Possessification optimization is used here.
		"a*",
		false,
	},
	{ // Possessification optimization is used here.
		"[0-9]*q",
		false,
	},
	{ // Single rollback point optimization is used here, because possessification can't be used.
		"[0-9]*3",
		true,
	},
	{ // Single rollback point optimization is not used here, since element after the sequence is too long.
		"[0-9]*3long_string",
		false,
	},
	{ // Single rollback point optimization is used here, since element after the sequence is no longer than sequence element itself.
		"[abcde]*aq",
		true,
	},
	{ // Single rollback point optimization is not used here, since element after the sequence is too complex.
		"[0-9]*(3|4)",
		false,
	},
	{ // Single rollback point optimization is not used here, since sequence element is too complex.
		"([0-9]q)*3",
		false,
	},
};

class SingleRollbackPointOptimizationTest : public ::testing::TestWithParam<TestDataElement> {};

TEST_P(SingleRollbackPointOptimizationTest, TestOptimization)
{
	const auto param= GetParam();
	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	const auto regex_graph= OptimizeRegexGraph( BuildRegexGraph(*regex_chain, Options()) );

	const auto single_rollback_point_sequence= std::get_if<GraphElements::SingleRollbackPointSequence>(regex_graph.root);
	if(param.is_optimized)
		ASSERT_TRUE(single_rollback_point_sequence != nullptr);
	else
		ASSERT_TRUE(single_rollback_point_sequence == nullptr);
}

INSTANTIATE_TEST_SUITE_P(SRPSO, SingleRollbackPointOptimizationTest, testing::ValuesIn(g_test_data));

} // namespace

} // namespace RegPanzer
