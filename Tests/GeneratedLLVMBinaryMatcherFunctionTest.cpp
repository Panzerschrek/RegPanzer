#include "MatcherTestData.hpp"
#include "../RegPanzerLib/MatcherGeneratorLLVM.hpp"
#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/RegexGraphOptimizer.hpp"
#include "../RegPanzerLib/Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

void RunTestCase(const MatcherTestDataElement& param, const bool is_multiline)
{
	auto target_machine= CreateTargetMachine();
	ASSERT_TRUE(target_machine != nullptr);

	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	Options options;
	options.multiline= is_multiline;
	const auto regex_graph= OptimizeRegexGraph( BuildRegexGraph(*regex_chain, options) );

	const std::string function_name= "Match";

	llvm::LLVMContext llvm_context;
	auto module= std::make_unique<llvm::Module>("id", llvm_context);
	module->setDataLayout(target_machine->createDataLayout());

	GenerateMatcherFunction(*module, regex_graph, function_name);

	llvm::EngineBuilder builder(std::move(module));
	builder.setEngineKind(llvm::EngineKind::JIT);
	builder.setMemoryManager(std::make_unique<llvm::SectionMemoryManager>());
	const std::unique_ptr<llvm::ExecutionEngine> engine(builder.create(target_machine.release())); // Engine takes ownership over target machine.
	ASSERT_TRUE(engine != nullptr);

	const auto function= reinterpret_cast<MatcherFunctionType>(engine->getFunctionAddress(function_name));
	ASSERT_TRUE(function != nullptr);

	for(const MatcherTestDataElement::Case& c : param.cases)
	{
		MatcherTestDataElement::Ranges result_ranges;
		for(size_t i= 0; i < c.input_str.size();)
		{
			size_t group[2]{};
			const auto subpatterns_extracted= function(c.input_str.data(), c.input_str.size(), i, group, 1);

			if(subpatterns_extracted == 0)
				break;

			result_ranges.emplace_back(group[0], group[1]);
			if(group[1] <= i && group[1] <= group[0])
				break;
			i= group[1];
		}

		EXPECT_EQ(result_ranges, c.result_ranges);
	}
}

class GeneratedLLVMBinaryMatcherTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(GeneratedLLVMBinaryMatcherTest, TestMatch)
{
	RunTestCase(GetParam(), false);
}

INSTANTIATE_TEST_SUITE_P(M, GeneratedLLVMBinaryMatcherTest, testing::ValuesIn(g_matcher_test_data, g_matcher_test_data + g_matcher_test_data_size));


class GeneratedLLVMBinaryMatcherMultilineTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(GeneratedLLVMBinaryMatcherMultilineTest, TestMatch)
{
	RunTestCase(GetParam(), true);
}

INSTANTIATE_TEST_SUITE_P(M, GeneratedLLVMBinaryMatcherMultilineTest, testing::ValuesIn(g_matcher_multiline_test_data, g_matcher_multiline_test_data + g_matcher_multiline_test_data_size));

} // namespace

} // namespace RegPanzer
