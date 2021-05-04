#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/MatcherGeneratorLLVM.hpp"
#include "../Tests/MatcherTestData.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <iostream>

namespace RegPanzer
{

namespace
{

namespace Options
{

namespace cl= llvm::cl;

cl::OptionCategory options_category("TestBinaryGenerator options");


cl::opt<std::string> output_directory(
	"o",
	cl::desc("Output directory"),
	cl::value_desc("dir"),
	cl::Required,
	cl::cat(options_category));

} // namespace Options

std::string GetNativeTargetFeaturesStr()
{
	llvm::SubtargetFeatures features;

	llvm::StringMap<bool> host_features;
	if(llvm::sys::getHostCPUFeatures(host_features))
	{
		for(auto& f : host_features)
			features.AddFeature(f.first(), f.second);
	}
	return features.getString();
}

std::string GetMatchFunctionName(const size_t index)
{
	return "Match" + std::to_string(index);
}

bool GenerateHeader()
{
	std::string function_prototypes;
	std::string function_pointers_array;

	for(const MatcherTestDataElement& test_element : g_matcher_test_data)
	{
		const auto index= size_t(&test_element - &g_matcher_test_data[0]);
		const std::string function_name= GetMatchFunctionName(index);

		function_prototypes+=
			"extern \"C\" const char* " + function_name + "(const char* begin, const char* end);\n";

		function_pointers_array+= function_name + ",\n";
	}

	const std::string ouptput_file_name= Options::output_directory + "/MatchersGenerated.hpp";


	std::error_code file_error_code;
	llvm::raw_fd_ostream out_file_stream(ouptput_file_name, file_error_code, llvm::sys::fs::F_None);

	out_file_stream << function_prototypes;
	out_file_stream << R"(
using MatcherFunction= const char*(*)(const char* begin, const char* end);

const MatcherFunction matcher_functions[]
{
)";
	out_file_stream << function_pointers_array;
	out_file_stream << "};";

	out_file_stream.flush();
	if(out_file_stream.has_error())
	{
		std::cerr << "Error while writing output file \"" << ouptput_file_name << "\": " << file_error_code.message() << std::endl;
		return false;
	}

	return true;
}

int Main(int argc, const char* argv[])
{
	const llvm::InitLLVM llvm_initializer(argc, argv);

	// Options
	llvm::cl::HideUnrelatedOptions(Options::options_category);
	llvm::cl::ParseCommandLineOptions(argc, argv, "TestBinaryGenerator\n");

	// LLVM stuff initialization.
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmPrinters();
	llvm::InitializeAllAsmParsers();

	{
		llvm::PassRegistry& registry= *llvm::PassRegistry::getPassRegistry();
		llvm::initializeCore(registry);
		llvm::initializeTransformUtils(registry);
		llvm::initializeScalarOpts(registry);
		llvm::initializeVectorization(registry);
		llvm::initializeInstCombine(registry);
		llvm::initializeAggressiveInstCombine(registry);
		llvm::initializeIPO(registry);
		llvm::initializeInstrumentation(registry);
		llvm::initializeAnalysis(registry);
		llvm::initializeCodeGen(registry);
		llvm::initializeTarget(registry);
	}

	uint32_t optimization_level= 0u, size_optimization_level= 0u;

	const llvm::Triple target_triple(llvm::sys::getDefaultTargetTriple());
	const std::string target_triple_str= target_triple.normalize();

	std::string error_str;
	const auto target= llvm::TargetRegistry::lookupTarget(target_triple_str, error_str);
	if(target == nullptr)
	{
		std::cerr << "Error, selecting target: " << error_str << std::endl;
		return 1;
	}

	llvm::TargetOptions target_options;

	auto code_gen_optimization_level= llvm::CodeGenOpt::None;
	if (size_optimization_level > 0)
		code_gen_optimization_level= llvm::CodeGenOpt::Default;
	else if(optimization_level == 0)
		code_gen_optimization_level= llvm::CodeGenOpt::None;
	else if(optimization_level == 1)
		code_gen_optimization_level= llvm::CodeGenOpt::Less;
	else if(optimization_level == 2)
		code_gen_optimization_level= llvm::CodeGenOpt::Default;
	else if(optimization_level == 3)
		code_gen_optimization_level= llvm::CodeGenOpt::Aggressive;

	std::unique_ptr<llvm::TargetMachine> target_machine(
		target->createTargetMachine(
			target_triple_str,
			llvm::sys::getHostCPUName(),
			GetNativeTargetFeaturesStr(),
			target_options,
			llvm::Reloc::Model::PIC_,
			llvm::Optional<llvm::CodeModel::Model>(),
			code_gen_optimization_level));

	if(target_machine == nullptr)
	{
		std::cerr << "Error, creating target machine." << std::endl;
		return 1;
	}

	llvm::LLVMContext llvm_context;
	llvm::Module module("Reg module", llvm_context);
	module.setDataLayout(target_machine->createDataLayout());
	module.setTargetTriple(target_triple_str);

	for(const MatcherTestDataElement& test_element : g_matcher_test_data)
	{
		const auto index= size_t(&test_element - &g_matcher_test_data[0]);

		const std::optional<RegexElementsChain> regex_chain= ParseRegexString(test_element.regex_str);
		if(regex_chain == std::nullopt)
		{
			// TODO - show exact error what went wrong.
			std::cerr << "Error, parsing regex." << std::endl;
			return 1;
		}

		const RegexGraphBuildResult regex_graph= BuildRegexGraph(*regex_chain);
		GenerateMatcherFunction(module, regex_graph, GetMatchFunctionName(index));
	}

	llvm::legacy::FunctionPassManager function_pass_manager(&module);
	llvm::legacy::PassManager pass_manager;

	// Setup target-dependent optimizations.
	pass_manager.add(llvm::createTargetTransformInfoWrapperPass(target_machine->getTargetIRAnalysis()));

	{
		llvm::PassManagerBuilder pass_manager_builder;
		pass_manager_builder.OptLevel = optimization_level;
		pass_manager_builder.SizeLevel = size_optimization_level;

		if(optimization_level == 0u)
			pass_manager_builder.Inliner= nullptr;
		else
			pass_manager_builder.Inliner= llvm::createFunctionInliningPass(optimization_level, size_optimization_level, false);

		// vectorization/unroll is same as in "opt"
		pass_manager_builder.DisableUnrollLoops= optimization_level == 0;
		pass_manager_builder.LoopVectorize= optimization_level > 1 && size_optimization_level < 2;
		pass_manager_builder.SLPVectorize= optimization_level > 1 && size_optimization_level < 2;

		target_machine->adjustPassManager(pass_manager_builder);

		if (llvm::TargetPassConfig* const target_pass_config= static_cast<llvm::LLVMTargetMachine &>(*target_machine).createPassConfig(pass_manager))
			pass_manager.add(target_pass_config);

		pass_manager_builder.populateFunctionPassManager(function_pass_manager);
		pass_manager_builder.populateModulePassManager(pass_manager);
	}

	const std::string ouptput_file_name= Options::output_directory + "/MatchersGenerated.o";

	std::error_code file_error_code;
	llvm::raw_fd_ostream out_file_stream(ouptput_file_name, file_error_code, llvm::sys::fs::F_None);

	if(target_machine->addPassesToEmitFile(pass_manager, out_file_stream, nullptr, llvm::TargetMachine::CGFT_ObjectFile))
	{
		std::cerr << "Error, creating file emit pass." << std::endl;
		return 1;
	}

	// Run per-function optimizations.
	function_pass_manager.doInitialization();
	for(llvm::Function& func : module)
		function_pass_manager.run(func);
	function_pass_manager.doFinalization();

	// Run all passes.
	pass_manager.run(module);

	out_file_stream.flush();
	if(out_file_stream.has_error())
	{
		std::cerr << "Error while writing output file \"" << ouptput_file_name << "\": " << file_error_code.message() << std::endl;
		return 1;
	}

	if(!GenerateHeader())
		return 1;

	return 0;
}

} // namespace

} // namespace RegPanzer

int main(const int argc, const char* argv[])
{
	// Place actual "main" body inside "RegPanzer" namespace.
	return RegPanzer::Main(argc, argv);
}
