#pragma once
#include "RegexpElements.hpp"
#include <optional>
#include <string_view>

namespace RegPanzer
{

std::optional<RegexpElementsChain> ParseRegexpString(std::basic_string_view<CharType> str);

} // namespace RegPanzer
