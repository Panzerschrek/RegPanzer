#pragma once
#include "RegexpElements.hpp"
#include <optional>
#include <string_view>

namespace RegPanzer
{

// Parse regexp in UTF-8 format.
std::optional<RegexpElementsChain> ParseRegexpString(std::string_view str);

} // namespace RegPanzer
