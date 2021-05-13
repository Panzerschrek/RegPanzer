#include "../RegPanzerLib/Parser.hpp"
#include "../RegPanzerLib/MatcherGeneratorLLVM.hpp"
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

void PrintAvailableTargets()
{
	std::string targets_list;
	for(const llvm::Target& target : llvm::TargetRegistry::targets())
	{
		if(!targets_list.empty())
			targets_list+= ", ";
		targets_list+= target.getName();
	}
	std::cout << "Available targets: " << targets_list << std::endl;
}

std::string GetFeaturesStr(const llvm::ArrayRef<std::string> features_list)
{
	llvm::SubtargetFeatures features;
	for(auto& f : features_list)
		features.AddFeature(f, true);
	return features.getString();
}

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

namespace Options
{

namespace cl= llvm::cl;

cl::OptionCategory options_category("RegPanzer Compier options");

cl::opt<std::string> input_regex(
	cl::Positional,
	cl::desc("<regex>"),
	cl::value_desc("input regex"),
	cl::Required,
	cl::cat(options_category));

cl::opt<std::string> result_function_name(
	"function-name",
	cl::desc("Result function name"),
	cl::init("Match"),
	cl::cat(options_category));

cl::opt<std::string> output_file_name(
	"o",
	cl::desc("Output filename"),
	cl::value_desc("filename"),
	cl::Required,
	cl::cat(options_category));

enum class FileType{ BC, LL, Obj, Asm };
cl::opt< FileType > file_type(
	"filetype",
	cl::init(FileType::Obj),
	cl::desc("Choose a file type (not all types are supported by all targets):"),
	cl::values(
		clEnumValN(FileType::BC, "bc", "Emit an llvm bitcode ('.bc') file"),
		clEnumValN(FileType::LL, "ll", "Emit an llvm asm ('.ll') file"),
		clEnumValN(FileType::Obj, "obj", "Emit a native object ('.o') file"),
		clEnumValN(FileType::Asm, "asm", "Emit an assembly ('.s') file")),
	cl::cat(options_category));

cl::opt<char> optimization_level(
	"O",
	cl::desc("Optimization level. [-O0, -O1, -O2, -O3, -Os or -Oz] (default = '-O0')"),
	cl::Prefix,
	cl::Optional,
	cl::init('0'),
	cl::cat(options_category));

cl::opt<std::string> architecture(
	"march",
	cl::desc("Architecture to generate code for (see --version)"),
	cl::cat(options_category));

cl::opt<std::string> target_vendor(
	"target-vendor",
	cl::desc("Target vendor"),
	cl::cat(options_category));

cl::opt<std::string> target_os(
	"target-os",
	cl::desc("Target OS"),
	cl::cat(options_category));

cl::opt<std::string> target_environment(
	"target-environment",
	cl::desc("Target environment"),
	cl::cat(options_category));

cl::opt<std::string> target_cpu(
	"mcpu",
	cl::desc("Target a specific cpu type (-mcpu=help for details)"),
	cl::value_desc("cpu-name"),
	cl::init(""),
	cl::cat(options_category));

cl::list<std::string> target_attributes(
	"mattr",
	cl::CommaSeparated,
	cl::desc("Target specific attributes (-mattr=help for details)"),
	cl::value_desc("a1,+a2,-a3,..."),
	cl::cat(options_category));

cl::opt<llvm::Reloc::Model> relocation_model(
	"relocation-model",
	cl::desc("Choose relocation model"),
	cl::init(llvm::Reloc::PIC_),
	cl::values(
		clEnumValN(llvm::Reloc::Static, "static", "Non-relocatable code"),
		clEnumValN(llvm::Reloc::PIC_, "pic", "Fully relocatable, position independent code"),
		clEnumValN(llvm::Reloc::DynamicNoPIC, "dynamic-no-pic", "Relocatable external references, non-relocatable code"),
		clEnumValN(llvm::Reloc::ROPI, "ropi", "Code and read-only data relocatable, accessed PC-relative"),
		clEnumValN(llvm::Reloc::RWPI, "rwpi", "Read-write data relocatable, accessed relative to static base"),
		clEnumValN(llvm::Reloc::ROPI_RWPI, "ropi-rwpi", "Combination of ropi and rwpi")),
	cl::cat(options_category));

cl::opt<llvm::CodeModel::Model> code_model(
	"code-model",
	cl::desc("Choose code model"),
	cl::values(
		clEnumValN(llvm::CodeModel::Tiny, "tiny", "Tiny code model"),
		clEnumValN(llvm::CodeModel::Small, "small", "Small code model"),
		clEnumValN(llvm::CodeModel::Kernel, "kernel", "Kernel code model"),
		clEnumValN(llvm::CodeModel::Medium, "medium", "Medium code model"),
		clEnumValN(llvm::CodeModel::Large, "large", "Large code model")),
	cl::cat(options_category));

} // namespace Options

int Main(int argc, const char* argv[])
{
	const llvm::InitLLVM llvm_initializer(argc, argv);

	// Options
	llvm::cl::HideUnrelatedOptions(Options::options_category);
	llvm::cl::ParseCommandLineOptions(argc, argv, "RegPanzer Compiler\n");

	// Select optimization level.
	uint32_t optimization_level= 0u, size_optimization_level= 0u;
	const char opt_level_char= Options::optimization_level;
	switch(opt_level_char)
	{
	case '0':
	case '1':
	case '2':
	case '3':
		optimization_level= uint32_t(opt_level_char - '0');
		break;
	case 's':
		size_optimization_level= 1u;
		optimization_level= 2u;
		break;
	case 'z':
		size_optimization_level= 2u;
		optimization_level= 2u;
		break;
	default:
		std::cout << "Unknown optimization: " << Options::optimization_level << std::endl;
		return 1;
	};

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

	// Prepare target machine.
	std::string target_triple_str;
	std::unique_ptr<llvm::TargetMachine> target_machine;
	{
		const llvm::Target* target= nullptr;

		llvm::Triple target_triple(llvm::sys::getDefaultTargetTriple());

		if(!Options::architecture.empty() && Options::architecture != "native")
			target_triple.setArchName(Options::architecture);
		if(!Options::target_vendor.empty())
			target_triple.setVendorName(Options::target_vendor);
		if(!Options::target_os.empty())
			target_triple.setOSName(Options::target_os);
		if(!Options::target_environment.empty())
			target_triple.setEnvironmentName(Options::target_environment);

		target_triple_str= target_triple.normalize();

		std::string error_str;
		target= llvm::TargetRegistry::lookupTarget(target_triple_str, error_str);
		if(target == nullptr)
		{
			std::cerr << "Error, selecting target: " << error_str << std::endl;
			PrintAvailableTargets();
			return 1;
		}

		const std::string cpu_name= (Options::architecture == "native" && Options::target_cpu.empty())
			? llvm::sys::getHostCPUName()
			: Options::target_cpu;

		const std::string features_str= (Options::architecture == "native" && Options::target_attributes.empty())
			? GetNativeTargetFeaturesStr()
			: GetFeaturesStr(Options::target_attributes);

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

		target_machine.reset(
			target->createTargetMachine(
				target_triple_str,
				cpu_name,
				features_str,
				target_options,
				Options::relocation_model.getValue(),
				Options::code_model.getNumOccurrences() > 0 ? Options::code_model.getValue() : llvm::Optional<llvm::CodeModel::Model>(),
				code_gen_optimization_level));

		if(target_machine == nullptr)
		{
			std::cerr << "Error, creating target machine." << std::endl;
			return 1;
		}
	}

	// Create llvm module.
	llvm::LLVMContext llvm_context;
	llvm::Module module("Reg module", llvm_context);
	module.setDataLayout(target_machine->createDataLayout());
	module.setTargetTriple(target_triple_str);

	// Parse and build regex.
	const auto parse_res= ParseRegexString(Options::input_regex);
	if(const auto parse_errors= std::get_if<ParseErrors>(&parse_res))
	{
		// TODO - show exact error what went wrong.
		std::cerr << "Errors, parsing regex:\n";
		for(const ParseError& e : *parse_errors)
			std::cerr << e.pos << ": " << e.message << "\n";
		std::cerr << std::endl;
		return 1;
	}
	const auto regex_chain= std::get_if<RegexElementsChain>(&parse_res);
	assert(regex_chain != nullptr);

	RegPanzer::Options regex_build_options;
	// TODO - fill options.

	const RegexGraphBuildResult regex_graph= BuildRegexGraph(*regex_chain, regex_build_options);
	GenerateMatcherFunction(module, regex_graph, Options::result_function_name);

	// Run optimizations.
	if(optimization_level > 0u || size_optimization_level > 0u)
	{
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

		// Run per-function optimizations.
		function_pass_manager.doInitialization();
		for(llvm::Function& func : module)
			function_pass_manager.run(func);
		function_pass_manager.doFinalization();

		// Run optimizations for module.
		pass_manager.run(module);
	}

	// Write result file.
	{
		std::error_code file_error_code;
		llvm::raw_fd_ostream out_file_stream(Options::output_file_name, file_error_code, llvm::sys::fs::F_None);

		if(Options::file_type == Options::FileType::BC)
			llvm::WriteBitcodeToFile(module, out_file_stream);
		else if(Options::file_type == Options::FileType::LL)
			module.print(out_file_stream, nullptr);
		else
		{
			llvm::TargetMachine::CodeGenFileType file_type= llvm::TargetMachine::CGFT_Null;
			switch(Options::file_type)
			{
			case Options::FileType::Obj: file_type= llvm::TargetMachine::CGFT_ObjectFile; break;
			case Options::FileType::Asm: file_type= llvm::TargetMachine::CGFT_AssemblyFile; break;
			case Options::FileType::BC:
			case Options::FileType::LL:
			assert(false);
			};

			llvm::legacy::PassManager pass_manager;
			if(target_machine->addPassesToEmitFile(pass_manager, out_file_stream, nullptr, file_type))
			{
				std::cerr << "Error, creating file emit pass." << std::endl;
				return 1;
			}

			pass_manager.run(module);
		}

		out_file_stream.flush();
		if(out_file_stream.has_error())
		{
			std::cerr << "Error while writing output file \"" << Options::output_file_name << "\": " << file_error_code.message() << std::endl;
			return 1;
		}
	}

	return 0;
}

} // namespace

} // namespace RegPanzer

int main(const int argc, const char* argv[])
{
	// Place actual "main" body inside "RegPanzer" namespace.
	return RegPanzer::Main(argc, argv);
}
