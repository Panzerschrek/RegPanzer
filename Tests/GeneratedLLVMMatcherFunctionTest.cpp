#include "MatcherTestData.hpp"
#include "../RegPanzerLib/MatcherGeneratorLLVM.hpp"
#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

namespace RegPanzer
{

class GeneratedLLVMMatcherTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(GeneratedLLVMMatcherTest, TestMatch)
{
	const auto param= GetParam();
	const auto regex_chain= RegPanzer::ParseRegexString(param.regex_str);
	ASSERT_NE(regex_chain, std::nullopt);

	const auto regex_graph= BuildRegexGraph(*regex_chain);

	llvm::LLVMContext llvm_context;
	llvm::Module module("id", llvm_context);
	GenerateMatcherFunction(module, regex_graph, "Match");
	module.dump();
}

const MatcherTestDataElement g_this_test_data[]
{
	g_matcher_test_data[0]
};

INSTANTIATE_TEST_CASE_P(M, GeneratedLLVMMatcherTest, testing::ValuesIn(g_this_test_data));

} // namespace RegPanzer
