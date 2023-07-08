#include "MatcherTestData.hpp"
#include "GroupsExtractionTestData.hpp"
#include "../RegPanzerLib/MatcherGeneratorLLVM.hpp"
#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/RegexGraphOptimizer.hpp"
#include "../RegPanzerLib/Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

const std::string GetTestsDataLayout()
{
	std::string result;

	result+= llvm::sys::IsBigEndianHost ? "E" : "e";
	const bool is_32_bit= sizeof(void*) <= 4u;
	result+= is_32_bit ? "-p:32:32" : "-p:64:64";
	result+= is_32_bit ? "-n8:16:32" : "-n8:16:32:64";
	result+= "-i8:8-i16:16-i32:32-i64:64";
	result+= "-f32:32-f64:64";
	result+= "-S128";

	return result;
}

void RunTestCase(const MatcherTestDataElement& param, const bool is_multiline)
{
	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	Options options;
	options.multiline= is_multiline;
	const auto regex_graph= OptimizeRegexGraph( BuildRegexGraph(*regex_chain, options) );

	llvm::LLVMContext llvm_context;
	auto module= std::make_unique<llvm::Module>("id", llvm_context);
	module->setDataLayout(GetTestsDataLayout());

	const std::string function_name= "Match";
	GenerateMatcherFunction(*module, regex_graph, function_name);

	llvm::EngineBuilder builder(std::move(module));
	builder.setEngineKind(llvm::EngineKind::Interpreter);
	const std::unique_ptr<llvm::ExecutionEngine> engine(builder.create());
	ASSERT_TRUE(engine != nullptr);

	llvm::Function* const function= engine->FindFunctionNamed(function_name);
	ASSERT_TRUE(function != nullptr);

	for(const MatcherTestDataElement::Case& c : param.cases)
	{
		MatcherTestDataElement::Ranges result_ranges;
		for(size_t i= 0; i < c.input_str.size();)
		{
			size_t group[2]{0, 0};

			llvm::GenericValue args[5];
			args[0].PointerVal= const_cast<char*>(c.input_str.data());
			args[1].IntVal= llvm::APInt(sizeof(size_t) * 8, c.input_str.size());
			args[2].IntVal= llvm::APInt(sizeof(size_t) * 8, i);
			args[3].PointerVal= &group;
			args[4].IntVal= llvm::APInt(sizeof(size_t) * 8, 1);

			const llvm::GenericValue result_value= engine->runFunction(function, args);
			const auto subpatterns_extracted= result_value.IntVal.getLimitedValue();

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

class GeneratedLLVMMatcherTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(GeneratedLLVMMatcherTest, TestMatch)
{
	RunTestCase(GetParam(), false);
}

INSTANTIATE_TEST_SUITE_P(M, GeneratedLLVMMatcherTest, testing::ValuesIn(g_matcher_test_data, g_matcher_test_data + g_matcher_test_data_size));


class GeneratedLLVMMatcherMultilineTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(GeneratedLLVMMatcherMultilineTest, TestMatch)
{
	RunTestCase(GetParam(), true);
}

INSTANTIATE_TEST_SUITE_P(M, GeneratedLLVMMatcherMultilineTest, testing::ValuesIn(g_matcher_multiline_test_data, g_matcher_multiline_test_data + g_matcher_multiline_test_data_size));


class GeneratedLLVMMatcherGroupsExtractionTest : public ::testing::TestWithParam<GroupsExtractionTestDataElement> {};

TEST_P(GeneratedLLVMMatcherGroupsExtractionTest, TestGroupsExtraction)
{
	const auto param= GetParam();
	const auto parse_res= RegPanzer::ParseRegexString(param.regex_str);
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	ASSERT_TRUE(regex_chain != nullptr);

	Options options;
	options.extract_groups= true;
	const auto regex_graph= OptimizeRegexGraph( BuildRegexGraph(*regex_chain, options) );

	llvm::LLVMContext llvm_context;
	auto module= std::make_unique<llvm::Module>("id", llvm_context);
	module->setDataLayout(GetTestsDataLayout());

	const std::string function_name= "Match";
	GenerateMatcherFunction(*module, regex_graph, function_name);

	llvm::EngineBuilder builder(std::move(module));
	builder.setEngineKind(llvm::EngineKind::Interpreter);
	const std::unique_ptr<llvm::ExecutionEngine> engine(builder.create());
	ASSERT_TRUE(engine != nullptr);

	llvm::Function* const function= engine->FindFunctionNamed(function_name);
	ASSERT_TRUE(function != nullptr);

	for(const GroupsExtractionTestDataElement::Case& c : param.cases)
	{
		std::vector<GroupsExtractionTestDataElement::GroupMatchResults> results;
		for(size_t i= 0; i < c.input_str.size();)
		{
			size_t groups[10][2]{};

			llvm::GenericValue args[5];
			args[0].PointerVal= const_cast<char*>(c.input_str.data());
			args[1].IntVal= llvm::APInt(sizeof(size_t) * 8, c.input_str.size());
			args[2].IntVal= llvm::APInt(sizeof(size_t) * 8, i);
			args[3].PointerVal= &groups[0][0];
			args[4].IntVal= llvm::APInt(sizeof(size_t) * 8, std::size(groups));

			const llvm::GenericValue result_value= engine->runFunction(function, args);
			const auto subpatterns_extracted= result_value.IntVal.getLimitedValue();

			if(subpatterns_extracted == 0)
				break;

			if(groups[0][1] <= i && groups[0][1] <= groups[0][0])
				break;
			i= groups[0][1];

			GroupsExtractionTestDataElement::GroupMatchResults result;

			for(size_t j= 0; j < std::min(subpatterns_extracted, std::size(groups)); ++j)
				result.emplace_back(groups[j][0], groups[j][1]);

			results.push_back(std::move(result));
		}

		EXPECT_EQ(results, c.results);
	}
}

INSTANTIATE_TEST_SUITE_P(GE, GeneratedLLVMMatcherGroupsExtractionTest, testing::ValuesIn(g_groups_extraction_test_data, g_groups_extraction_test_data + g_groups_extraction_test_data_size));

} // namespace

} // namespace RegPanzer
