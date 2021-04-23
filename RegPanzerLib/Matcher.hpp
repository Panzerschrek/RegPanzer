#pragma once
#include "RegexElements.hpp"
#include <optional>
#include <string_view>

namespace RegPanzer
{

// Match UTF-8 string.
using MatchResult= std::optional<std::string_view>;
MatchResult Match(const RegexElementsChain& regex, std::string_view str);

} // namespace RegPanzer
