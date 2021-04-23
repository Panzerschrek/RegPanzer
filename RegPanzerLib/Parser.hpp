#pragma once
#include "RegexElements.hpp"
#include <optional>
#include <string_view>

namespace RegPanzer
{

// Parse regex in UTF-8 format.
std::optional<RegexElementsChain> ParseRegexString(std::string_view str);

} // namespace RegPanzer
