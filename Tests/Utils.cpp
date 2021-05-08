#include "../RegPanzerLib/Parser.hpp"
#include "Utils.hpp"
#include "../RegPanzerLib/PushDisableLLVMWarnings.hpp"
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include "../RegPanzerLib/PopLLVMWarnings.hpp"
#include <iostream>

namespace RegPanzer
{

namespace
{

RegexFeatureFlags GetSequeneFeatures(const RegexElementsChain& chain);

template<typename T> RegexFeatureFlags GetSequeneFeaturesForElement(const T&){ return 0; }

RegexFeatureFlags GetSequeneFeaturesForElement(const Look& look)
{
	return RegexFeatureFlag::Look | GetSequeneFeatures(look.elements);
}

RegexFeatureFlags GetSequeneFeaturesForElement(const Group& group)
{
	return GetSequeneFeatures(group.elements);
}

RegexFeatureFlags GetSequeneFeaturesForElement(const NonCapturingGroup& noncapturing_group)
{
	return RegexFeatureFlag::NoncapturingGroups | GetSequeneFeatures(noncapturing_group.elements);
}

RegexFeatureFlags GetSequeneFeaturesForElement(const AtomicGroup& atomic_group)
{
	return RegexFeatureFlag::AtomicGroups | GetSequeneFeatures(atomic_group.elements);
}

RegexFeatureFlags GetSequeneFeaturesForElement(const ConditionalElement& conditional_element)
{
	return
		RegexFeatureFlag::ConditionalElements |
		GetSequeneFeaturesForElement(conditional_element.look) |
		GetSequeneFeaturesForElement(conditional_element.alternatives);
}

RegexFeatureFlags GetSequeneFeaturesForElement(const Alternatives& alternatives)
{
	RegexFeatureFlags flags= 0;
	for(const auto& alternative : alternatives.alternatives)
		flags|= GetSequeneFeatures(alternative);
	return flags;
}

RegexFeatureFlags GetSequeneFeaturesForElement(const SubroutineCall&)
{
	return RegexFeatureFlag::Subroutines;
}

RegexFeatureFlags GetSequeneFeatures(const RegexElementsChain& chain)
{
	RegexFeatureFlags flags= 0;
	for(const RegexElementFull& element : chain)
	{
		flags|= std::visit([&](const auto& el){ return GetSequeneFeaturesForElement(el); }, element.el);

		if(element.seq.mode == SequenceMode::Lazy)
			flags|= RegexFeatureFlag::LazySequences;
		if(element.seq.mode == SequenceMode::Possessive)
			flags|= RegexFeatureFlag::PossessiveSequences;
	}
	return flags;
}

RegexFeatureFlags GetExcapeSequencesFlags(const std::string& regex_str)
{
	RegexFeatureFlags flags= 0;
	for(auto it= regex_str.begin(), it_end= regex_str.end(); it != it_end; )
	{
		const auto next_it= std::next(it);
		if(*it == '\\' && next_it != regex_str.end())
		{
			if(*next_it == '\\')
				it+= 2;
			else if(*next_it == 'd' || *next_it == 'D' || *next_it == 'w' || *next_it == 'W' || *next_it == 's' || *next_it == 'S')
			{
				flags |= RegexFeatureFlag::SymbolClasses;
				it+= 2;
			}
			else if(*next_it == 'u')
			{
				flags |= RegexFeatureFlag::FourDigitHexCodes;
				it+= 2;
			}
			else
				++it;
		}
		else
			++it;
	}

	return flags;
}

} // namespace

RegexFeatureFlags GetRegexFeatures(const std::string& regex_str)
{
	RegexFeatureFlags flags= 0;
	if(StringContainsNonASCIISymbols(regex_str))
		flags|= RegexFeatureFlag::UTF8;

	flags|= GetExcapeSequencesFlags(regex_str);

	const auto parse_res= ParseRegexString(regex_str);
	if(const auto res= std::get_if<RegexElementsChain>(&parse_res))
		flags|= GetSequeneFeatures(*res);

	return flags;
}

bool StringContainsNonASCIISymbols(const std::string& str)
{
	for(const char c : str)
		if((c & 0b10000000) != 0)
			return true;

	return false;
}

std::unique_ptr<llvm::TargetMachine> CreateTargetMachine()
{
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmPrinters();
	llvm::InitializeAllAsmParsers();

	const llvm::Triple target_triple(llvm::sys::getDefaultTargetTriple());
	const std::string target_triple_str= target_triple.normalize();

	std::string error_str;
	const auto target= llvm::TargetRegistry::lookupTarget(target_triple_str, error_str);
	if(target == nullptr)
	{
		std::cerr << "Error, selecting target: " << error_str << std::endl;
		return nullptr;
	}

	llvm::SubtargetFeatures features;
	llvm::StringMap<bool> host_features;
	if(llvm::sys::getHostCPUFeatures(host_features))
		for(auto& f : host_features)
			features.AddFeature(f.first(), f.second);

	llvm::TargetOptions target_options;

	const auto code_gen_optimization_level= llvm::CodeGenOpt::Default;

	return
		std::unique_ptr<llvm::TargetMachine>(
			target->createTargetMachine(
				target_triple_str,
				llvm::sys::getHostCPUName(),
				features.getString(),
				target_options,
				llvm::Reloc::Model::PIC_,
				llvm::Optional<llvm::CodeModel::Model>(),
				code_gen_optimization_level));
}

} // namespace RegPanzer
