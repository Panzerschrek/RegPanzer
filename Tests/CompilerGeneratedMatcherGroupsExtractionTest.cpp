#include "GroupsExtractionTestData.hpp"
#include "Utils.hpp"
#include "../RegPanzerLib/MatcherGeneratorLLVM.hpp"
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

std::string EscapeShellString(const std::string& str)
{
	std::string res;
	res+= "\"";
	for(const char c : str)
	{
		if(c == '\\')
			res+= "\\\\";
		else if(c == '"')
			res+= "\\\"";
		else
			res.push_back(c);
	}
	res+= "\"";
	return res;
}

class CompilerGeneratedMatcherGroupsExtractionTest : public ::testing::TestWithParam<GroupsExtractionTestDataElement> {};

TEST_P(CompilerGeneratedMatcherGroupsExtractionTest, TestGroupsExtraction)
{
	// Launch compiler, produce object file, load it into MCJIT Execition engine and run function from it.

	const auto param= GetParam();

	const std::string function_name= "test_match";
	const std::string object_file_path= "test.o";
	{
		// This test must be launched from build directory, where also located the Compiler executable.

		const std::string compiler_path= "./RegPanzerCompiler";
		const std::string command=
			compiler_path + " " +
			EscapeShellString(param.regex_str) + " " +
			"--function-name " + function_name + " " +
			"--extract-groups" + " " +
			"-o " + object_file_path + " " +
			"-O2";

		const auto res= std::system(command.c_str());
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
				++i;
			else
			{
				if(groups[0][1] <= i && groups[0][1] <= groups[0][0])
					break;
				i= groups[0][1];

				GroupsExtractionTestDataElement::GroupMatchResults result;

				for(size_t j= 0; j < std::min(subpatterns_extracted, std::size(groups)); ++j)
					result.emplace_back(groups[j][0], groups[j][1]);

				results.push_back(std::move(result));
			}
		}

		EXPECT_EQ(results, c.results);
	}
}

INSTANTIATE_TEST_CASE_P(GE, CompilerGeneratedMatcherGroupsExtractionTest, testing::ValuesIn(g_groups_extraction_test_data, g_groups_extraction_test_data + g_groups_extraction_test_data_size));

} // namespace

} // namespace RegPanzer
