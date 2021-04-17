#pragma once
#include "RegexpElements.hpp"
#include <optional>

namespace RegPanzer
{

using MatchResult= std::optional<std::basic_string_view<CharType>>;

MatchResult Match(const RegexpElementsChain& regexp, std::basic_string_view<CharType> str);

} // namespace RegPanzer
