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
	std::optional<size_t> length; // None if optimization is not used.
};

const TestDataElement g_test_data[]
{
	{ // Possessification is used here instead of FLES optimization.
		"f*",
		std::nullopt,
	},
	{ // Possessification is used here instead of FLES optimization.
		"[0-9]*",
		std::nullopt,
	},
	{ // Possessification is used here instead of FLES optimization.
		"[a-f]*Q",
		std::nullopt,
	},
	{ // FLES optimization is not used here because sequence is possessive.
		"[A-Z]*+Q",
		std::nullopt,
	},
	{ // FLES optimization is not used here because sequence is lazy.
		"[A-Z]*?Q",
		std::nullopt,
	},
	{ // FLES optimization is used here because possessification optimization can't be used because of same symbols in sequence and after it.
		"[a-z]*q",
		1,
	},
	{ // FLES optimization for sequence with element size greater than 1.
		"(?:vRe){3,16}v",
		/* 3 */ std::nullopt, // TODO - fix it.
	},
	{ // FLES optimization for sequence with fixed length sequence inside.
		"(?:[0-9]{3}c)+0",
		/* 4 */ std::nullopt, // TODO - fix it.
	},
	{ // FLES optimization is not used because of backreference.
		"([a-z]*)(?:[0-9]\\1)+4",
		std::nullopt,
	},
	{ // FLES for sequence element with alternatives inside.
		"(?:lol|wat|kek)*[a-z]",
		/* 3 */ std::nullopt, // TODO - fix it.
	},
	{ // FLES optimization is not used because of different length of alternatives.
		"(?:lol|ya|kek)*[a-z]",
		std::nullopt,
	},
};

class FixedLengthElementSequenceOptimizationTest : public ::testing::TestWithParam<TestDataElement> {};

TEST_P(FixedLengthElementSequenceOptimizationTest, TestOptimization)
{
	const auto param= GetParam();
	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	const auto regex_graph= OptimizeRegexGraph( BuildRegexGraph(*regex_chain, Options()) );

	const auto seq= std::get_if<GraphElements::FixedLengthElementSequence>(regex_graph.root);
	if(param.length != std::nullopt)
		ASSERT_TRUE(seq != nullptr && seq->element_length == *param.length);
	else
		ASSERT_TRUE(seq == nullptr);
}

INSTANTIATE_TEST_SUITE_P(FLES, FixedLengthElementSequenceOptimizationTest, testing::ValuesIn(g_test_data));

} // namespace

} // namespace RegPanzer
