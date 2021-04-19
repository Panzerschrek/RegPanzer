#pragma once
#include "RegexpElements.hpp"
#include <optional>
#include <string_view>

namespace RegPanzer
{

// Match UTF-8 string.
using MatchResult= std::optional<std::string_view>;
MatchResult Match(const RegexpElementsChain& regexp, std::string_view str);

} // namespace RegPanzer
