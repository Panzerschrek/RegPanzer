#pragma once
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
	};
};

using RegexFeatureFlags= size_t;

RegexFeatureFlags GetRegexFeatures(const std::string& regex_str);
bool StringContainsNonASCIISymbols(const std::string& str);

} // namespace RegPanzer
