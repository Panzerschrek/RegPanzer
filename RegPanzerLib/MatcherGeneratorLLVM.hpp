#pragma once
#include "RegexGraph.hpp"
#include "PushDisableLLVMWarnings.hpp"
#include <llvm/IR/Module.h>
#include "PopLLVMWarnings.hpp"

namespace RegPanzer
{

void GenerateMatcherFunction(
	llvm::Module& module,
	const GraphElements::NodePtr& regex_graph_root,
	const std::string& function_name);

} // namespace RegPanzer
