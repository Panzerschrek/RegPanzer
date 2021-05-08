#pragma once
#include "RegexGraph.hpp"
#include "PushDisableLLVMWarnings.hpp"
#include <llvm/IR/Module.h>
#include "PopLLVMWarnings.hpp"

namespace RegPanzer
{

// Type of generated matcher function. sizeof(size_t) depends on target data layout.
using MatcherFunctionType= const char*(*)(const char* str, size_t str_size, size_t start_offset);

// Input module should contain data layout.

void GenerateMatcherFunction(
	llvm::Module& module,
	const RegexGraphBuildResult& regex_graph,
	const std::string& function_name);

} // namespace RegPanzer
