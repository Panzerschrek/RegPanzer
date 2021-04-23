#include "../RegPanzerLib/Parser.hpp"
#include "Utils.hpp"

namespace RegPanzer
{

namespace
{

RegexFeatureFlags GetSequeneFeatures(const RegexElementsChain& chain);

template<typename T> RegexFeatureFlags GetSequeneFeaturesForElement(const T&){ return 0; }

RegexFeatureFlags GetSequeneFeaturesForElement(const Look&)
{
	return RegexFeatureFlag::Look;
}

RegexFeatureFlags GetSequeneFeaturesForElement(const Group& group)
{
	return GetSequeneFeatures(group.elements);
}

RegexFeatureFlags GetSequeneFeaturesForElement(const Alternatives& alternatives)
{
	RegexFeatureFlags flags= 0;
	for(const auto& alternative : alternatives.alternatives)
		flags|= GetSequeneFeatures(alternative);
	return flags;
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

} // namespace

RegexFeatureFlags GetRegexFeatures(const std::string& regex_str)
{
	RegexFeatureFlags flags= 0;
	if(StringContainsNonASCIISymbols(regex_str))
		flags|= RegexFeatureFlag::UTF8;

	if(const auto res= ParseRegexString(regex_str))
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

} // namespace RegPanzer
