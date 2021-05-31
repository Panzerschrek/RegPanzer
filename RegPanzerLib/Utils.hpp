#pragma once
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <llvm/Target/TargetMachine.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <string>

namespace RegPanzer
{

struct RegexFeatureFlag
{
	enum
	{
		UTF8= 1 << 0,
		LazySequences= 1 << 1,
		PossessiveSequences= 1 << 2,
		Look= 1 << 3,
		NoncapturingGroups= 1 << 4,
		AtomicGroups= 1 << 5,
		ConditionalElements= 1 << 6,
		Subroutines= 1 << 7,
		SymbolClasses= 1 << 8,
		FourDigitHexCodes= 1 << 9,
		LookBehind= 1 << 10,
		LineStartEndAssertions= 1 << 11,
	};
};

using RegexFeatureFlags= size_t;

RegexFeatureFlags GetRegexFeatures(const std::string& regex_str);
bool StringContainsNonASCIISymbols(const std::string& str);

std::unique_ptr<llvm::TargetMachine> CreateTargetMachine();

std::string Utf32ToUtf8(std::basic_string_view<char32_t> str);
std::basic_string<char32_t> Utf8ToUtf32(std::string_view str);

} // namespace RegPanzer
