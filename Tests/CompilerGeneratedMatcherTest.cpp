#include "MatcherTestData.hpp"
#include "GroupsExtractionTestData.hpp"
#include "../RegPanzerLib/MatcherGeneratorLLVM.hpp"
#include "../RegPanzerLib/Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/Program.h>
#include <gtest/gtest.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"

namespace RegPanzer
{

namespace
{

const std::string function_name= "test_match";
const std::string object_file_path= "test.o";
const std::string compiler_program= "RegPanzerCompiler";

void RunTestCase(const MatcherTestDataElement& param, const bool is_multiline)
{
	// Launch compiler, produce object file, load it into MCJIT Execition engine and run function from it.

	{
		// This test must be launched from build directory, where also located the Compiler executable.

		llvm::SmallVector<llvm::StringRef, 8> args
			{compiler_program, param.regex_str, "--function-name", function_name, "-o", object_file_path, "-O2"};

		if(is_multiline)
			args.push_back("-m");

		const int res= llvm::sys::ExecuteAndWait(compiler_program, args);
		ASSERT_EQ(res, 0);
	}

	auto target_machine= CreateTargetMachine();
	ASSERT_TRUE(target_machine != nullptr);

	llvm::LLVMContext llvm_context;
	auto module= std::make_unique<llvm::Module>("id", llvm_context);
	module->setDataLayout(target_machine->createDataLayout());

	llvm::EngineBuilder builder(std::move(module));
	builder.setEngineKind(llvm::EngineKind::JIT);
	builder.setMemoryManager(std::make_unique<llvm::SectionMemoryManager>());
	const std::unique_ptr<llvm::ExecutionEngine> engine(builder.create(target_machine.release())); // Engine takes ownership over target machine.
	ASSERT_TRUE(engine != nullptr);

	auto object_file= llvm::object::ObjectFile::createObjectFile(object_file_path);
	ASSERT_TRUE(static_cast<bool>(object_file));

	engine->addObjectFile(std::move(*object_file));

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

class CompilerGeneratedMatcherTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(CompilerGeneratedMatcherTest, TestMatch)
{
	RunTestCase(GetParam(), false);
}

INSTANTIATE_TEST_SUITE_P(M, CompilerGeneratedMatcherTest, testing::ValuesIn(g_matcher_test_data, g_matcher_test_data + g_matcher_test_data_size));


class CompilerGeneratedMatcherMultilineTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(CompilerGeneratedMatcherMultilineTest, TestMatch)
{
	RunTestCase(GetParam(), true);
}

INSTANTIATE_TEST_SUITE_P(M, CompilerGeneratedMatcherMultilineTest, testing::ValuesIn(g_matcher_multiline_test_data, g_matcher_multiline_test_data + g_matcher_multiline_test_data_size));


class CompilerGeneratedMatcherGroupsExtractionTest : public ::testing::TestWithParam<GroupsExtractionTestDataElement> {};

TEST_P(CompilerGeneratedMatcherGroupsExtractionTest, TestGroupsExtraction)
{
	// Launch compiler, produce object file, load it into MCJIT Execition engine and run function from it.

	const auto param= GetParam();

	{
		// This test must be launched from build directory, where also located the Compiler executable.
		const int res= llvm::sys::ExecuteAndWait(
			compiler_program,
			{compiler_program, param.regex_str, "--function-name", function_name, "--extract-groups", "-o", object_file_path, "-O2"});
		ASSERT_EQ(res, 0);
	}

	auto target_machine= CreateTargetMachine();
	ASSERT_TRUE(target_machine != nullptr);

	llvm::LLVMContext llvm_context;
	auto module= std::make_unique<llvm::Module>("id", llvm_context);
	module->setDataLayout(target_machine->createDataLayout());

	llvm::EngineBuilder builder(std::move(module));
	builder.setEngineKind(llvm::EngineKind::JIT);
	builder.setMemoryManager(std::make_unique<llvm::SectionMemoryManager>());
	const std::unique_ptr<llvm::ExecutionEngine> engine(builder.create(target_machine.release())); // Engine takes ownership over target machine.
	ASSERT_TRUE(engine != nullptr);

	auto object_file= llvm::object::ObjectFile::createObjectFile(object_file_path);
	ASSERT_TRUE(static_cast<bool>(object_file));

	engine->addObjectFile(std::move(*object_file));

	const auto function= reinterpret_cast<MatcherFunctionType>(engine->getFunctionAddress(function_name));
	ASSERT_TRUE(function != nullptr);

	for(const GroupsExtractionTestDataElement::Case& c : param.cases)
	{
		std::vector<GroupsExtractionTestDataElement::GroupMatchResults> results;
		for(size_t i= 0; i < c.input_str.size();)
		{
			size_t groups[10][2]{};
			const auto subpatterns_extracted= function(c.input_str.data(), c.input_str.size(), i, &groups[0][0], std::size(groups));

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

INSTANTIATE_TEST_SUITE_P(GE, CompilerGeneratedMatcherGroupsExtractionTest, testing::ValuesIn(g_groups_extraction_test_data, g_groups_extraction_test_data + g_groups_extraction_test_data_size));

} // namespace

} // namespace RegPanzer
