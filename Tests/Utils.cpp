#include "../RegPanzerLib/Parser.hpp"
#include "Utils.hpp"
#include <unordered_set>

namespace RegPanzer
{

namespace
{

using ModesSet= std::unordered_set<SequenceMode>;

void GetSequeneModes(const RegexElementsChain& chain, ModesSet& modes);

template<typename T> void GetSequeneModesForElement(const T&, ModesSet&){}

void GetSequeneModesForElement(const BracketExpression& bracket_expression, ModesSet& modes)
{
	GetSequeneModes(bracket_expression.elements, modes);
}

void GetSequeneModesForElement(const Alternatives& alternatives, ModesSet& modes)
{
	for(const auto& alternative : alternatives.alternatives)
		GetSequeneModes(alternative, modes);
}

void GetSequeneModes(const RegexElementsChain& chain, ModesSet& modes)
{
	for(const RegexElementFull& element : chain)
	{
		std::visit([&](const auto& el){ GetSequeneModesForElement(el, modes); }, element.el);
		modes.insert(element.seq.mode);
	}
}

} // namespace

bool StringContainsNonASCIISymbols(const std::string& str)
{
	for(const char c : str)
		if((c & 0b10000000) != 0)
			return true;

	return false;
}

bool RegexContainsLazySequences(const std::string& regex_str)
{
	const auto res= ParseRegexString(regex_str);
	if(res == std::nullopt)
		return false;

	ModesSet modes;
	GetSequeneModes(*res, modes);
	return modes.count(SequenceMode::Lazy) > 0;
}

bool RegexContainsPossessiveSequences(const std::string& regex_str)
{
	const auto res= ParseRegexString(regex_str);
	if(res == std::nullopt)
		return false;

	ModesSet modes;
	GetSequeneModes(*res, modes);
	return modes.count(SequenceMode::Possessive) > 0;
}

} // namespace RegPanzer
