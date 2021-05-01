#include "MatcherTestData.hpp"
#include "Utils.hpp"
#include "../RegPanzerLib/MatcherGeneratorLLVM.hpp"
#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
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

	const std::string function_name= "Match";

	llvm::LLVMContext llvm_context;
	auto module= std::make_unique<llvm::Module>("id", llvm_context);
	GenerateMatcherFunction(*module, regex_graph, "Match");

	llvm::EngineBuilder builder( std::move(module) );
	llvm::ExecutionEngine* const engine= builder.create();
	ASSERT_NE(engine, nullptr);

	llvm::Function* const function= engine->FindFunctionNamed(function_name);
	ASSERT_NE(function, nullptr);

	for(const MatcherTestDataElement::Case& c : param.cases)
	{
		// TODO - remove this check when this matcher generator will support UTF-8.
		if(StringContainsNonASCIISymbols(c.input_str))
			continue;

		MatcherTestDataElement::Ranges result_ranges;
		for(size_t i= 0; i < c.input_str.size();)
		{
			llvm::GenericValue args[2];
			args[0].PointerVal= const_cast<char*>(c.input_str.data() + i);
			args[1].PointerVal= const_cast<char*>(c.input_str.data() + c.input_str.size());

			const llvm::GenericValue result_value= engine->runFunction(function, args);
			const char* const match_end_ptr= reinterpret_cast<const char*>(result_value.PointerVal);

			if(match_end_ptr == nullptr)
				++i;
			else
			{
				const size_t end_offset= size_t(match_end_ptr - c.input_str.data());
				result_ranges.emplace_back(i, end_offset);
				i= end_offset;
			}
		}

		EXPECT_EQ(result_ranges, c.result_ranges);
	}
}

const MatcherTestDataElement g_this_test_data[]
{
	g_matcher_test_data[0],
	g_matcher_test_data[1],
	g_matcher_test_data[2],
	g_matcher_test_data[3],
	g_matcher_test_data[4],
	g_matcher_test_data[5],
	g_matcher_test_data[6],
	g_matcher_test_data[7],
};

INSTANTIATE_TEST_CASE_P(M, GeneratedLLVMMatcherTest, testing::ValuesIn(g_this_test_data));

} // namespace RegPanzer
