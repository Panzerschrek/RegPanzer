#include "MatcherTestData.hpp"
#include "Utils.hpp"
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

class CompilerGeneratedMatcherTest : public ::testing::TestWithParam<MatcherTestDataElement> {};

TEST_P(CompilerGeneratedMatcherTest, TestMatch)
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

	using FunctionType= const char*(*)(const char*, const char*);
	const auto function= reinterpret_cast<FunctionType>(engine->getFunctionAddress(function_name));
	ASSERT_TRUE(function != nullptr);

	for(const MatcherTestDataElement::Case& c : param.cases)
	{
		MatcherTestDataElement::Ranges result_ranges;
		for(size_t i= 0; i < c.input_str.size();)
		{
			const char* const match_end_ptr= function(c.input_str.data() + i, c.input_str.data() + c.input_str.size());

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

INSTANTIATE_TEST_CASE_P(M, CompilerGeneratedMatcherTest, testing::ValuesIn(g_matcher_test_data));

} // namespace

} // namespace RegPanzer
