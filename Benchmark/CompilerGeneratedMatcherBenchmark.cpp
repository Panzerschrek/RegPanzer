#include "BenchmarkData.hpp"
#include "../RegPanzerLib/MatcherGeneratorLLVM.hpp"
#include "../RegPanzerLib/Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <benchmark/benchmark.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/Program.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"


namespace RegPanzer
{

namespace
{

void CompilerGeneratedMatcherBenchmark(benchmark::State& st)
{
	const auto& param= g_benchmark_data[st.range(0)];
	const std::string function_name= "test_match";
	const std::string object_file_path= "test.o";
	const std::string compiler_program= "RegPanzerCompiler";

	llvm::sys::ExecuteAndWait(
		compiler_program,
		{compiler_program, param.regex_str, "--function-name", function_name, "-o", object_file_path, "-O2"});

	auto target_machine= CreateTargetMachine();

	llvm::LLVMContext llvm_context;
	auto module= std::make_unique<llvm::Module>("id", llvm_context);
	module->setDataLayout(target_machine->createDataLayout());

	llvm::EngineBuilder builder(std::move(module));
	builder.setEngineKind(llvm::EngineKind::JIT);
	builder.setMemoryManager(std::make_unique<llvm::SectionMemoryManager>());
	const std::unique_ptr<llvm::ExecutionEngine> engine(builder.create(target_machine.release())); // Engine takes ownership over target machine.

	auto object_file= llvm::object::ObjectFile::createObjectFile(object_file_path);
	engine->addObjectFile(std::move(*object_file));

	const auto function= reinterpret_cast<MatcherFunctionType>(engine->getFunctionAddress(function_name));

	const auto test_data= param.data_generation_func();

	for (auto _ : st)
	{
		size_t count= 0;
		for(size_t i= 0; i < test_data.size();)
		{
			size_t group[2]{};
			const auto subpatterns_extracted= function(test_data.data(), test_data.size(), i, group, 1);

			if(subpatterns_extracted == 0)
				++i;
			else
			{
				++count;

				if(group[1] <= i && group[1] <= group[0])
					break;
				i= group[1];
			}
		}
	}
}

BENCHMARK(CompilerGeneratedMatcherBenchmark)->DenseRange(0, int64_t(g_benchmark_data_size) - 1);

} // namespace

} // namespace RegPanzer
