#pragma once
#include <string>

namespace RegPanzer
{

bool StringContainsNonASCIISymbols(const std::string& str);
bool RegexContainsLazySequences(const std::string& regex_str);
bool RegexContainsPossessiveSequences(const std::string& regex_str);

} // namespace RegPanzer
