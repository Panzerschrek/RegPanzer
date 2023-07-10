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
	bool is_possessive;
};

const TestDataElement g_test_data[]
{
	{ // Simple optimization - should create possessive alternative.
		"a|b",
		true,
	},
	{ // Contents of regexp after alternatives is not affected.
		"(c|d)[0-9]+",
		true,
	},
	{ // () does not affect the optimization.
		"(1)|(0)",
		true,
	},
	{ // Long strings.
		"foo|querty",
		true,
	},
	{ // Optimization fails here, since start symbols are the same. TODO - check not only start symbols.
		"bar|bazz",
		false,
	},
	{ // Optimization works for case if second element is also alternative.
		"(foo)|(bar|baz)",
		true,
	},
	{ // Optimization for alternatives of SomeOf.
		"[0-9]|[a-z]",
		true,
	},
	{ // Optimization for alternatives of SomeOf - fails.
		"[0-9]|[a-z1]",
		false,
	},
	{ // Optimization for alternatives of SomeOf and string.
		"[a-z]|66",
		true,
	},
	{ // Optimization for alternatives of specific symbol and OneOf.
		"Q|[0-9a-z]",
		true,
	},
	{ // Optimization for alternatives of specific symbol and OneOf - fails.
		"q|[0-9a-z]",
		false,
	},
	{ // Optimization fails FOR NOW, since only two branches are supported. TODO - fix this.
		"a|b|c",
		false,
	},
	{ // Optimization fails FOR NOW, because first alternative isn't simple enought. TODO - fix this.
		"(a|b)|c",
		false,
	},
	{ // Optimization fails since first alternative is too complex (starts with sequence). TODO - fix this.
		"([a-z]*)|7",
		false,
	},
	{ // Optimization works since first alternative is sequence start symbol.
		"([a-z]+)|7",
		true,
	},
	{ // Sequence is implemented as alternative and this alternative is optimized.
		"[0-9]*",
		true,
	},
	{ // Sequence is implemented as alternative and this alternative is optimized.
		"[0-9]*q",
		true,
	},
	{ // Sequence is implemented as alternative and this alternative is optimized, even with alternatives after sequence.
		"[0-9]*(q|Q)",
		true,
	},
	{ // Sequence is implemented as alternative and this alternative is not optimized, since symbol after sequence may be same as in the sequence.
		"[0-9]*3",
		false,
	},
	{ // Sequence is implemented as alternative and this alternative is not optimized, since symbol after sequence may be same as in the sequence.
		"[bar]*baz",
		false,
	},
	{ // Sequence is implemented as alternative and this alternative is not optimized, since sequence element is too complicated (is also sequence).
		"([a]*)*",
		false,
	},
	{ // Sequence is implemented as alternative and this alternative is not optimized, since sequence itself is possessive.
		"[0-9]*+",
		false,
	},
};

class AlternativesPossessificationOptimizationTest : public ::testing::TestWithParam<TestDataElement> {};

TEST_P(AlternativesPossessificationOptimizationTest, TestOptimization)
{
	const auto param= GetParam();
	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	const auto regex_graph= OptimizeRegexGraph( BuildRegexGraph(*regex_chain, Options()) );

	const auto possessive_alternative= std::get_if<GraphElements::AlternativesPossessive>(regex_graph.root);
	if(param.is_possessive)
		ASSERT_TRUE(possessive_alternative != nullptr);
	else
		ASSERT_TRUE(possessive_alternative == nullptr);
}

INSTANTIATE_TEST_SUITE_P(APO, AlternativesPossessificationOptimizationTest, testing::ValuesIn(g_test_data));

} // namespace

} // namespace RegPanzer
