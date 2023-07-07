#include "../RegPanzerLib/RegexGraph.hpp"
#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"


namespace RegPanzer
{

namespace
{

struct TestDataElement
{
	std::string regex_str;
	bool is_possessive;
};

const TestDataElement g_test_data[]
{
	{ // Simple sequence with single symobl - should make it possessive.
		"f+",
		true,
	},
	{  // Simple sequence with OneOf - should make it possessive.
		"[0-9]+",
		true,
	},
	{  // Simple sequence with 0 or more elements - should make it possessive.
		"[a-z]*",
		true,
	},
	{ // Sequence with length in range.
		"[0-9]{7,16}",
		true,
	},
	{ // Sequence with string element - should make it possessive.
		"(bla)+",
		true,
	},
	{ // Sequence with symbol after it - should make it possessive, because symbol is not in range of sequence element start symbol.
		"[a-f]+q",
		true,
	},
	{ // Sequence with symbol after it - should NOT make it possessive, because symbol is in range of sequence element start symbol.
		"[a-z]+q",
		false,
	},
	{ // Sequence with OneOf it - should make it possessive, because OneOf set is not intersection with range start symbols set.
		"[0-9]+[a-z]",
		true,
	},
	{ // Sequence with OneOf it - should NOT make it possessive, because sequence symbol is in range of symbols after it.
		"7*[0-9]",
		false,
	},
	{ // Alternatives inside sequence and after it - should make sequence possessive.
		"(lol|wat)+(kek|haha)",
		true,
	},
	{ // Alternatives inside sequence and after it - should NOT make sequence possessive because of possible intersection.
		"(lol|wat)+wat",
		false,
	},
	{ // Non-capturing group in sequence and after it.
		"(?:[sbt])+(?:w)",
		true,
	},
	{ // Non-capturing group in sequence and after it - shouuld not make sequence possessive.
		"(?:[a-z])*(?:c)",
		false,
	},
	{ // Should not apply auto-possessification for sequence with 0 or 1 elements.
		"[a-z]?[a-f]",
		false,
	},
	{ // Should not apply auto-possessification for lazy sequence.
		"[a-z]+?q",
		false,
	},
};

class PossessificationOptimizationTest : public ::testing::TestWithParam<TestDataElement> {};

TEST_P(PossessificationOptimizationTest, TestOptimization)
{
	const auto param= GetParam();
	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	const auto regex_graph= BuildRegexGraph(*regex_chain, Options());

	const auto possessive_sequence= std::get_if<GraphElements::PossessiveSequence>(regex_graph.root.get());
	if(param.is_possessive)
		ASSERT_TRUE(possessive_sequence != nullptr);
	else
		ASSERT_TRUE(possessive_sequence == nullptr);
}

INSTANTIATE_TEST_SUITE_P(DISABLED_PO, PossessificationOptimizationTest, testing::ValuesIn(g_test_data));

} // namespace

} // namespace RegPanzer
